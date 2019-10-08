#include "forg_meta_asset.cpp"

#ifndef ONLY_DATA_FILES
// NOTE(Leonardo): all the functions here can be called just by the main thread!
inline Assets* DEBUGGetGameAssets(PlatformClientMemory* memory)
{
    GameState* gameState = (GameState*) memory->gameState;
    Assets* result = gameState->assets;
    return result;
}
#endif

internal AssetFile* GetAssetFile(Assets* assets, u32 fileIndex)
{
    Assert(fileIndex < assets->fileCount);
    AssetFile* file = assets->files + fileIndex;
    return file;
}

internal PAKFileHeader* GetFileInfo(Assets* assets, u32 fileIndex)
{
    AssetFile* file = GetAssetFile(assets, fileIndex);
    PAKFileHeader* result = &file->header;
    return result;
}

inline PlatformFileHandle* GetHandleFor(Assets* assets, u32 fileIndex)
{
    AssetFile* file = GetAssetFile(assets, fileIndex);
    PlatformFileHandle* result = &file->handle;
    
    return result;
}

internal void CloseAllHandles(Assets* assets)
{
    while(assets->threadsReading){}
    for(u32 fileIndex = 0; fileIndex < assets->fileCount; ++fileIndex)
    {
        AssetFile* file = assets->files + fileIndex;
        platformAPI.CloseFile(&file->handle);
    }
    assets->fileCount = 0;
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

internal AssetMemoryBlock* FreeAsset(Assets* assets, Asset* asset)
{
    Assert(asset->state == Asset_loaded || asset->state == Asset_preloaded || asset->state == Asset_locked);
    DLLIST_REMOVE(asset);
    
    AssetMemoryBlock* result = (AssetMemoryBlock*) ((u8*) asset->data - sizeof(AssetMemoryBlock));
    result->flags &= ~AssetBlock_used;
    
    if(MergeIfPossible(assets, result->prev, result))
    {
        result = result->prev;
    }
    
    MergeIfPossible(assets, result, result->next);
    
    asset->state = Asset_unloaded;
    asset->data = 0;
    
    return result;
}

struct GetAssetResult
{
    Asset* asset;
    b32 derived;
};

internal Asset* GetAsset(AssetSubtypeArray* assets, u16 index)
{
    Asset* result = 0;
    
    u16 runningIndex = index;
    AssetBlock* block = assets->firstAssetBlock;
    while(true)
    {
        if(index < ASSETS_PER_BLOCK)
        {
            break;
        }
        block = block->next;
        index -= ASSETS_PER_BLOCK;
    }
    
    Assert(block);
    result = block->assets + index;
    
    return result;
}

internal AssetBlock* AcquireAssetBlocks(Assets* assets, u16 assetCount)
{
    AssetBlock* result = 0;
    Assert(assetCount > 0);
    u16 blockCount = ((assetCount - 1) / ASSETS_PER_BLOCK) + 1;
    
    for(u16 blockIndex = 0; blockIndex < blockCount; ++blockIndex)
    {
        AssetBlock* block;
        FREELIST_ALLOC(block, assets->firstFreeAssetBlock, PushStruct(assets->pool, AssetBlock));
        block->next = result;
        result = block;
    }
    
    return result;
}



internal AssetSubtypeArray* GetSubtype(AssetArray* array, u32 subtype)
{
    u16 hashIndex = SafeTruncateToU16(subtype >> 16);
    u16 count = SafeTruncateToU16(subtype & 0xffff);
    Assert(hashIndex < ArrayCount(array->subtypes));
    AssetSubtypeArray* result = 0;
    AssetSubtypeArray* first = array->subtypes[hashIndex];
    u16 runningCount = 0;
    while(true)
    {
        if(runningCount++ == count)
        {
            result = first;
            break;
        }
        
        first = first->next;
    }
    
    return result;
}

inline GetAssetResult GetAssetRaw(Assets* assets, AssetID ID)
{
    GetAssetResult result = {};
    
    Assert(ID.type);
    Assert(ID.type < AssetType_Count);
    AssetArray* array = assets->assets + ID.type;
    
    AssetSubtypeArray* subtype = GetSubtype(array, ID.subtypeHashIndex);
    
    if(ID.index < (subtype->standardAssetCount + subtype->derivedAssetCount))
    {
        Asset* asset = GetAsset(subtype, ID.index);
        result.asset = asset;
        result.derived = (ID.index >= subtype->standardAssetCount);
    }
    return result;
}

inline void LockAssetForCurrentFrame(Assets* assets, AssetID ID)
{
    GetAssetResult get = GetAssetRaw(assets, ID);
    Asset* asset = get.asset;
    
    if(asset && asset->state == Asset_loaded)
    {
        asset->state = Asset_locked;
        DLLIST_REMOVE(&asset->LRU);
        DLLIST_INSERT_AS_LAST(&assets->lockedLRUSentinel, &asset->LRU);
    }
}

inline void UnlockLockedAssets(Assets* assets)
{
    for(AssetLRULink* unlock = assets->lockedLRUSentinel.next; unlock != &assets->lockedLRUSentinel;)
    {
        AssetLRULink* next = unlock->next;
        
        Asset* asset = (Asset*) unlock;
        Assert(asset->state == Asset_locked);
        asset->state = Asset_loaded;
        
        DLLIST_REMOVE(unlock);
        DLLIST_INSERT_AS_LAST(&assets->LRUSentinel, unlock);
        
        unlock = next;
    }
    
    Assert(DLLIST_ISEMPTY(&assets->lockedLRUSentinel));
}

internal void* AcquireAssetMemory(Assets* assets, u32 size, Asset* destAsset)
{
    Assert(destAsset->data == 0);
    
    void* result = 0;
    
    AssetMemoryBlock* block = 0;
    for(AssetMemoryBlock* current = assets->blockSentinel.next;
        current != &assets->blockSentinel;
        current = current->next)
    {
        if(!(current->flags & AssetBlock_used))
        {
            if(size <= current->size)
            {
                block = current;
                break;
            }
        }
    }
    
    for(;;)
    {
        if(block && size <= block->size)
        {
            block->flags |= AssetBlock_used;
            
            result = (void*) (block + 1);
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
            for(Asset* asset = assets->assetSentinel.prev;
                asset != &assets->assetSentinel;
                asset = asset->prev)
            {
                if(asset->state == Asset_loaded)
                {
                    block = FreeAsset(assets, asset);
                    break;
                }
            }
        }
    }
    
    Assert(result);
    
    DLLIST_REMOVE(destAsset);
    DLLIST_INSERT(&assets->assetSentinel, destAsset);
    
    return result;
}
struct LoadAssetWork
{
    PlatformFileHandle* handle;
    u32 memorySize;
    void* dest;
    u64 dataOffset;
    
    Asset* asset;
    u32 finalState;
    
    u32 finalizeOperation;
    PlatformTextureOpQueue* textureQueue;
    
    u32* threadsReading;
    
    TaskWithMemory* task;
};


enum FinalizeAssetOperation
{
    Finalize_none,
    Finalize_font,
    Finalize_Bitmap,
};

#ifndef ONLY_DATA_FILES
inline void AddOp(PlatformTextureOpQueue* queue, TextureOp op)
{
    BeginTicketMutex(&queue->mutex);
    Assert(queue->firstFree);
    TextureOp* dest = queue->firstFree;
    queue->firstFree = dest->next;
    *dest = op;
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

internal void LoadAsset(LoadAssetWork* work)
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
                Font* font = &asset->font;
                PAKFont* info = &asset->paka.font;
                
                for(u32 glyphIndex = 0; glyphIndex < info->glyphCount; glyphIndex++)
                {
                    u32 codePoint = font->codepoints[glyphIndex];
                    Assert((u16) glyphIndex == glyphIndex);
                    Assert(codePoint < info->onePastHighestCodePoint);
                    font->unicodeMap[codePoint] = (u16) glyphIndex;
                }
            } break;
            
            case Finalize_Bitmap:
            {
                Asset* asset = work->asset;
                Bitmap* bitmap = &asset->bitmap;
                TextureOp op = {};
                op.update.data = bitmap->pixels;
                op.update.texture = work->asset->textureHandle;
                PlatformTextureOpQueue* queue = work->textureQueue;
                AddOp(queue, op);
            } break;
            
            InvalidCodePath;
        }
        
        CompletePastWritesBeforeFutureWrites;
        work->asset->state = work->finalState;
    }
    
    CompletePastWritesBeforeFutureWrites;
    *work->threadsReading = *work->threadsReading - 1;
}
#else


