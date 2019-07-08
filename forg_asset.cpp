inline Assets* DEBUGGetGameAssets(PlatformClientMemory* memory)
{
    GameState* gameState = (GameState*) memory->gameState;
    Assets* result = gameState->assets;
    return result;
}

inline void BeginAssetLock(Assets* assets)
{
    for(;;)
    {
        if(AtomicCompareExchangeUint32(&assets->gameState->lock, 1, 0) == 0)
        {
            break;
        }
    }
}

inline void EndAssetLock(Assets* assets)
{
    CompletePastWritesBeforeFutureWrites;
    assets->gameState->lock = 0;
}

inline AssetFile* GetFile(Assets* assets, u32 fileIndex)
{
    AssetFile* file = assets->files + fileIndex;
    return file;
}

inline PlatformFileHandle* GetHandleFor(Assets* assets, u32 fileIndex)
{
    Assert(fileIndex < assets->fileCount);
    PlatformFileHandle* result = &GetFile(assets, fileIndex)->handle;
    
    return result;
}

struct AssetMemorySize
{
    u32 data;
    u32 total;
};

inline void AddHeaderInFront(Assets* assets, AssetMemoryHeader* header)
{
    AssetMemoryHeader* sentinel = &assets->assetSentinel;
    
    header->next = sentinel->next;
    header->prev = sentinel;
    
    header->next->prev = header;
    header->prev->next = header;
}

inline void RemoveHeaderFromList(AssetMemoryHeader* header)
{
    header->next->prev = header->prev;
    header->prev->next = header->next;
    
    header->next = 0;
    header->prev = 0;
}



enum AssetBlockState
{
    AssetBlock_none,
    AssetBlock_used,
};

internal AssetMemoryBlock* InsertBlock(AssetMemoryBlock* prev, u64 size, void* memory)
{
    AssetMemoryBlock* block = (AssetMemoryBlock*) memory;
    
    Assert(size > sizeof(AssetMemoryBlock));
    block->size = size - sizeof(AssetMemoryBlock);
    
    block->prev = prev;
    block->next = prev->next;
    
    block->prev->next = block;
    block->next->prev = block;
    
    block->flags = 0;
    
    return block;
}

internal AssetMemoryBlock* FindBlockForSize(Assets* assets, u64 size)
{
    AssetMemoryBlock* result = 0;
    for(AssetMemoryBlock* current = assets->blockSentinel.next;
        current != &assets->blockSentinel;
        current = current->next)
    {
        if(!(current->flags & AssetBlock_used))
        {
            if(size <= current->size)
            {
                result = current;
                break;
            }
        }
    }
    
    return result;
}

internal b32 MergeIfPossible(Assets* assets, AssetMemoryBlock* first, AssetMemoryBlock* second)
{
    b32 result = false;
    
    if(first != &assets->blockSentinel && second != &assets->blockSentinel)
    {
        if(!(first->flags & AssetBlock_used) &&
           !(second->flags & AssetBlock_used))
        {
            u8* expectedSecond = (u8*) first + sizeof(AssetMemoryBlock) + first->size;
            if(expectedSecond == (u8*) second)
            {
                first->size += sizeof(AssetMemoryBlock) + second->size;
                
                second->next->prev = second->prev;
                second->prev->next = second->next;
                
                result = true;
            }
        }
    }
    
    return result;
}


enum FinalizeAssetOperation
{
    Finalize_none,
    Finalize_font,
    Finalize_Bitmap,
};

internal void AddOp(PlatformTextureOpQueue* queue, TextureOp source)
{
    BeginTicketMutex(&queue->mutex);
    
    Assert(queue->firstFree);
    TextureOp* dest = queue->firstFree;
    queue->firstFree = dest->next;
    *dest = source;
    Assert(dest->next == 0);
    
    if(queue->last)
    {
        queue->last = queue->last->next = dest;
    }
    else
    {
        queue->first = queue->last = dest;
    }
    
    EndTicketMutex(&queue->mutex);
}

internal AssetMemoryBlock* FreeAsset(Assets* assets, Asset* asset)
{
    AssetMemoryHeader* header = asset->header;
    Assert(asset->state == Asset_loaded);
    RemoveHeaderFromList(header);
    
    AssetMemoryBlock* result = (AssetMemoryBlock*) asset->header - 1;
    result->flags &= ~AssetBlock_used;
    
    if(MergeIfPossible(assets, result->prev, result))
    {
        result = result->prev;
    }
    MergeIfPossible(assets, result, result->next);
    
    asset->state = Asset_unloaded;
    asset->header = 0;
    
    return result;
}

inline void LockAsset(Assets* assets, u32 assetID)
{
	BeginAssetLock(assets);
    Asset* asset = assets->assets + assetID;
	++asset->lockCounter;
	EndAssetLock(assets);
}

inline void UnlockAsset(Assets* assets, u32 assetID)
{
	BeginAssetLock(assets);
    Asset* asset = assets->assets + assetID;
	Assert(asset->lockCounter > 0);
	--asset->lockCounter;
	EndAssetLock(assets);
}