internal void LoadAsset(LoadAssetWork* work)
{
    platformAPI.ReadFromFile(work->handle, work->dataOffset, work->memorySize, work->dest);
    if(PlatformNoFileErrors(work->handle))
    {
        switch(work->finalizeOperation)
        {
            case Finalize_none:
            {
                
            } break;
            
            InvalidDefaultCase;
        }
        
        CompletePastWritesBeforeFutureWrites;
        work->asset->state = work->finalState;
    }
    
    CompletePastWritesBeforeFutureWrites;
    *work->threadsReading = *work->threadsReading - 1;
}
#endif

PLATFORM_WORK_CALLBACK(LoadAssetThreaded)
{
    TIMED_FUNCTION();
    LoadAssetWork* work = (LoadAssetWork*) param;
    LoadAsset(work);
    
    if(work->task)
    {
        EndTaskWithMemory(work->task);
    }
}

#ifndef ONLY_DATA_FILES
inline u32 AcquireTextureHandle(Assets* assets)
{
    u32 result = 0;
    if(assets->nextFreeTextureHandle <= assets->maxTextureHandleIndex)
    {
        result = assets->nextFreeTextureHandle++;
    }
    else
    {
		b32 free = true;
        AssetLRULink* freeThis = 0;
		if(!DLLIST_ISEMPTY(&assets->LRUFreeSentinel))
		{
            free = false;
			freeThis = assets->LRUFreeSentinel.next;
		}
		else
		{
			Assert(!DLLIST_ISEMPTY(&assets->LRUSentinel));
            freeThis = assets->LRUSentinel.next;
		}
        
		Assert(freeThis);
		DLLIST_REMOVE(freeThis);
		Asset* LRU = (Asset*) freeThis;
		result = LRU->textureHandle.index;
		Clear(&LRU->textureHandle);
        
		if(free && LRU->state == Asset_loaded)
		{
			FreeAsset(assets, LRU);
		}
    }
    
    return result;
}

internal void FreeSpecialTexture(Assets* assets, SpecialTexture* texture)
{
    if(IsValidSpecial(texture))
    {
        texture->textureHandle.width = 0;
        texture->textureHandle.height = 0;
        
        DLLIST_REMOVE(&texture->LRU);
        DLLIST_INSERT_AS_LAST(&assets->specialLRUFreeSentinel, &texture->LRU);
    }
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
        
        AssetLRULink* freeThis = 0;
		if(!DLLIST_ISEMPTY(&assets->specialLRUFreeSentinel))
		{
			freeThis = assets->specialLRUFreeSentinel.next;
		}
		else
		{
			Assert(!DLLIST_ISEMPTY(&assets->specialLRUSentinel));
            freeThis = assets->specialLRUSentinel.next;
		}
        
		Assert(freeThis);
		SpecialTexture* LRU = (SpecialTexture*) freeThis;
        DLLIST_REMOVE(&LRU->LRU);
        
		result = LRU->textureHandle.index;
        Assert(result >= MAX_TEXTURE_COUNT);
        Clear(&LRU->textureHandle);
    }
    
    return result;
}

inline void RefreshSpecialTexture(Assets* assets, SpecialTexture* texture)
{
    DLLIST_REMOVE(&texture->LRU);
    DLLIST_INSERT_AS_LAST(&assets->specialLRUSentinel, &texture->LRU);
}

#endif

internal AssetSubtypeArray* GetAssetSubtypeArray(Assets* assets, u16 type, u32 subtype)
{
    Assert(type < AssetType_Count);
    AssetArray* assetTypeArray = assets->assets + type;
    
    AssetSubtypeArray* result = 0;
    if(subtype != 0xffffffff)
    {
        result = GetSubtype(assetTypeArray, subtype);
    }
    return result;
}

internal u32 GetAssetSubtype(Assets* assets, u16 type, u64 hash)
{
    u32 result = 0xffffffff;
    if(type < AssetType_Count)
    {
        AssetArray* array = assets->assets + type;
        u32 hashIndex = (hash & (ArrayCount(array->subtypes) - 1));
        AssetSubtypeArray* first = array->subtypes[hashIndex];
        u32 index = 0;
        while(first)
        {
            if(first->hash == hash)
            {
                result = (hashIndex << 16) | index;
                break;
            }
            ++index;
        }
    }
    return result;
}

internal u32 GetAssetSubtype(Assets* assets, u16 type, char* subtype)
{
    u32 result = GetAssetSubtype(assets, type, StringHash(subtype));
    return result;
}

internal u32 GetAssetSubtype(Assets* assets, u16 type, Token subtype)
{
    u32 result = GetAssetSubtype(assets, type, StringHash(subtype.text, subtype.textLength));
    return result;
}

internal u32 GetAssetSubtype(Assets* assets, u16 type, Enumerator subtype)
{
    u64 hash = StringHash(subtype.value);
    u32 result = GetAssetSubtype(assets, type, hash);
    return result;
}

internal StringArray GetAssetSubtypeList(Assets* assets, MemoryPool* pool, u16 type)
{
    StringArray result = {};
    
    Assert(type < AssetType_Count);
    AssetArray* array = assets->assets + type;
    
    u32 counter = 0;
    for(u32 hashIndex = 0; hashIndex < ArrayCount(array->subtypes); ++hashIndex)
    {
        AssetSubtypeArray* subtypeArray = array->subtypes[hashIndex];
        while(subtypeArray)
        {
            AssetFile* file = GetAssetFile(assets, subtypeArray->fileIndex);
            if(file->valid)
            {
                ++counter;
            }
            
            subtypeArray = subtypeArray->next;
        }
    }
    
    result.strings = PushArray(pool, char*, counter);
    for(u32 hashIndex = 0; hashIndex < ArrayCount(array->subtypes); ++hashIndex)
    {
        AssetSubtypeArray* subtypeArray = array->subtypes[hashIndex];
        while(subtypeArray)
        {
            AssetFile* file = GetAssetFile(assets, subtypeArray->fileIndex);
            if(file->valid)
            {
                result.strings[result.count++] = PushString(pool, file->header.subtype);
            }
            
            subtypeArray = subtypeArray->next;
        }
    }
    
    
    return result;
}

internal char* GetAssetSubtypeName(Assets* assets, u16 type, u32 subtype)
{
    char* result = 0;
    AssetSubtypeArray* subtypeArray = GetAssetSubtypeArray(assets, type, subtype);
    if(subtypeArray)
    {
        AssetFile* file = GetAssetFile(assets, subtypeArray->fileIndex);
        if(file->valid)
        {
            result = file->header.subtype;
        }
    }
    
    return result;
}

internal char* GetAssetSubtypeName(Assets* assets, u16 type, u64 subtypeHash)
{
    u32 subtype = GetAssetSubtype(assets, type, subtypeHash);
    char* result = GetAssetSubtypeName(assets, type, subtype);
    return result;
}

internal AssetSubtypeArray* GetAssetSubtype(Assets* assets, char* typeString, char* subtypeString)
{
    u16 type = GetMetaAssetType(typeString);
    AssetSubtypeArray* result = 0;
    if(type < AssetType_Count)
    {
        AssetArray* assetTypeArray = assets->assets + type;
        u32 subtypeHashIndex = GetAssetSubtype(assets, type, subtypeString);
        result = GetAssetSubtypeArray(assets, type, subtypeHashIndex);
    }
    return result;
    
}