internal AssetMemoryHeader* AcquireAssetMemory(Assets* assets, u32 size, u32 assetIndex, AssetHeaderType assetType)
{
    AssetMemoryHeader* result = 0;
    
    BeginAssetLock(assets);
    AssetMemoryBlock* block = FindBlockForSize(assets, size);
    for(;;)
    {
        if(block && size <= block->size)
        {
            block->flags |= AssetBlock_used;
            
            result = (AssetMemoryHeader*) (block + 1);
            u64 remainingSize = block->size - size;
            
            u64 splitThreshold = KiloBytes(4);
            if(remainingSize >= splitThreshold)
            {
                block->size -= remainingSize;
                InsertBlock(block, remainingSize, (u8*) result + size);
            }
            
            break;
        }
        else
        {
            for(AssetMemoryHeader* header = assets->assetSentinel.prev;
                header != &assets->assetSentinel;
                header = header->prev)
            {
                Asset* asset = assets->assets + header->assetIndex;
                if(asset->state == Asset_loaded && asset->lockCounter == 0)
                {
                    block = FreeAsset(assets, asset);
                    break;
                }
            }
        }
    }
    
    Assert(result);
    result->assetIndex = assetIndex;
    result->assetType = assetType;
    result->totalSize = size;
    AddHeaderInFront(assets, result);
    
    EndAssetLock(assets);
    
    return result;
}

struct LoadAssetWork
{
    u32 finalizeOperation;
    PlatformFileHandle* handle;
    u32 memorySize;
    void* dest;
    u64 dataOffset;
    
    Asset* asset;
    u32 finalState;
    TaskWithMemory* task;
    
    PlatformTextureOpQueue* textureQueue;
};

internal void LoadAssetDirectly(LoadAssetWork* work)
{
    platformAPI.ReadFromFile(work->handle, work->dataOffset, work->memorySize, work->dest);
    if(PlatformNoFileErrors(work->handle))
    {
        switch(work->finalizeOperation)
        {
            case Finalize_none:
            {
                
            } break;
            
            case Finalize_font:
            {
                Asset* asset = work->asset;
                Font* font = &asset->header->font;
                PakFont* info = &asset->paka.font;
                
                
                for(u32 glyphIndex = 0; glyphIndex < info->glyphsCount; glyphIndex++)
                {
                    PakGlyph* glyph = font->glyphs + glyphIndex;
                    
                    Assert((u16) glyphIndex == glyphIndex);
                    Assert(glyph->unicodeCodePoint < info->onePastHighestCodePoint);
                    font->unicodeMap[glyph->unicodeCodePoint] = (u16) glyphIndex;
                }
                
            } break;
            
            case Finalize_Bitmap:
            {
                Bitmap* bitmap = &work->asset->header->bitmap;
                TextureOp op = {};
                op.update.data = bitmap->pixels;
                op.update.texture = work->asset->textureHandle;
                AddOp(work->textureQueue, op);
            } break;
            
            InvalidCodePath;
        }
        
        CompletePastWritesBeforeFutureWrites;
        work->asset->state = work->finalState;
    }
    
}

PLATFORM_WORK_CALLBACK(LoadAssetThreaded)
{
    TIMED_FUNCTION();
    LoadAssetWork* work = (LoadAssetWork*) param;
    LoadAssetDirectly(work);
    EndTaskWithMemory(work->task);
}

inline u32 AcquireTextureHandle(Assets* assets)
{
    u32 result = 0;
    if(assets->nextFreeTextureHandle <= assets->maxTextureHandleIndex)
    {
        result = assets->nextFreeTextureHandle++;
    }
    else
    {
        AssetLRULink* sentinel = &assets->LRUSentinel;
        Assert(!DLLIST_ISEMPTY(sentinel));
        
        AssetLRULink* free = 0;
        AssetLRULink* first = sentinel->next;
        while(true)
        {
            Asset* test = (Asset*) first;
            if(test->lockCounter > 0)
            {
                free = first;
                break;
            }
            
            first = first->next;
            Assert(first != sentinel);
        }
        Assert(free);
        DLLIST_REMOVE(free);
        
        Asset* LRU = (Asset*) free;
        result = LRU->textureHandle.index;
        Clear(&LRU->textureHandle);
        
        if(LRU->state == Asset_loaded)
        {
            FreeAsset(assets, LRU);
        }
    }
    
    return result;
}

inline void RefreshSpecialTexture(Assets* assets, AssetLRULink* LRU)
{
    DLLIST_REMOVE(LRU);
    DLLIST_INSERT(&assets->specialLRUSentinel, LRU);
}

inline u32 AcquireSpecialTextureHandle(Assets* assets)
{
    u32 result = 0;
    if(assets->nextFreeSpecialTextureHandle <= assets->maxSpecialTextureHandleIndex)
    {
        result = assets->nextFreeSpecialTextureHandle++;
    }
    else
    {
        AssetLRULink* sentinel = &assets->specialLRUSentinel;
        Assert(!DLLIST_ISEMPTY(sentinel));
        
        AssetLRULink* first = sentinel->next;
        DLLIST_REMOVE(first);
        
        WorldChunk* LRU = (WorldChunk*) first;
        result = LRU->textureHandle.index;
        Clear(&LRU->textureHandle);
    }
    
    return result;
}