internal AssetSubtypeArray* AllocateAssetSubtype(Assets* assets, PAKFileHeader* header)
{
    u16 type = GetMetaAssetType(header->type);
    
    AssetArray* array = assets->assets + type;
    
    u16 hashIndex = (StringHash(header->subtype) & (ArrayCount(array->subtypes) - 1));
    AssetSubtypeArray* result = PushStruct(assets->pool, AssetSubtypeArray);
    result->hash = StringHash(header->subtype);
    result->next = array->subtypes[hashIndex];
    array->subtypes[hashIndex] = result;
    
    return result;
}

internal AssetSubtypeArray* GetAssetSubtypeForFile(Assets* assets, PAKFileHeader* header)
{
    AssetSubtypeArray* result = GetAssetSubtype(assets, header->type, header->subtype);
    if(!result)
    {
        result = AllocateAssetSubtype(assets, header);
    }
    
    return result;
}

struct AssetBoilerplate
{
    b32 valid;
    TaskWithMemory* task;
    Asset* asset;
    u32 fileIndex;
    b32 immediate;
};

internal AssetBoilerplate BeginAssetBoilerplate(Assets* assets, AssetID ID, b32 immediate = false)
{
    AssetBoilerplate result = {};
    result.immediate = immediate;
    GetAssetResult get = GetAssetRaw(assets, ID);
    Asset* asset = get.asset;
    
    if(ID.type == AssetType_Image && get.derived)
    {
        ID.index = asset->paka.coloration.bitmapIndex;
        asset = GetAssetRaw(assets, ID).asset;
    }
    
    if(asset)
    {
        if(AtomicCompareExchangeUint32((u32*) &asset->state, Asset_queued, Asset_unloaded) == Asset_unloaded)
        {
            if(!immediate)
            {
                result.task = BeginTaskWithMemory(assets->tasks, assets->taskCount, false);
            }
            
            if(immediate || result.task)
            {
                result.valid = true;
                result.asset = asset;
                asset->data = 0;
                
                AssetSubtypeArray* subtype = GetAssetSubtypeArray(assets, ID.type, ID.subtypeHashIndex);
                result.fileIndex = subtype->fileIndex;
            }
            else
            {
                asset->state = Asset_unloaded;
            }
        }
    }
    
    return result;
}

internal void EndAssetBoilerplate(Assets* assets, AssetBoilerplate boilerplate, u32 size, AssetState state = Asset_loaded, FinalizeAssetOperation finalize = Finalize_none)
{
    TaskWithMemory* task = boilerplate.task;
    Asset* asset = boilerplate.asset;
    
    LoadAssetWork* work = 0;
    
    LoadAssetWork work_ = {};
    if(boilerplate.task)
    {
        work = PushStruct(&task->pool, LoadAssetWork);
    }
    else
    {
        work = &work_;
    }
    
    work->handle = GetHandleFor(assets, boilerplate.fileIndex);
    work->dest = asset->data;
    work->memorySize = size;
    work->dataOffset = asset->paka.dataOffset;
    work->asset = asset;
    work->finalState = state;
    work->finalizeOperation = finalize;
    
    ++assets->threadsReading;
    work->threadsReading = &assets->threadsReading;
    
    work->textureQueue = assets->textureQueue;
    work->task = task;
    
    if(boilerplate.immediate)
    {
        LoadAssetThreaded(work);
    }
    else
    {
        platformAPI.PushWork(assets->loadQueue, LoadAssetThreaded, work);
    }
}

#ifndef ONLY_DATA_FILES
void LoadBitmap(Assets* assets, AssetID ID, b32 immediate = false)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID, immediate);
    if(boilerplate.valid)
    {
        Asset* asset = boilerplate.asset;
        PAKBitmap* info = &asset->paka.bitmap;
        u32 pixelSize = info->dimension[0] * info->dimension[1] * 4;
        u32 attachmentSize = info->attachmentPointCount * sizeof(PAKAttachmentPoint);
        u32 size = pixelSize + attachmentSize;
        
        asset->data = AcquireAssetMemory(assets, size, asset);
        Bitmap* bitmap = &asset->bitmap;
        
        bitmap->width = SafeTruncateToU16(info->dimension[0]);
        bitmap->height = SafeTruncateToU16(info->dimension[1]);
        bitmap->nativeHeight = info->nativeHeight;
        bitmap->widthOverHeight = (r32) bitmap->width / (r32) bitmap->height;
        bitmap->pixels = asset->data;
        bitmap->attachmentPoints = (PAKAttachmentPoint*) AdvanceVoidPtrBytes(asset->data, pixelSize);
        
        u32 textureHandle = AcquireTextureHandle(assets);
        asset->textureHandle = TextureHandle(textureHandle, bitmap->width, bitmap->height);
        bitmap->textureHandle = asset->textureHandle;
        
        EndAssetBoilerplate(assets, boilerplate, size, Asset_loaded, Finalize_Bitmap);
    }
}

internal void LoadFont(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.valid)
    {
        Asset* asset = boilerplate.asset;
        PAKFont* info = &asset->paka.font;
        
        u32 glyphsSize = info->glyphCount * sizeof(u32);
        u32 horizontalAdvanceSize = sizeof(r32) * info->glyphCount * info->glyphCount; 
        u32 unicodeMapSize = sizeof(u16) * info->onePastHighestCodePoint;
        
        u32 size = glyphsSize + horizontalAdvanceSize + unicodeMapSize;
        
        asset->data = AcquireAssetMemory(assets, size, asset);
        Font* font = &asset->font;
        
        font->codepoints = (u32*) (asset->data);
        font->horizontalAdvance = (r32*) ((u8*) font->codepoints + glyphsSize);
        font->unicodeMap = (u16*) ((u8*) font->horizontalAdvance + horizontalAdvanceSize);
        ZeroSize(unicodeMapSize, font->unicodeMap);
        
        EndAssetBoilerplate(assets, boilerplate, size, Asset_loaded, Finalize_font);
    }
}


void LoadSound(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.valid)
    {
        Asset* asset = boilerplate.asset;
        PAKSound* info = &asset->paka.sound;
        
        u32 size = info->channelCount * info->sampleCount * sizeof(i16);
        asset->data = AcquireAssetMemory(assets, size, asset);
        
        Sound* sound = &asset->sound;
        sound->countChannels = info->channelCount;
        sound->countSamples = info->sampleCount;
        Assert(info->channelCount <= ArrayCount(sound->samples));
        
        u8* sampleStart = (u8*) asset->data;
        for(u32 channelIndex = 0; channelIndex < sound->countChannels; channelIndex++)
        {
            sound->samples[channelIndex] = (i16*) sampleStart;
            sampleStart += sizeof(i16) * sound->countSamples;
        }
        
        EndAssetBoilerplate(assets, boilerplate, size);
    }
}

void LoadAnimation(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.valid)
    {
        Asset* asset = boilerplate.asset;
        PAKAnimation* info = &asset->paka.animation;
        
        u32 countBones = info->boneCount;
        u32 countAss = info->assCount;
        u32 frameCount = info->frameCount;
        u32 spriteCount = info->spriteCount;
        
        u32 size =spriteCount * sizeof(SpriteInfo) + frameCount * sizeof(FrameData) + countAss * sizeof(PieceAss) + countBones * sizeof(Bone);
        
        asset->data = AcquireAssetMemory(assets, size, asset);
        u8* base = (u8*) asset->data;
        
        Animation* animation = &asset->animation;
        
        animation->spriteInfoCount = spriteCount;
        animation->frameCount = frameCount;
        
        animation->spriteInfos = (SpriteInfo*) (base);
        animation->frames = (FrameData*) (base + spriteCount * sizeof(SpriteInfo));
        animation->bones = (Bone*) (base + frameCount * sizeof(FrameData) + spriteCount * sizeof(SpriteInfo));
        animation->ass = (PieceAss*) (base + frameCount * sizeof(FrameData) + spriteCount * sizeof(SpriteInfo) + countBones * sizeof(Bone));
        
        EndAssetBoilerplate(assets, boilerplate, size);
    }
}

void LoadModel(Assets* assets, AssetID ID, b32 immediate = false)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID, immediate);
    if(boilerplate.valid)
    {
        Asset* asset = boilerplate.asset;
        PAKModel* info = &asset->paka.model;
        
        u32 vertexCount = info->vertexCount;
        u32 faceCount = info->faceCount;
        
        u32 size = vertexCount * sizeof(ColoredVertex) + faceCount * sizeof(ModelFace);
        
        asset->data = AcquireAssetMemory(assets, size, asset);
        void* memory = asset->data;
        u8* base = (u8*) memory;
        
        VertexModel* model = &asset->model;
        
        model->vertexCount = vertexCount;
        model->faceCount = faceCount;
        
        model->dim = info->dim;
        
        model->vertexes = (ColoredVertex*) base;
        model->faces = (ModelFace*) (base + vertexCount * sizeof(ColoredVertex));
        
        EndAssetBoilerplate(assets, boilerplate, size);
    }
    
}
#endif


void LoadDataFile(Assets* assets, AssetID ID, b32 immediate = false)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID, immediate);
    if(boilerplate.valid)
    {
        Asset* asset = boilerplate.asset;
        u32 size = asset->paka.dataFile.rawSize;
        asset->data = AcquireAssetMemory(assets, size, asset);
        EndAssetBoilerplate(assets, boilerplate, size, Asset_preloaded);
    }
}

internal void ComputeRuntimeProperties(PAKAsset* asset)
{
    for(u32 propertyIndex = 0; propertyIndex < ArrayCount(asset->runtime); ++propertyIndex)
    {
        u64 propertyHash = asset->propertyHash[propertyIndex];
        u64 valueHash = asset->valueHash[propertyIndex];
        
        if(propertyHash && valueHash)
        {
            PAKProperty* dest = asset->runtime + propertyIndex;
            
            for(u16 metaPropertyIndex = 0; metaPropertyIndex < meta_propertyTypeCount; ++metaPropertyIndex)
            {
                MetaPropertyList* list = meta_properties + metaPropertyIndex;
                if(StringHash(list->name) == propertyHash)
                {
                    for(u16 valueIndex = 0; valueIndex < list->propertyCount; ++valueIndex)
                    {
                        char* value = list->properties[valueIndex];
                        if(StringHash(value) == valueHash)
                        {
                            dest->property = metaPropertyIndex;
                            dest->value = valueIndex;
                            break;
                        }
                    }
                    
                    break;
                }
            }
        }
    }
}

internal void LoadAssetFile(Assets* assets, AssetFile* file, AssetSubtypeArray* assetSubtypeArray, MemoryPool* pool)
{
    TempMemory assetMemory = BeginTemporaryMemory(pool);
    
    u16 assetCount = file->header.standardAssetCount + file->header.derivedAssetCount;
    u32 pakAssetArraySize = sizeof(PAKAsset) * assetCount;
    PAKAsset* pakAssetArray = (PAKAsset*) PushSize(pool, pakAssetArraySize, NoClear());
    
    u64 assetOffset = sizeof(PAKFileHeader);
    platformAPI.ReadFromFile(&file->handle, assetOffset, pakAssetArraySize, pakAssetArray);
    
    for(u16 assetIndex = 0; assetIndex < assetCount; assetIndex++)
    {
        Asset* dest = GetAsset(assetSubtypeArray, assetIndex);
        Assert(dest->data == 0);
        dest->paka = pakAssetArray[assetIndex];
        ComputeRuntimeProperties(&dest->paka);
        
        
        dest->state = Asset_unloaded;
        
        u16 assetType = GetMetaAssetType(file->header.type);
        Assert(assetType < AssetType_Count);
        
        if((assetType == AssetType_Skeleton && assetIndex < file->header.standardAssetCount) ||
           (assetType == AssetType_Image && assetIndex >= file->header.standardAssetCount))
        {
            dest->state = Asset_loadedNoData;
        }
    }
    EndTemporaryMemory(assetMemory);
}

internal void InitFileAssetHeader(AssetFile* file)
{
    b32 valid = true;
    ZeroStruct(file->header);
    
    u32 pakVersion;
    platformAPI.ReadFromFile(&file->handle, 0, sizeof(u32), &pakVersion);
    
    if(pakVersion == PAK_VERSION)
    {
        platformAPI.ReadFromFile(&file->handle, 0, sizeof(file->header), &file->header);
        if(!PlatformNoFileErrors(&file->handle))
        {
            valid = false;
        }
        
        PAKFileHeader* header = &file->header;
        if(header->magicValue != PAK_MAGIC_NUMBER)
        {
            valid = false;
            platformAPI.FileError(&file->handle, "magic number not valid");
        }
        
        u32 expectedAssetVersion = MetaGetCurrentVersion(file->header.type);
        if(header->assetVersion != expectedAssetVersion)
        {
            valid = false;
        }
    }
    else
    {
        valid = false;
    }
    file->valid = valid;
}

internal AssetFile* CloseAssetFileFor(Assets* assets, u16 closeType, u64 closeSubtypeHash, u32* fileIndex)
{
    AssetFile* result = 0;
    for(u32 assetFileIndex = 0; assetFileIndex < assets->fileCount; ++assetFileIndex)
    {
        AssetFile* file = GetAssetFile(assets, assetFileIndex);
        u16 type = GetMetaAssetType(file->header.type);
        u64 subtypeHash = StringHash(file->header.subtype);
        
        if(closeType == type && closeSubtypeHash == subtypeHash)
        {
            platformAPI.CloseFile(&file->handle);
            result = file;
            *fileIndex = assetFileIndex;
            
            break;
        }
    }
    
    return result;
}

internal void ReopenReloadAssetFile(Assets* assets, AssetFile* file, u32 fileIndex, u16 typeIn, char* subtypeIn, u8* content, u32 size, MemoryPool* pool)
{
    char* type = GetAssetTypeName(typeIn);
    char* subtype = subtypeIn;
    u64 subtypeHashIn = StringHash(subtype);
    char newName[128];
    FormatString(newName, sizeof(newName), "%s_%s", type, subtype);
    platformAPI.ReplaceFile(PlatformFile_AssetPack, ASSETS_PATH, newName, content, size, 0);
    
    char path[64];
    PlatformFileGroup fake = {};
    fake.path = path;
    FormatString(fake.path, sizeof(path), "%s", ASSETS_PATH);
    
    char name[64];
    PlatformFileInfo fakeInfo = {};
    fakeInfo.name = name;
    FormatString(fakeInfo.name, sizeof(name), "%s.upak", newName);
    
    file->handle = platformAPI.OpenFile(&fake, &fakeInfo);
    InitFileAssetHeader(file);
    if(file->valid)
    {
        AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeForFile(assets, &file->header);
        Assert(assetSubtypeArray);
        u16 assetCount = (assetSubtypeArray->standardAssetCount + assetSubtypeArray->derivedAssetCount);
        for(u16 assetIndex = 0; assetIndex < assetCount; ++assetIndex)
        {
            Asset* asset = GetAsset(assetSubtypeArray, assetIndex);
            if(asset->state == Asset_loaded || asset->state == Asset_preloaded || asset->state == Asset_locked)
            {
#ifndef ONLY_DATA_FILES
                if(IsValid(&asset->textureHandle))
                {
                    DLLIST_REMOVE(&asset->LRU);
                    DLLIST_INSERT_AS_LAST(&assets->LRUFreeSentinel, &asset->LRU);
                }
#endif
                FreeAsset(assets, asset);
            }
        }
        FREELIST_FREE(assetSubtypeArray->firstAssetBlock, AssetBlock, assets->firstFreeAssetBlock);
        
        assetSubtypeArray->standardAssetCount = file->header.standardAssetCount;
        assetSubtypeArray->derivedAssetCount = file->header.derivedAssetCount;
        u16 newAssetCount = assetSubtypeArray->standardAssetCount + assetSubtypeArray->derivedAssetCount;
        
        assetSubtypeArray->firstAssetBlock = AcquireAssetBlocks(assets, newAssetCount);
        assetSubtypeArray->fileIndex = fileIndex;
        LoadAssetFile(assets, file, assetSubtypeArray, pool);
    }
    else
    {
        platformAPI.CloseFile(&file->handle);
    }
}