void LoadBitmap(Assets* assets, BitmapId ID, b32 immediate)
{
    if(ID.value)
    {
        Asset* asset = assets->assets + ID.value;
        if(AtomicCompareExchangeUint32((u32*) &asset->state, Asset_queued, Asset_unloaded) == Asset_unloaded)
        {
            TaskWithMemory* task = 0;
            if(!immediate)
            {
                task = BeginTaskWithMemory(assets->gameState->tasks, ArrayCount(assets->gameState->tasks), false);
            }
            
            if(task || immediate)
            {
                PakBitmap* info = &asset->paka.bitmap;
                AssetMemorySize size = {};
                size.data = info->dimension[0] * info->dimension[1] * 4;
                size.total = size.data + sizeof(AssetMemoryHeader);
                
                asset->header = AcquireAssetMemory(assets, size.total, ID.value, AssetType_Bitmap);
                Bitmap* bitmap = &asset->header->bitmap;
                
                bitmap->width = SafeTruncateToU16(info->dimension[0]);
                bitmap->height = SafeTruncateToU16(info->dimension[1]);
                bitmap->nativeHeight = info->nativeHeight;
                bitmap->pivot = V2(info->align[0], info->align[1]);
                bitmap->widthOverHeight = (r32) bitmap->width / (r32) bitmap->height;
                bitmap->pixels = asset->header + 1;
                
                u32 textureHandle = AcquireTextureHandle(assets);
                asset->textureHandle = TextureHandle(textureHandle, bitmap->width, bitmap->height);
                bitmap->textureHandle = asset->textureHandle;
                
                LoadAssetWork work = {};
                work.handle = GetHandleFor(assets, asset->fileIndex);
                work.dest = asset->header + 1;
                work.memorySize = size.data;
                work.dataOffset = asset->paka.dataOffset;
                work.asset = asset;
                work.finalState = Asset_loaded;
                work.task = task;
                work.finalizeOperation = Finalize_Bitmap;
                work.textureQueue = assets->textureQueue;
                
                if(!immediate)
                {
                    Assert(work.task);
                    LoadAssetWork* assetWork = PushStruct(&task->pool, LoadAssetWork);
                    *assetWork = work;
                    platformAPI.PushWork(assets->gameState->slowQueue, LoadAssetThreaded, assetWork);
                }
                else
                {
                    LoadAssetDirectly(&work);
                }
            }
            else
            {
                asset->state = Asset_unloaded;
            }
        }
        else if(immediate)
        {
            u32 volatile* state = (u32 volatile*) &asset->state;
            while(*state == Asset_queued) {}
        }
    }
}

internal void LoadFont(Assets* assets, FontId ID, b32 immediate)
{
    if(ID.value)
    {
        Asset* asset = assets->assets + ID.value;
        if(AtomicCompareExchangeUint32((u32*) &asset->state, Asset_queued, 
                                       Asset_unloaded) == Asset_unloaded)
        {
            TaskWithMemory* task = 0;
            if(!immediate)
            {
                task = BeginTaskWithMemory(assets->gameState->tasks,
                                           ArrayCount(assets->gameState->tasks), false);
            }
            
            if(task || immediate)
            {
                PakFont* info = &asset->paka.font;
                
                u32 glyphsSize = info->glyphsCount * sizeof(PakGlyph);
                u32 horizontalAdvanceSize = sizeof(r32) * info->glyphsCount * info->glyphsCount; 
                AssetMemorySize size = {};
                
                u32 unicodeMapSize = sizeof(u16) * info->onePastHighestCodePoint;
                size.data = glyphsSize + horizontalAdvanceSize;
                size.total = size.data + unicodeMapSize + sizeof(AssetMemoryHeader);
                
                asset->header = AcquireAssetMemory(assets, size.total, ID.value, AssetType_None);
                Font* font = &asset->header->font;
                
                AssetFile* file = GetFile(assets, asset->fileIndex);
                
                font->offsetRebaseGlyphs = file->fontBitmapsOffsetIndex;
                
                font->glyphs = (PakGlyph*) (asset->header + 1);
                font->horizontalAdvance = (r32*) ((u8*) font->glyphs + glyphsSize);
                font->unicodeMap = (u16*) ((u8*) font->horizontalAdvance + horizontalAdvanceSize);
                ZeroSize(unicodeMapSize, font->unicodeMap);
                
                LoadAssetWork work = {};
                work.handle = GetHandleFor(assets, asset->fileIndex);
                work.dest = asset->header + 1;
                work.memorySize = size.data;
                work.dataOffset = asset->paka.dataOffset;
                work.asset = asset;
                work.finalState = Asset_loaded;
                work.finalizeOperation = Finalize_font;
                
                work.task = task;
                
                if(!immediate)
                {
                    Assert(work.task);
                    LoadAssetWork* assetWork = PushStruct(&task->pool, LoadAssetWork);
                    *assetWork = work;
                    platformAPI.PushWork(assets->gameState->slowQueue, 
                                         LoadAssetThreaded, assetWork);
                }
                else
                {
                    LoadAssetDirectly(&work);
                }
            }
            else
            {
                asset->state = Asset_unloaded;
            }
        }
        else if(immediate)
        {
            u32 volatile* state = (u32 volatile*) &asset->state;
            while(*state == Asset_queued) {}
        }
    }
}