internal void WriteAssetMarkupDataToStream(Stream* stream, AssetType type, PAKAsset* asset, b32 derivedAsset)
{
    unm rollbackSize = OutputToStream(stream, "\"%s\":", asset->sourceName);
    
    b32 nothingWrote = true;
    for(u32 propertyIndex = 0; propertyIndex < ArrayCount(asset->runtime); ++propertyIndex)
    {
        PAKProperty* property = asset->runtime + propertyIndex;
        if(property->property)
        {
            char* propertyType = GetMetaPropertyTypeName(property->property);
            char* propertyValue = GetMetaPropertyValueName(property->property, property->value);
            
            if(propertyType && propertyValue)
            {
                OutputToStream(stream, "%s=%s;", propertyType, propertyValue);
                nothingWrote = false;
            }
        }
    }
    
    switch(type)
    {
        case AssetType_Image:
        {
            if(!derivedAsset)
            {
                if(asset->bitmap.align[0] != 0.5f)
                {
                    OutputToStream(stream, "%s=%f;", IMAGE_PROPERTY_ALIGN_X, asset->bitmap.align[0]);
                    nothingWrote = false;
                }
                if(asset->bitmap.align[1] != 0.5f)
                {
                    OutputToStream(stream, "%s=%f;", IMAGE_PROPERTY_ALIGN_Y, asset->bitmap.align[1]);
                    nothingWrote = false;
                }
            }
        } break;
        
        case AssetType_Skeleton:
        {
            if(derivedAsset)
            {
                if(asset->animation.syncThreesoldMS != 0)
                {
                    OutputToStream(stream, "%s=%f;", ANIMATION_PROPERTY_SYNC_THREESOLD, asset->animation.syncThreesoldMS);
                    nothingWrote = false;
                }
                
                if(asset->animation.preparationThreesoldMS != 0)
                {
                    OutputToStream(stream, "%s=%f;", ANIMATION_PROPERTY_PREPARATION_THREESOLD, asset->animation.preparationThreesoldMS);
                    nothingWrote = false;
                }
            }
        } break;
    }
    
    
    if(nothingWrote)
    {
        stream->current -= rollbackSize;
        stream->left += (u32) rollbackSize;
    }
}

#ifndef ONLY_DATA_FILES
internal void DumpColorationToStream(PAKColoration* coloration, Stream* stream)
{
    OutputToStream(stream, "\"%s\";", coloration->imageName);
    OutputToStream(stream, "%f;%f;%f;%f;", 
                   coloration->color.r, 
                   coloration->color.g,
                   coloration->color.b,
                   coloration->color.a);
}

internal void DumpPivotToStream(PAKBitmap* bitmap, Stream* stream)
{
    OutputToStream(stream, "%s=%f;%s=%f;", IMAGE_PROPERTY_ALIGN_X, IMAGE_PROPERTY_ALIGN_Y, bitmap->align[0], bitmap->align[1]);
}

internal void WritebackAssetToFileSystem(Assets* assets, AssetID ID, char* basePath, b32 editorMode)
{
    u16 type = ID.type;
    u32 subtype = ID.subtypeHashIndex;
    u16 index = ID.index;
    
    char* assetType = GetAssetTypeName(type);
    char* assetSubtype = GetAssetSubtypeName(assets, type, subtype);
    
    char path[128];
    FormatString(path, sizeof(path), "%s/%s/%s", basePath, assetType, assetSubtype);
    
    AssetSubtypeArray* assetSubtypeArray = GetAssetSubtype(assets, assetType, assetSubtype);
    Assert(assetSubtypeArray);
    MemoryPool tempPool = {};
    
    Stream metaDataStream = PushStream(&tempPool, MegaBytes(1));
    
    Asset* asset = GetAsset(assetSubtypeArray, index);
    b32 derivedAsset = (index >= assetSubtypeArray->standardAssetCount);
    
    WriteAssetMarkupDataToStream(&metaDataStream, (AssetType) type, &asset->paka, derivedAsset);
    TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
    
    u32 replaceFlags = editorMode ? PlatformFileReplace_Hidden : 0; 
    switch(type)
    {
        case AssetType_Font:
        case AssetType_Model:
        case AssetType_Skeleton:
        case AssetType_Sound:
        {
            // NOTE(Leonardo): we can't possibly have edited these!
        } break;
        
        case AssetType_Image:
        {
            if(derivedAsset)
            {
                Stream file = PushStream(&tempPool, KiloBytes(1));
                DumpColorationToStream(&asset->paka.coloration, &file);
                char* filename = asset->paka.sourceName;
                
                char filenameNoExtension_[64];
                char* filenameNoExtension = filenameNoExtension_;
                u32 bufferSize = sizeof(filenameNoExtension_);
                
                if(editorMode)
                {
                    u32 written = (u32) FormatString(filenameNoExtension, bufferSize, TEST_FILE_PREFIX);
                    filenameNoExtension += written;
                    bufferSize -= written;
                }
                
                TrimToFirstCharacter(filenameNoExtension, bufferSize, filename, '.');
                
                
                while(true)
                {
                    b32 replaced = platformAPI.ReplaceFile(PlatformFile_Coloration, path, filenameNoExtension_, file.begin, file.written, replaceFlags);
                    
                    if(!editorMode || replaced)
                    {
                        break;
                    }
                }
            }
        } break;
        
        default:
        {
            Stream file = PushStream(&tempPool, MegaBytes(16));
            char* metaAssetType = metaAsset_assetType[type];
            String structName = {};
            structName.ptr = metaAssetType;
            structName.length = StrLen(metaAssetType);
            
            DumpStructToStream(assets, structName, &file, asset->data);
            
            char filenameNoExtension_[64];
            char* filenameNoExtension = filenameNoExtension_;
            u32 bufferSize = sizeof(filenameNoExtension_);
            
            if(editorMode)
            {
                u32 written = (u32) FormatString(filenameNoExtension, bufferSize, TEST_FILE_PREFIX);
                filenameNoExtension += written;
                bufferSize -= written;
            }
            
            TrimToFirstCharacter(filenameNoExtension, bufferSize,  asset->paka.sourceName, '.');
            
            while(true)
            {
                b32 replaced = platformAPI.ReplaceFile(PlatformFile_data, path, filenameNoExtension_, file.begin, file.written, replaceFlags);
                if(!editorMode || replaced)
                {
                    break;
                }
            }
        } break;
        
    }
    
    EndTemporaryMemory(fileMemory);
    
    char markupFilename_[128];
    char* markupFilename = markupFilename_;
    u32 bufferSize = sizeof(markupFilename_);
    
    if(editorMode)
    {
        u32 written = (u32) FormatString(markupFilename, bufferSize, TEST_FILE_PREFIX);
        markupFilename += written;
        bufferSize -= written;
    }
    
    FormatString(markupFilename, bufferSize, "%s", asset->paka.sourceName);
    ReplaceAll(markupFilename, '.', '_');
    
    platformAPI.ReplaceFile(PlatformFile_markup, path, markupFilename_, metaDataStream.begin, metaDataStream.written, replaceFlags);
    Clear(&tempPool);
}
#endif

internal StringArray BuildAssetIndexNames(Assets* assets, MemoryPool* pool, u16 type, u32 subtypeHashIndex)
{
    StringArray result = {};
    AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeArray(assets, type, subtypeHashIndex);
    if(assetSubtypeArray)
    {
        u16 assetCount = assetSubtypeArray->standardAssetCount + assetSubtypeArray->derivedAssetCount;
        
        result.strings = PushArray(pool, char*, assetCount);
        for(u16 assetIndex = 0; assetIndex < assetCount; ++assetIndex)
        {
            Asset* asset = GetAsset(assetSubtypeArray, assetIndex);
            result.strings[result.count++] = PushString(pool, asset->paka.sourceName);
        }
    }
    
    return result;
}

internal u16 GetAssetIndex(Assets* assets, u16 type, u32 subtype, Token indexName)
{
    u16 result = 0;
    AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeArray(assets, type, subtype);
    if(assetSubtypeArray)
    {
        u16 assetCount = assetSubtypeArray->standardAssetCount;
        for(u16 assetIndex = 0; assetIndex < assetCount; ++assetIndex)
        {
            Asset* asset = GetAsset(assetSubtypeArray, assetIndex);
            if(StrEqual(asset->paka.sourceName, StrLen(asset->paka.sourceName), indexName.text, indexName.textLength))
            {
                result = assetIndex;
                break;
            }
        }
    }
    return result;
}

internal char* GetAssetIndexName(Assets* assets, AssetID ID)
{
    Asset* asset = GetAssetRaw(assets, ID).asset;
    char* result = asset->paka.sourceName;
    return result;
}






internal Assets* InitAssets(PlatformWorkQueue* loadQueue, TaskWithMemory* tasks, u32 taskCount, MemoryPool* pool, PlatformTextureOpQueue* textureQueue, memory_index size)
{
    Assets* assets = PushStruct(pool, Assets); 
    
    DLLIST_INIT(&assets->LRUSentinel);
    DLLIST_INIT(&assets->LRUFreeSentinel);
    DLLIST_INIT(&assets->specialLRUSentinel);
    DLLIST_INIT(&assets->specialLRUFreeSentinel);
    DLLIST_INIT(&assets->lockedLRUSentinel);
    
    assets->blockSentinel.prev = &assets->blockSentinel;
    assets->blockSentinel.next = &assets->blockSentinel;
    assets->blockSentinel.flags = 0;
    assets->blockSentinel.size = 0;
    
    InsertBlock(&assets->blockSentinel, size, (AssetMemoryBlock*)PushSize(pool, size, NoClear()));
    
#ifndef ONLY_DATA_FILES
    assets->textureQueue = textureQueue;
    assets->whitePixel = 0xFFFFFFFF;
    TextureOp op = {};
    op.update.data = &assets->whitePixel;
    op.update.texture = TextureHandle(0, 1, 1);
    AddOp(assets->textureQueue, op);
    
    assets->nextFreeTextureHandle = 1;
    assets->maxTextureHandleIndex = MAX_TEXTURE_COUNT - 1;
    
    assets->nextFreeSpecialTextureHandle = MAX_TEXTURE_COUNT;
    assets->maxSpecialTextureHandleIndex = MAX_TEXTURE_COUNT + MAX_SPECIAL_TEXTURE_COUNT - 1;
#endif
    
    assets->assetSentinel.next = &assets->assetSentinel;
    assets->assetSentinel.prev = &assets->assetSentinel;
    
    assets->loadQueue = loadQueue;
    assets->tasks = tasks;
    assets->taskCount = taskCount;
    
    assets->firstFreeAssetBlock = 0;
    assets->pool = pool;
    
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(PlatformFile_AssetPack, ASSETS_PATH);
    
    assets->maxFileCount = Max(1024, fileGroup.fileCount);
    assets->fileCount = fileGroup.fileCount;
    assets->files = PushArray(pool, AssetFile, assets->maxFileCount);
    
    u32 fileIndex = 0;
    for(PlatformFileInfo* info = fileGroup.firstFileInfo; info; info = info->next)
    {
        AssetFile* file = assets->files + fileIndex;
        file->handle = platformAPI.OpenFile(&fileGroup, info);
        file->size = SafeTruncateUInt64ToU32(info->size);
        PAKFileHeader* header = &file->header;
        InitFileAssetHeader(file);
        if(file->valid)
        {
            u16 assetCount = (header->standardAssetCount + header->derivedAssetCount);
            AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeForFile(assets, header);
            if(assetSubtypeArray)
            {
                assetSubtypeArray->standardAssetCount = header->standardAssetCount;
                assetSubtypeArray->derivedAssetCount = header->derivedAssetCount;
                assetSubtypeArray->firstAssetBlock = AcquireAssetBlocks(assets, assetCount);
                assetSubtypeArray->fileIndex = fileIndex;
                LoadAssetFile(assets, file, assetSubtypeArray, pool);
            }
            else
            {
                // TODO(Leonardo): delete the file!
            }
        }
        else
        {
            platformAPI.CloseFile(&file->handle);
            // TODO(Leonardo): notify user! so that he can download the file again.
        }
        
        ++fileIndex;
    }
    platformAPI.GetAllFilesEnd(&fileGroup);
    
    return assets;
}

inline b32 MatchesProperties(Asset* asset, GameProperties* properties, b32* exactMatch)
{
    b32 result = true;
    b32 exactMatching = true;
    
    if(properties)
    {
        for(u32 propertyIndex = 0; propertyIndex < ArrayCount(properties->properties); ++propertyIndex)
        {
            PAKProperty* property = properties->properties + propertyIndex;
            u32 flags = properties->flags[propertyIndex];
            
            if(property->property)
            {
                b32 hasProperty = false;
                b32 propertyMatches = false;
                
                for(u32 assetPropertyIndex = 0; assetPropertyIndex < ArrayCount(asset->paka.runtime); ++assetPropertyIndex)
                {
                    PAKProperty* assetProperty = asset->paka.runtime + assetPropertyIndex;
                    if(assetProperty->property == property->property)
                    {
                        hasProperty = true;
                        if(property->value == assetProperty->value)
                        {
                            propertyMatches = true;
                        }
                        break;
                    }
                }
                
                if(!hasProperty)
                {
                    if(flags & GameProperty_Optional)
                    {
                        exactMatching = false;
                    }
                    else
                    {
                        result = false;
                        break;
                    }
                }
                else
                {
                    if(!propertyMatches)
                    {
                        result = false;
                        break;
                    }
                }
            }
        }
    }
    *exactMatch = exactMatching;
    return result;
}

#ifndef ONLY_DATA_FILES
#define QueryBitmaps(assets, subtype, seq, properties) QueryAssets_(assets, AssetType_Image, subtype, seq, properties, true)

#define QuerySkeletons(assets, subtype, seq, properties) QueryAssets_(assets, AssetType_Skeleton, subtype, seq, properties)
#define QueryFonts(assets, subtype, seq, properties) QueryAssets_(assets, AssetType_Font, GetAssetSubtype(assets, AssetType_Font, subtype), seq, properties)
#define QueryModels(assets, subtype, seq, properties) QueryAssets_(assets, AssetType_Model, subtype, seq, properties)
#endif

#define QueryDataFiles(assets, type, sub, seq, properties) QueryAssets_(assets, AssetType_##type, GetAssetSubtype(assets, AssetType_##type, sub), seq, properties)



#define AddGameProperty(properties, property, value) AddGameProperty_(properties, Property_##property, value, 0)
#define AddGamePropertyRaw(properties, property) AddGameProperty_(properties, property, 0)
#define AddOptionalGameProperty(properties, property, value) AddGameProperty_(properties, Property_##property, value, GameProperty_Optional)
#define AddOptionalGamePropertyRaw(properties, property) AddGameProperty_(properties, property, GameProperty_Optional)