void LoadSound(Assets* assets, SoundId ID)
{
    if(ID.value)
    {
        Asset* asset = assets->assets + ID.value;
        TaskWithMemory* task = BeginTaskWithMemory(assets->gameState->tasks,
                                                   ArrayCount(assets->gameState->tasks), false);
        if(task)
        {
            if(AtomicCompareExchangeUint32((u32*) &asset->state, Asset_queued, 
                                           Asset_unloaded) == Asset_unloaded)
            {
                PakSound* info = &asset->paka.sound;
                
                AssetMemorySize size = {};
                size.data = info->channelCount * info->sampleCount * sizeof(i16);
                size.total = size.data + sizeof(AssetMemoryHeader);
                
                asset->header = AcquireAssetMemory(assets, size.total, ID.value, AssetType_None);
                void* memory = asset->header + 1;
                
                Sound* sound = &asset->header->sound;
                sound->countChannels = info->channelCount;
                sound->countSamples = info->sampleCount;
                Assert(info->channelCount <= ArrayCount(sound->samples));
                
                u8* sampleStart = (u8*) memory;
                for(u32 channelIndex = 0; channelIndex < sound->countChannels; channelIndex++)
                {
                    sound->samples[channelIndex] = (i16*) sampleStart;
                    sampleStart += sizeof(i16) * sound->countSamples;
                }
                
                LoadAssetWork* work = PushStruct(&task->pool, LoadAssetWork);
                work->handle = GetHandleFor(assets, asset->fileIndex);
                work->dest = memory;
                work->memorySize = size.data;
                work->dataOffset = asset->paka.dataOffset;
                work->asset = asset;
                work->finalState = Asset_loaded;
                work->task = task;
                
                platformAPI.PushWork(assets->gameState->slowQueue, LoadAssetThreaded, work);
            }
            else
            {
                EndTaskWithMemory(task);
            }
        }
    }
}

void LoadAnimation(Assets* assets, AnimationId ID)
{
    if(ID.value)
    {
        Asset* asset = assets->assets + ID.value;
        TaskWithMemory* task = BeginTaskWithMemory(assets->gameState->tasks, ArrayCount(assets->gameState->tasks), false);
        if(task)
        {
            if(AtomicCompareExchangeUint32((u32*) &asset->state, Asset_queued, 
                                           Asset_unloaded) == Asset_unloaded)
            {
                PakAnimation* info = &asset->paka.animation;
                
                u32 countBones = info->boneCount;
                u32 countAss = info->assCount;
                u32 frameCount = info->frameCount;
                u32 spriteCount = info->spriteCount;
                
                AssetMemorySize size = {};
                size.data = sizeof(AnimationHeader) + spriteCount * sizeof(SpriteInfo) + frameCount * sizeof(FrameData) + countAss * sizeof(PieceAss) + countBones * sizeof(Bone);
                size.total = size.data + sizeof(AssetMemoryHeader);
                
                asset->header = AcquireAssetMemory(assets, size.total, ID.value, AssetType_None);
                void* memory = asset->header + 1;
                u8* base = (u8*) memory;
                
                Animation* animation = &asset->header->animation;
                animation->header = (AnimationHeader*) base;
                
                
                animation->spriteInfoCount = spriteCount;
                animation->frameCount = frameCount;
                animation->spriteInfos = (SpriteInfo*) (base + sizeof(AnimationHeader));
                animation->frames = (FrameData*) (base + sizeof(AnimationHeader) + spriteCount * sizeof(SpriteInfo));
                animation->bones = (Bone*) (base + sizeof(AnimationHeader) + frameCount * sizeof(FrameData) + spriteCount * sizeof(SpriteInfo));
                animation->ass = (PieceAss*) (base + sizeof(AnimationHeader) + frameCount * sizeof(FrameData) + spriteCount * sizeof(SpriteInfo) + countBones * sizeof(Bone));
                
                LoadAssetWork* work = PushStruct(&task->pool, LoadAssetWork);
                
                work->handle = GetHandleFor(assets, asset->fileIndex);
                work->dest = memory;
                work->memorySize = size.data;
                work->dataOffset = asset->paka.dataOffset;
                work->asset = asset;
                work->finalState = Asset_loaded;
                work->task = task;
                
                
                platformAPI.PushWork(assets->gameState->slowQueue, LoadAssetThreaded, work);
            }
            else
            {
                EndTaskWithMemory(task);
            }
        }
    }
}


void LoadModel(Assets* assets, ModelId ID)
{
    if(ID.value)
    {
        Asset* asset = assets->assets + ID.value;
        TaskWithMemory* task = BeginTaskWithMemory(assets->gameState->tasks, ArrayCount(assets->gameState->tasks), false);
        if(task)
        {
            if(AtomicCompareExchangeUint32((u32*) &asset->state, Asset_queued, 
                                           Asset_unloaded) == Asset_unloaded)
            {
                PakModel* info = &asset->paka.model;
                
                u32 vertexCount = info->vertexCount;
                u32 faceCount = info->faceCount;
                
                AssetMemorySize size = {};
                size.data = vertexCount * sizeof(ColoredVertex) + faceCount * sizeof(ModelFace);
                size.total = size.data + sizeof(AssetMemoryHeader);
                
                asset->header = AcquireAssetMemory(assets, size.total, ID.value, AssetType_None);
                void* memory = asset->header + 1;
                u8* base = (u8*) memory;
                
                VertexModel* model = &asset->header->model;
                
                model->vertexCount = vertexCount;
                model->faceCount = faceCount;
                
                model->dim = info->dim;
                
                model->vertexes = (ColoredVertex*) base;
                model->faces = (ModelFace*) (base + vertexCount * sizeof(ColoredVertex));
                
                LoadAssetWork* work = PushStruct(&task->pool, LoadAssetWork);
                
                work->handle = GetHandleFor(assets, asset->fileIndex);
                work->dest = memory;
                work->memorySize = size.data;
                work->dataOffset = asset->paka.dataOffset;
                work->asset = asset;
                work->finalState = Asset_loaded;
                work->task = task;
                
                platformAPI.PushWork(assets->gameState->slowQueue, LoadAssetThreaded, work);
            }
            else
            {
                EndTaskWithMemory(task);
            }
        }
    }
}

inline b32 IsValid(BitmapId ID)
{
    b32 result = (ID.value != 0);
    return result;
}

inline b32 IsValid(FontId ID)
{
    b32 result = (ID.value != 0);
    return result;
}

inline b32 IsValid(SoundId ID)
{
    b32 result = (ID.value != 0);
    return result;
}

inline b32 IsValid(AnimationId ID)
{
    b32 result = (ID.value != 0);
    return result;
}

inline b32 IsValid(ModelId ID)
{
    b32 result = (ID.value != 0);
    return result;
}


inline void LockBitmap(Assets* assets, BitmapId ID)
{
    Assert(IsValid(ID));
    LockAsset(assets, ID.value);
}

inline void UnlockBitmap(Assets* assets, BitmapId ID)
{
    Assert(IsValid(ID));
    UnlockAsset(assets, ID.value);
}

internal void CloseAllHandles(Assets* assets)
{
    for(u32 fileIndex = 0; fileIndex < assets->fileCount; ++fileIndex)
    {
        AssetFile* file = assets->files + fileIndex;
        platformAPI.CloseHandle(&file->handle);
    }
}

internal Assets* InitAssets(GameState* gameState, MemoryPool* pool, PlatformTextureOpQueue* textureQueue, memory_index size)
{
    Assets* assets = PushStruct(pool, Assets); 
    assets->gameState = gameState;
    
    
    DLLIST_INIT(&assets->LRUSentinel);
    DLLIST_INIT(&assets->specialLRUSentinel);
    
    assets->blockSentinel.prev = &assets->blockSentinel;
    assets->blockSentinel.next = &assets->blockSentinel;
    assets->blockSentinel.flags = 0;
    assets->blockSentinel.size = 0;
    
    InsertBlock(&assets->blockSentinel, size, (AssetMemoryBlock*)PushSize(pool, size, NoClear()));
    assets->textureQueue = textureQueue;
    
    assets->whitePixel = 0xFFFFFFFF;
    TextureOp op = {};
    op.update.data = &assets->whitePixel;
    op.update.texture = TextureHandle(0, 1, 1);
    AddOp(assets->textureQueue, op);
    
    assets->redPixel = 0xFF0000FF;
    TextureOp specialOp = {};
    op.update.data = &assets->redPixel;
    op.update.texture = TextureHandle(MAX_TEXTURE_COUNT, 1, 1);
    AddOp(assets->textureQueue, op);
    
    assets->nextFreeTextureHandle = 1;
    assets->maxTextureHandleIndex = MAX_TEXTURE_COUNT - 1;
    
    assets->nextFreeSpecialTextureHandle = MAX_TEXTURE_COUNT + 1;
    assets->maxSpecialTextureHandleIndex = MAX_TEXTURE_COUNT + MAX_SPECIAL_TEXTURE_COUNT - 1;
    
    assets->assetSentinel.next = &assets->assetSentinel;
    assets->assetSentinel.prev = &assets->assetSentinel;
    assets->assetSentinel.assetIndex = 0;
    
    char* assetPath = "assets";
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(PlatformFile_uncompressedAsset, assetPath);
    
    assets->fileCount = fileGroup.fileCount;
    assets->files = PushArray(pool, AssetFile, assets->fileCount);
    
    assets->tagCount = 1;
    assets->assetCount = 1;
    
    for(u32 fileIndex = 0; fileIndex < fileGroup.fileCount; fileIndex++)
    {
        AssetFile* file = assets->files + fileIndex;
        file->fontBitmapsOffsetIndex = 0;
        
        file->tagBase = assets->tagCount;
        
        file->handle = platformAPI.OpenNextFile(&fileGroup, assetPath);
        ZeroStruct(file->header);
        platformAPI.ReadFromFile(&file->handle, sizeof(u64), sizeof(file->header), &file->header);
        
        u32 assetTypeArraySize = file->header.assetTypeCount * sizeof(PakAssetType);
        file->assetTypes = (PakAssetType*) PushSize(pool, assetTypeArraySize);
        
        platformAPI.ReadFromFile(&file->handle, file->header.assetTypeOffset, assetTypeArraySize, file->assetTypes);
        if(file->header.magicValue != PAK_MAGIC_NUMBER)
        {
            platformAPI.FileError(&file->handle, "magic number not valid");
        }
        
        if(file->header.version != PAK_VERSION)
        {
            platformAPI.FileError(&file->handle, "pak file version not valid");
        }
        
        if(PlatformNoFileErrors(&file->handle))
        {
            assets->tagCount += (file->header.tagCount - 1);
            // NOTE(Leonardo): take into account the null asset!
            assets->assetCount += (file->header.assetcount - 1);
        }
        else
        {
            // TODO(Leonardo): notify user! so that he can download the file again.
            InvalidCodePath;
        }
    }
    
    platformAPI.GetAllFilesEnd(&fileGroup);
    
    assets->tags = PushArray(pool, PakTag, assets->tagCount);
    assets->assets = PushArray(pool, Asset, assets->assetCount);
    
    // NOTE(Leonardo): rebase the tags!
    for(u32 fileIndex = 0; fileIndex < assets->fileCount; fileIndex++)
    {
        AssetFile* file = assets->files + fileIndex;
        
        // NOTE(Leonardo): account for the null tag!
        u32 tagArraySize = (file->header.tagCount - 1) * sizeof(PakTag);
        platformAPI.ReadFromFile(&file->handle, file->header.tagOffset + sizeof(PakTag), 
                                 tagArraySize, assets->tags + file->tagBase);
    }
    
    // NOTE(Leonardo): zero the null asset!
    ZeroStruct(assets->assets[0]);
    u32 assetCount = 1;
    
    // NOTE(Leonardo): zero the null tag!
    ZeroStruct(assets->tags[0]);
    
    for(u32 destTypeID = 0; destTypeID < Asset_count + HASHED_ASSET_SLOTS; destTypeID++)
    {
        AssetType* destType = assets->types + destTypeID;
        
        destType->firstAssetIndex = assetCount;
        
        for(u32 fileIndex = 0; fileIndex < assets->fileCount; fileIndex++)
        {
            AssetFile* file = assets->files + fileIndex;
            
            if(PlatformNoFileErrors(&file->handle))
            {
                for(u32 sourceTypeID = 0; 
                    sourceTypeID < file->header.assetTypeCount; 
                    sourceTypeID++)
                {
                    PakAssetType* sourceType = file->assetTypes + sourceTypeID;
                    if(sourceType->ID == destTypeID)
                    {
                        if(sourceType->ID == Asset_glyph)
                        {
                            file->fontBitmapsOffsetIndex = assetCount - sourceType->firstAssetIndex;
                        }
                        TempMemory assetMemory = BeginTemporaryMemory(pool);
                        
                        u32 assetCountForType = sourceType->onePastLastAssetIndex - sourceType->firstAssetIndex;
                        u32 assetArraySize = assetCountForType * sizeof(PakAsset);
                        
                        u64 assetOffset = file->header.assetOffset + sourceType->firstAssetIndex * sizeof(PakAsset);
                        PakAsset* pakAssetArray = (PakAsset*) PushSize(pool, assetArraySize, NoClear());
                        
                        platformAPI.ReadFromFile(&file->handle, assetOffset, assetArraySize, pakAssetArray);
                        
                        for(u32 assetIndex = 0; assetIndex < assetCountForType; assetIndex++)
                        {
                            Assert(assetCount < assets->assetCount);
                            Asset* dest = assets->assets + assetCount++;
                            PakAsset* source = pakAssetArray + assetIndex;
                            
                            dest->paka = *source;
                            
                            if(dest->paka.firstTagIndex == 0)
                            {
                                dest->paka.onePastLastTagIndex = 0;
                            }
                            else
                            {
                                dest->paka.firstTagIndex += (file->tagBase - 1);
                                dest->paka.onePastLastTagIndex += (file->tagBase - 1);
                            }
                            dest->fileIndex = fileIndex;
                        }
                        
                        EndTemporaryMemory(assetMemory);
                    }
                }
            }
        }
        destType->onePastLastAssetIndex = assetCount;
    }
    Assert(assetCount == assets->assetCount);
    
    return assets;
}