inline void AddGameProperty_(GameProperties* properties, GameProperty property, u32 flags)
{
    for(u32 propertyIndex = 0; propertyIndex < ArrayCount(properties->properties); ++propertyIndex)
    {
        GameProperty* prop = properties->properties + propertyIndex;
        if(!prop->property)
        {
            *prop = property;
            properties->flags[propertyIndex] = flags;
            break;
        }
    }
}

inline void AddGameProperty_(GameProperties* properties, PropertyType property, u32 value, u32 flags)
{
    GameProperty prop = {};
    prop.property = SafeTruncateToU16(property);
    prop.value = SafeTruncateToU16(value);
    
    AddGameProperty_(properties, prop, flags);
}

inline void Clear(GameProperties* properties)
{
    for(u32 propertyIndex = 0; propertyIndex < ArrayCount(properties->properties); ++propertyIndex)
    {
        properties->properties[propertyIndex].property = 0;
    }
}

internal AssetID QueryAssets_(Assets* assets, AssetType type, u32 subtype, RandomSequence* seq, GameProperties* properties, b32 derivedAssets = false, u16 startingIndex = 0, u16 onePastEndingIndex = 0)
{
    AssetID result = {};
    if(type)
    {
        AssetArray* array = assets->assets + type;
        
        AssetSubtypeArray* subtypeArray = GetSubtype(array, subtype);
        if(subtypeArray->standardAssetCount > 0)
        {
            u16 matchingOptional = 0;
            u16 matchingAssetsOptional[32];
            
            u16 matchingExact = 0;
            u16 matchingAssetsExact[32];
            
            u16 maximumAssetIndexAllowed = subtypeArray->standardAssetCount;
            if(derivedAssets)
            {
                maximumAssetIndexAllowed += subtypeArray->derivedAssetCount;
            }
            
            if(!onePastEndingIndex)
            {
                onePastEndingIndex = maximumAssetIndexAllowed;
            }
            onePastEndingIndex = Min(onePastEndingIndex, maximumAssetIndexAllowed);
            
            Assert(startingIndex <= onePastEndingIndex);
            for(u16 assetIndex = startingIndex; assetIndex < onePastEndingIndex; ++assetIndex)
            {
                Asset* asset = GetAsset(subtypeArray, assetIndex);
                
                b32 exactMatch;
                if(MatchesProperties(asset, properties, &exactMatch))
                {
                    if(exactMatch)
                    {
                        if(matchingExact < ArrayCount(matchingAssetsExact))
                        {
                            matchingAssetsExact[matchingExact++] = assetIndex;
                        }
                    }
                    else
                    {
                        if(matchingOptional < ArrayCount(matchingAssetsOptional))
                        {
                            matchingAssetsOptional[matchingOptional++] = assetIndex;
                        }
                    }
                }
            }
            
            if(matchingExact > 0 || matchingOptional > 0)
            {
                result.type = SafeTruncateToU16(type);
                result.subtypeHashIndex = subtype;
                
                if(matchingExact > 0)
                {
                    u32 choice = RandomChoice(seq, matchingExact);
                    result.index = matchingAssetsExact[choice];
                }
                else
                {
                    u32 choice = RandomChoice(seq, matchingOptional);
                    result.index = matchingAssetsOptional[choice];
                }
            }
        }
    }
    
    return result;
}


inline void LoadAssetDataStructure(Assets* assets, Asset* asset, AssetID ID)
{
    MemoryPool tempPool = {};
    
    Buffer sourceBuffer;
    sourceBuffer.ptr = (char*) asset->data;
    sourceBuffer.length = asset->paka.dataFile.rawSize;
    
    Buffer tempBuffer = CopyBuffer(&tempPool, sourceBuffer);
    
    Tokenizer tokenizer = {};;
    tokenizer.at = (char*) tempBuffer.ptr;
    
    char* assetType = metaAsset_assetType[ID.type];
    String structName = {};
    structName.ptr = assetType;
    structName.length = StrLen(assetType);
    u32 finalSize = GetStructSize(structName, &tokenizer);
    
    if(finalSize > 0)
    {
        FreeAsset(assets, asset);
        asset->data = AcquireAssetMemory(assets, finalSize, asset);
        tokenizer.at = (char*) tempBuffer.ptr;
        ParseBufferIntoStruct(assets, structName, &tokenizer, asset->data, finalSize);
        asset->state = Asset_loaded;
    }
    else
    {
        InvalidCodePath;
    }
    
    Clear(&tempPool);
}

#ifndef ONLY_DATA_FILES
#define PreloadAll(assets, type, subtype, immediate) PreloadAll_(assets, type, GetAssetSubtype(assets, type, subtype), immediate)
internal void PreloadAll_(Assets* assets, u16 type, u32 subtype, b32 immediate)
{
    AssetSubtypeArray* assetArray = GetAssetSubtypeArray(assets, type, subtype);
    Assert(assetArray);
    BitmapId ID = {};
    ID.type = type;
    ID.subtypeHashIndex = subtype;
    for(u16 assetIndex = 0; assetIndex < (assetArray->standardAssetCount + assetArray->derivedAssetCount); ++assetIndex)
    {
        ID.index = assetIndex;
        
        switch(type)
        {
            case AssetType_Image:
            {
                LoadBitmap(assets, ID, immediate);
            } break;
            
            InvalidDefaultCase;
        }
    }
}

internal AssetID* GetAllSkinBitmaps(MemoryPool* tempPool, Assets* assets, u32 skin, GameProperties* skinProperties, u32* bitmapCount)
{
    AssetID* result = 0;
    *bitmapCount = 0;
    
    AssetArray* array = assets->assets + AssetType_Image;
    AssetSubtypeArray* skinBitmaps = GetSubtype(array, skin);
    
    u16 totalAssetCount = skinBitmaps->standardAssetCount + skinBitmaps->derivedAssetCount;
    *bitmapCount = totalAssetCount;
    
    result = PushArray(tempPool, AssetID, totalAssetCount);
    for(u16 assetIndex = 0; assetIndex < totalAssetCount; ++assetIndex)
    {
        AssetID* dest = result + assetIndex;
        dest->type = AssetType_Image;
        dest->subtypeHashIndex = skin;
        dest->index = assetIndex;
    }
    
    return result;
}

#define GetAllSkinBitmaps(pool, assets, skin, properties, count) GetAllAssets_(pool, assets, SafeTruncateToU16(AssetType_Image), skin, properties, count)


inline void PreloadAllGroundBitmaps(Assets* assets)
{
    PreloadAll(assets, AssetType_Image, "default", true);
}
#endif

#define GetAllDataAsset(pool, assets, type, sub, properties, count) GetAllAssets_(pool, assets, SafeTruncateToU16(AssetType_##type), GetAssetSubtype(assets, AssetType_##type, StringHash(sub)), properties, count)

internal AssetID* GetAllAssets_(MemoryPool* tempPool, Assets* assets, u16 assetType, u32 subtype, GameProperties* properties, u16* count)
{
    AssetID* result = 0;
    *count = 0;
    
    AssetArray* array = assets->assets + assetType;
    AssetSubtypeArray* assetArray = GetSubtype(array, subtype);
    u16 totalAssetCount = assetArray->standardAssetCount + assetArray->derivedAssetCount;
    *count = totalAssetCount;
    
    result = PushArray(tempPool, AssetID, totalAssetCount);
    for(u16 assetIndex = 0; assetIndex < totalAssetCount; ++assetIndex)
    {
        AssetID* dest = result + assetIndex;
        dest->type = assetType;
        dest->subtypeHashIndex = subtype;
        dest->index = assetIndex;
    }
    
    return result;
}

struct GetGameAssetResult
{
    Asset* asset;
    PAKAsset* info;
    b32 derived;
};