inline u32 GetRandomAsset_(Assets* assets, u32 assetID, RandomSequence* seq)
{
    u32 result = {};
    AssetType* type = assets->types + assetID;
    
    u32 range = type->onePastLastAssetIndex - type->firstAssetIndex;
    if(range > 0)
    {
        u32 choice = RandomChoice(seq, range);
        result = type->firstAssetIndex + choice;
    }
    
    return result;
}



inline u32 GetFirstAsset_(Assets* assets, u32 assetID)
{
    u32 result = 0;
    AssetType* type = assets->types + assetID;
    if(type->onePastLastAssetIndex > type->firstAssetIndex)
    {
        result = type->firstAssetIndex;
    }
    
    return result;
}

inline b32 IsLabel(u32 ID)
{
    b32 result = (ID >= Tag_count);
    return result;
}

struct FindAnimationResult
{
    AnimationId ID;
    AssetTypeId assetType;
};

inline ModelId FindModelByName(Assets* assets, u64 typeHashID, u64 nameHashID)
{
    ModelId result = {};
    
    u32 assetID = Asset_count + (typeHashID & (HASHED_ASSET_SLOTS - 1));
    AssetType* type = assets->types + assetID;
    for(u32 assetIndex = type->firstAssetIndex; 
        assetIndex < type->onePastLastAssetIndex;
        assetIndex++)
    {
        Asset* asset = assets->assets + assetIndex;
        if(asset->paka.typeHashID == typeHashID && asset->paka.nameHashID == nameHashID)
        {
            result.value = assetIndex;
            break;
        }
	}
    
    
    return result;
}

inline FindAnimationResult FindAnimationByName(Assets* assets, u64 skeletonHashID, u64 animationNameHashID)
{
    FindAnimationResult result = {};
    
	for(u32 assetType = Asset_rig; assetType < Asset_AnimationLast && !result.assetType; ++assetType)
	{
        AssetType* type = assets->types + assetType;
		for(u32 assetIndex = type->firstAssetIndex; 
            assetIndex < type->onePastLastAssetIndex;
            assetIndex++)
		{
			Asset* asset = assets->assets + assetIndex;
			if(asset->paka.typeHashID == skeletonHashID && asset->paka.nameHashID == animationNameHashID)
			{
                result.assetType = (AssetTypeId) assetType;
                result.ID = {assetIndex};
                break;
			}
		}
	}
    
    return result;
}

inline SoundId FindSoundByName(Assets* assets, u64 typeHashID, u64 nameHashID)
{
    SoundId result = {};
    
    u32 assetID = Asset_count + (typeHashID & (HASHED_ASSET_SLOTS - 1));
    AssetType* type = assets->types + assetID;
    
    for(u32 assetIndex = type->firstAssetIndex; 
        assetIndex < type->onePastLastAssetIndex;
        assetIndex++)
    {
        Asset* asset = assets->assets + assetIndex;
        if(asset->paka.typeHashID == typeHashID && asset->paka.nameHashID == nameHashID)
        {
            result = {assetIndex};
            break;
        }
	}
    
    return result;
}

inline BitmapId FindBitmapByName(Assets* assets, u32 assetType, u64 typeHashID, u64 nameHashID, u16 colorationIndex)
{
    BitmapId result = {};
    
    AssetType* type = assets->types + assetType;
    for(u32 assetIndex = type->firstAssetIndex; 
        assetIndex < type->onePastLastAssetIndex;
        assetIndex++)
    {
        Asset* asset = assets->assets + assetIndex;
        if(asset->paka.typeHashID == typeHashID && asset->paka.nameHashID == nameHashID && asset->paka.offsetFromOriginalOffset == colorationIndex)
        {
            result.coloration = V4(1, 1, 1, 1);
            result.value = assetIndex;
            
            u16 clonedOffset = asset->paka.offsetFromOriginalOffset;
            if(clonedOffset)
            {
                result.value = assetIndex - (u32) clonedOffset;
                result.coloration = asset->paka.coloration.coloration;
            }
            break;
        }
	}
    
    
    return result;
}


inline BitmapId FindBitmapByName(Assets* assets, u32 assetID, u64 nameHashID)
{
    BitmapId result = {};
    result = FindBitmapByName(assets, assetID, 0, nameHashID, 0);
    return result;
}

inline BitmapId FindBitmapByName(Assets* assets, u64 typeHashID, u64 nameHashID, u16 colorationIndex)
{
    BitmapId result = {};
    
    u32 assetID = Asset_count + (typeHashID & (HASHED_ASSET_SLOTS - 1));
    result = FindBitmapByName(assets, assetID, typeHashID, nameHashID, colorationIndex);
    return result;
}

inline MatchingAssetResult GetMatchingAsset_(Assets* assets, u32 assetID, u64 stringHashID,
                                             TagVector* values, TagVector* weights, LabelVector* labels)
{
    MatchingAssetResult result = {};
    
    r32 bestDiff = R32_MAX;
    AssetType* type = assets->types + assetID;
    u32 bestAsset = 0;
    
    for(u32 assetIndex = type->firstAssetIndex; 
        assetIndex < type->onePastLastAssetIndex;
        assetIndex++)
    {
        Asset* asset = assets->assets + assetIndex;
        if(asset->paka.typeHashID == stringHashID)
        {
            r32 currentDiff = 0.0f;
            for(u32 tagIndex = asset->paka.firstTagIndex; 
                tagIndex < asset->paka.onePastLastTagIndex;
                tagIndex++)
            {
                PakTag* tag = assets->tags + tagIndex;
                if(!IsLabel(tag->ID))
                {
                    r32 tagDiff = Abs(tag->value - values->E[tag->ID]) * weights->E[tag->ID];
                    currentDiff += tagDiff;
                }
            }
            
            
            if(labels)
            {
                for(u32 labelIndex = 0; labelIndex - labels->labelCount; ++labelIndex)
                {
                    u32 searchingID = labels->IDs[labelIndex];
                    r32 tagDiff = labels->values[labelIndex];
                    
                    for(u32 tagIndex = asset->paka.firstTagIndex; 
                        tagIndex < asset->paka.onePastLastTagIndex;
                        tagIndex++)
                    {
                        PakTag* tag = assets->tags + tagIndex;
                        if(IsLabel(tag->ID) && tag->ID == searchingID)
                        {
                            tagDiff = Abs(labels->values[labelIndex] - tag->value);
                            break;
                        }
                        
                    }
                    
                    currentDiff += tagDiff;
                }
                
            }
            
            if(currentDiff < bestDiff)
            {
                bestDiff = currentDiff;
                
                u16 clonedOffset = asset->paka.offsetFromOriginalOffset;
                if(clonedOffset)
                {
                    result.cloned = true;
                    result.clonedColoration = asset->paka.coloration.coloration;
                }
                result.assetIndex = assetIndex - (u32) clonedOffset;
                Assert(result.assetIndex > 0);
            }
        }
    }
    
    return result;
}


inline AssetMemoryHeader* GetAsset(Assets* assets, u32 ID, b32 insertAtLRUList = false)
{
    AssetMemoryHeader* result = 0;
    
    Assert(ID < assets->assetCount);
    Asset* asset = assets->assets + ID;
    
    BeginAssetLock(assets);
    if(asset->state == Asset_loaded)
    {
        result = asset->header;
        
        if(insertAtLRUList)
        {
            if(IsValid(&asset->textureHandle))
            {
                DLLIST_REMOVE(&asset->LRU);
                DLLIST_INSERT(&assets->LRUSentinel, &asset->LRU);
            }
        }
        
        RemoveHeaderFromList(result);
        AddHeaderInFront(assets, result);
        
        CompletePastWritesBeforeFutureWrites;
    }
    
    EndAssetLock(assets);
    
    return result;
}

inline Bitmap* GetBitmap(Assets* assets, BitmapId ID)
{
    Assert(IsValid(ID));
    
    AssetMemoryHeader* header = GetAsset(assets, ID.value, true);
    Bitmap* result = 0;
    
    if(header)
    {
        result = &header->bitmap;
    }
    
    return result;
}