inline GetGameAssetResult GetGameAsset(Assets* assets, AssetID ID)
{
    GetGameAssetResult result = {};
    GetAssetResult get = GetAssetRaw(assets, ID);
    Asset* asset = get.asset;
    if(asset)
    {
        result.info = &asset->paka;
        result.derived = get.derived;
        
        if(asset->state == Asset_preloaded)
        {
            LoadAssetDataStructure(assets, asset, ID);
            asset->state = Asset_loaded;
        }
        
        if(asset->state == Asset_loaded || asset->state == Asset_locked || asset->state == Asset_loadedNoData)
        {
            result.asset = asset;
            
#ifndef ONLY_DATA_FILES
            if(IsValid(&asset->textureHandle))
            {
                DLLIST_REMOVE(&asset->LRU);
                DLLIST_INSERT_AS_LAST(&assets->LRUSentinel, &asset->LRU);
            }
#endif
            
            DLLIST_REMOVE(asset);
            DLLIST_INSERT(&assets->assetSentinel, asset);
            CompletePastWritesBeforeFutureWrites;
        }
    }
    
    return result;
}

#ifndef ONLY_DATA_FILES
struct ColoredBitmap
{
    Vec2 pivot;
    Bitmap* bitmap;
    Vec4 coloration;
};

inline ColoredBitmap GetBitmap(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    ColoredBitmap result = {};
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    if(ID.type == AssetType_Image && get.derived)
    {
        AssetID parentID = ID;
        parentID.index = get.info->coloration.bitmapIndex;
        GetGameAssetResult getParent = GetGameAsset(assets, parentID);
        result.bitmap = getParent.asset ? &getParent.asset->bitmap : 0;
        result.pivot = V2(getParent.info->bitmap.align[0], getParent.info->bitmap.align[1]);
        result.coloration = get.info->coloration.color;
    }
    else
    {
        result.bitmap = get.asset ? &get.asset->bitmap : 0;
        result.pivot = V2(get.info->bitmap.align[0], get.info->bitmap.align[1]);
        result.coloration = V4(1, 1, 1, 1);
    }
    
    return result;
}

internal PAKAttachmentPoint* GetAttachmentPoint(Assets* assets, AssetID ID, u32 attachmentPointIndex)
{
    PAKAttachmentPoint* result = 0;
    ColoredBitmap bitmap = GetBitmap(assets, ID);
    if(bitmap.bitmap)
    {
        result = bitmap.bitmap->attachmentPoints + attachmentPointIndex;
    }
    
    return result;
}

inline Font* GetFont(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    Font* result = get.asset ? &get.asset->font : 0;
    
    return result;
}

inline Sound* GetSound(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    Sound* result = get.asset ? &get.asset->sound : 0;
    
    return result;
}

inline Skeleton* GetSkeleton(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    Skeleton* result = get.asset ? &get.asset->skeleton : 0;
    
    return result;
}

inline Animation* GetAnimation(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    Animation* result = get.asset ? &get.asset->animation : 0;
    
    return result;
}

inline VertexModel* GetModel(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    VertexModel* result = get.asset ? &get.asset->model : 0;
    
    return result;
}
#endif

#define GetData(assets, type, ID) (type*)GetDataDefinition_(assets, AssetType_##type, ID)
inline void* GetDataDefinition_(Assets* assets, AssetType type, AssetID ID, b32 immediate = true)
{
    Assert(type == ID.type);
    Assert(IsValid(ID));
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    void* result = get.asset ? get.asset->data : 0;
    
    if(!result && immediate)
    {
        if(get.info)
        {
            LoadDataFile(assets, ID, immediate);
            get = GetGameAsset(assets, ID);
            Assert(get.asset);
            result = get.asset->data;
        }
    }
    
    return result;
}

inline PAKAsset* GetPakAsset(Assets* assets, AssetID ID)
{
    PAKAsset* result = 0;
    GetGameAssetResult get = GetGameAsset(assets, ID);
    result = get.info;
    
    return result;
}

#ifndef ONLY_DATA_FILES
inline PAKBitmap* GetBitmapInfo(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    PAKAsset* asset = GetPakAsset(assets, ID);
    PAKBitmap* result = &asset->bitmap;
    return result;
}

inline PAKFont* GetFontInfo(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    PAKAsset* asset = GetPakAsset(assets, ID);
    PAKFont* result = &asset->font;
    return result;
}

inline PAKSound* GetSoundInfo(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    PAKAsset* asset = GetPakAsset(assets, ID);
    PAKSound* result = &asset->sound;
    return result;
}

inline PAKSkeleton* GetSkeletonInfo(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    PAKAsset* asset = GetPakAsset(assets, ID);
    PAKSkeleton* result = &asset->skeleton;
    return result;
}

inline PAKAnimation* GetAnimationInfo(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    PAKAsset* asset = GetPakAsset(assets, ID);
    PAKAnimation* result = &asset->animation;
    return result;
}

inline PAKModel* GetModelInfo(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    PAKAsset* asset = GetPakAsset(assets, ID);
    PAKModel* result = &asset->model;
    return result;
}

inline u32 GetGlyphIndexForCodePoint(Font* font, PAKFont* info, u32 desired)
{
    u32 result = 0;
    if(desired < info->onePastHighestCodePoint)
    {
        result = font->unicodeMap[desired];
        Assert(result < info->glyphCount);
    }
    return result;
}

internal r32 GetHorizontalAdvanceForPair(Font* font, PAKFont* info, u32 desiredPrevCodePoint, u32 desiredCodePoint)
{
    u32 glyph = GetGlyphIndexForCodePoint(font, info, desiredCodePoint);
    u32 prevGlyph = GetGlyphIndexForCodePoint(font, info, desiredPrevCodePoint);
    r32 result = font->horizontalAdvance[prevGlyph * info->glyphCount + glyph];
    return result;
}

internal r32 GetLineAdvance(PAKFont* info)
{
    r32 result = info->externalLeading + info->ascenderHeight + info->descenderHeight;
    return result;
}

internal r32 GetStartingLineY(PAKFont* info)
{
    r32 result = info->ascenderHeight;
    return result;
}


inline AssetID GetBitmapForGlyph(Assets* assets, AssetID fontID, u32 desiredCodePoint)
{
    AssetID result = {};
    
    Font* font = GetFont(assets, fontID);
    if(font)
    {
        PAKFont* info = GetFontInfo(assets, fontID);
        Asset* asset = GetAssetRaw(assets, fontID).asset;
        
        u16 index = SafeTruncateToU16(GetGlyphIndexForCodePoint(font, info, desiredCodePoint));
        Assert(index);
        
        result.type = fontID.type;
        result.subtypeHashIndex = fontID.subtypeHashIndex;
        result.index = info->glyphAssetsFirstIndex + index - 1;
    }
    
    return result;
    
}

inline AssetID GetMatchingAnimationForSkeleton(Assets* assets, AssetID skeletonID, RandomSequence* seq, GameProperties* properties)
{
    AssetID result = {};
    Asset* asset = GetAssetRaw(assets, skeletonID).asset;
    if(asset)
    {
        PAKSkeleton* info = GetSkeletonInfo(assets, skeletonID);
        if(info->animationCount > 0)
        {
            u16 startingIndex = info->animationAssetsFirstIndex;
            u16 endingIndex = startingIndex + info->animationCount;
            
            result = QueryAssets_(assets, (AssetType) skeletonID.type, skeletonID.subtypeHashIndex, seq, properties, true, startingIndex, endingIndex);
            
        }
    }
    
    return result;
}

internal AssetID QueryAnimations(Assets* assets, u64 skeletonHash, RandomSequence* seq, GameProperties* properties)
{
    u32 skeleton = GetAssetSubtype(assets, AssetType_Skeleton, skeletonHash);
    AssetID result = {};
    AssetID skeletonID = QuerySkeletons(assets, skeleton, seq, properties);
    if(IsValid(skeletonID))
    {
        result = GetMatchingAnimationForSkeleton(assets, skeletonID, seq, properties);
    }
    
    return result;
}
#endif