inline Font* GetFont(Assets* assets, FontId ID)
{
    Assert(IsValid(ID));
    
    AssetMemoryHeader* header = GetAsset(assets, ID.value);
    Font* result = 0;
    
    if(header)
    {
        result = &header->font;
    }
    return result;
}

inline Sound* GetSound(Assets* assets, SoundId ID)
{
    Assert(IsValid(ID));
    
    AssetMemoryHeader* header = GetAsset(assets, ID.value);
    Sound* result = 0;
    
    if(header)
    {
        result = &header->sound;
    }
    
    return result;
}

inline Animation* GetAnimation(Assets* assets, AnimationId ID)
{
    Assert(IsValid(ID));
    
    AssetMemoryHeader* header = GetAsset(assets, ID.value);
    Animation* result = 0;
    
    if(header)
    {
        result = &header->animation;
    }
    
    return result;
}

inline VertexModel* GetModel(Assets* assets, ModelId ID)
{
    Assert(IsValid(ID));
    
    AssetMemoryHeader* header = GetAsset(assets, ID.value);
    VertexModel* result = 0;
    
    if(header)
    {
        result = &header->model;
    }
    
    return result;
}

inline PakBitmap* GetBitmapInfo(Assets* assets, BitmapId ID)
{
    Assert(IsValid(ID));
    PakBitmap* result = &assets->assets[ID.value].paka.bitmap;
    return result;
}

inline PakFont* GetFontInfo(Assets* assets, FontId ID)
{
    Assert(IsValid(ID));
    PakFont* result = &assets->assets[ID.value].paka.font;
    return result;
}

inline PakSound* GetSoundInfo(Assets* assets, SoundId ID)
{
    Assert(IsValid(ID));
    PakSound* result = &assets->assets[ID.value].paka.sound;
    return result;
}

inline PakAnimation* GetAnimationInfo(Assets* assets, AnimationId ID)
{
    Assert(IsValid(ID));
    PakAnimation* result = &assets->assets[ID.value].paka.animation;
    return result;
}

inline PakModel* GetModelInfo(Assets* assets, ModelId ID)
{
    Assert(IsValid(ID));
    PakModel* result = &assets->assets[ID.value].paka.model;
    return result;
}

inline SoundId GetNextSoundInChain(Assets* assets, SoundId ID)
{
    SoundId result = {};
    PakSound* info = GetSoundInfo(assets, ID);
    switch(info->chain)
    {
        case Chain_none:
        {
            
        } break;
        
        case Chain_loop:
        {
            InvalidCodePath;
            result = ID;
        } break;
        
        case Chain_next:
        {
            result = { ID.value + 1 };
        }
    }
    
    return result;
}

inline u32 GetGlyphFromCodePoint(Font* font, PakFont* info, u32 desired)
{
    u32 result = 0;
    if(desired < info->onePastHighestCodePoint)
    {
        result = font->unicodeMap[desired];
        Assert(result < info->glyphsCount);
    }
    return result;
}

internal r32 GetHorizontalAdvanceForPair(Font* font, PakFont* info, u32 desiredPrevCodePoint, u32 desiredCodePoint)
{
    u32 glyph = GetGlyphFromCodePoint(font, info, desiredCodePoint);
    u32 prevGlyph = GetGlyphFromCodePoint(font, info, desiredPrevCodePoint);
    
    r32 result = font->horizontalAdvance[prevGlyph * info->glyphsCount + glyph];
    return result;
}

internal r32 GetLineAdvance(PakFont* info)
{
    r32 result = info->externalLeading + info->ascenderHeight + info->descenderHeight;
    return result;
}

internal r32 GetStartingLineY(PakFont* info)
{
    r32 result = info->ascenderHeight;
    return result;
}


inline BitmapId GetBitmapForGlyph(Assets* assets, Font* font, PakFont* info, u32 desiredCodePoint)
{
    u32 glyph = GetGlyphFromCodePoint(font, info, desiredCodePoint);
    BitmapId result = font->glyphs[glyph].bitmapId;
    if(result.value)
    {
        result.value += font->offsetRebaseGlyphs;
    }
    
    return result;
    
}

inline AssetTypeId GetAssetIDForEntity(Assets* assets, TaxonomyTable* table, u32 taxonomy, u32 action, b32 dragging, r32 tileHeight)
{
    u32 currentTaxonomy = taxonomy;
    
    AssetTypeId result = Asset_standing;
    if(!IsObject(table, taxonomy))
    {
        switch(action)
        {
            case Action_Move:
            {
                result = Asset_moving;
                
                if(dragging)
                {
                    result = Asset_movingDragging;
                }
            } break;
            
            case Action_Attack:
            {
                result = Asset_attacking;
            } break;
            
            case Action_Cast:
            {
                result = Asset_casting;
            } break;
            
            case Action_Eat:
            {
                result = Asset_eating;
            } break;
            
            case Action_Craft:
            {
                result = Asset_attacking;
            } break;
            
            case Action_Rolling:
            {
                result = Asset_rolling;
            } break;
            
            case Action_Protecting:
            {
                result = Asset_protecting;
            } break;
            
            case Action_Drag:
            {
                result = Asset_standing;
            } break;
            
            default:
            {
                result = Asset_standing;
                if(dragging)
                {
                    result = Asset_standingDragging;
                }
            } break;
        }
    }
    else
    {
        InvalidCodePath;
    }
    
    if(tileHeight < SWALLOW_WATER_LEVEL)
    {
        result = Asset_swimming;
    }
    
    return result;
}
