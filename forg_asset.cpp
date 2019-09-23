#include "forg_meta_asset.cpp"
// NOTE(Leonardo): all the functions here can be called just by the main thread!
inline Assets* DEBUGGetGameAssets(PlatformClientMemory* memory)
{
    GameState* gameState = (GameState*) memory->gameState;
    Assets* result = gameState->assets;
    return result;
}

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
        FREELIST_ALLOC(block, assets->firstFreeAssetBlock, PushStruct(assets->blockPool, AssetBlock));
        block->next = result;
        result = block;
    }
    
    return result;
}



inline GetAssetResult GetAssetRaw(Assets* assets, AssetID ID)
{
    GetAssetResult result = {};
    
    Assert(ID.type);
    Assert(ID.type < AssetType_Count);
    AssetArray* array = assets->assets + ID.type;
    
    Assert(ID.subtype < array->subtypeCount);
    AssetSubtypeArray* subtype = array->subtypes + ID.subtype;
    
    Assert(ID.index < (subtype->standardAssetCount + subtype->derivedAssetCount));
    Asset* asset = GetAsset(subtype, ID.index);
    
    result.asset = asset;
    result.derived = (ID.index >= subtype->standardAssetCount);
    
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

PLATFORM_WORK_CALLBACK(LoadAssetThreaded)
{
    TIMED_FUNCTION();
    LoadAssetWork* work = (LoadAssetWork*) param;
    LoadAsset(work);
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

internal AssetSubtypeArray* GetAssetSubtype(Assets* assets, u16 type, u16 subtype)
{
    Assert(type < AssetType_Count);
    AssetArray* assetTypeArray = assets->assets + type;
    Assert(subtype < assetTypeArray->subtypeCount);
    AssetSubtypeArray* result = assetTypeArray->subtypes + subtype;
    return result;
}


internal AssetSubtypeArray* GetAssetSubtype(Assets* assets, char* typeString, char* subtypeString)
{
    u16 type = GetMetaAssetType(typeString);
    AssetSubtypeArray* result = 0;
    if(type < AssetType_Count)
    {
        AssetArray* assetTypeArray = assets->assets + type;
        u16 subtype = GetMetaAssetSubtype(type, subtypeString);
        if(subtype < assetTypeArray->subtypeCount)
        {
            result = GetAssetSubtype(assets, type, subtype);
            
        }
    }
    return result;
    
}

internal AssetSubtypeArray* GetAssetSubtypeForFile(Assets* assets, PAKFileHeader* header)
{
    AssetSubtypeArray* result = GetAssetSubtype(assets, header->type, header->subtype);
    return result;
}

struct AssetBoilerplate
{
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
            result.task = BeginTaskWithMemory(assets->tasks, assets->taskCount, false);
            if(result.task)
            {
                result.asset = asset;
                asset->data = 0;
                
                AssetSubtypeArray* subtype = GetAssetSubtype(assets, ID.type, ID.subtype);
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
    
    LoadAssetWork* work = PushStruct(&task->pool, LoadAssetWork);
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


void LoadBitmap(Assets* assets, AssetID ID, b32 immediate = false)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID, immediate);
    if(boilerplate.task)
    {
        Asset* asset = boilerplate.asset;
        PAKBitmap* info = &asset->paka.bitmap;
        u32 size = info->dimension[0] * info->dimension[1] * 4;
        
        asset->data = AcquireAssetMemory(assets, size, asset);
        Bitmap* bitmap = &asset->bitmap;
        
        bitmap->width = SafeTruncateToU16(info->dimension[0]);
        bitmap->height = SafeTruncateToU16(info->dimension[1]);
        bitmap->nativeHeight = info->nativeHeight;
        bitmap->pivot = V2(info->align[0], info->align[1]);
        bitmap->widthOverHeight = (r32) bitmap->width / (r32) bitmap->height;
        bitmap->pixels = asset->data;
        
        u32 textureHandle = AcquireTextureHandle(assets);
        asset->textureHandle = TextureHandle(textureHandle, bitmap->width, bitmap->height);
        bitmap->textureHandle = asset->textureHandle;
        
        EndAssetBoilerplate(assets, boilerplate, size, Asset_loaded, Finalize_Bitmap);
    }
}

internal void LoadFont(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.task)
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
    if(boilerplate.task)
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
    if(boilerplate.task)
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

void LoadModel(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.task)
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

void LoadDataFile(Assets* assets, AssetID ID, b32 immediate = false)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID, immediate);
    if(boilerplate.task)
    {
        Asset* asset = boilerplate.asset;
        u32 size = asset->paka.dataFile.rawSize;
        asset->data = AcquireAssetMemory(assets, size, asset);
        EndAssetBoilerplate(assets, boilerplate, size, Asset_preloaded);
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

internal b32 InitFileAssetHeader(AssetFile* file)
{
    b32 result = false;
    ZeroStruct(file->header);
    platformAPI.ReadFromFile(&file->handle, 0, sizeof(file->header), &file->header);
    if(PlatformNoFileErrors(&file->handle))
    {
        result = true;
    }
    PAKFileHeader* header = &file->header;
    if(header->magicValue != PAK_MAGIC_NUMBER)
    {
        result = false;
        platformAPI.FileError(&file->handle, "magic number not valid");
    }
    
    if(header->version != PAK_VERSION)
    {
        result = false;
        platformAPI.FileError(&file->handle, "pak file version not valid");
    }
    
    return result;
}

internal void ReloadAssetFile(Assets* assets, AssetFile* file, u32 fileIndex, MemoryPool* pool)
{
    if(InitFileAssetHeader(file))
    {
        AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeForFile(assets, &file->header);
        Assert(assetSubtypeArray);
        u16 assetCount = (assetSubtypeArray->standardAssetCount + assetSubtypeArray->derivedAssetCount);
        for(u16 assetIndex = 0; assetIndex < assetCount; ++assetIndex)
        {
            Asset* asset = GetAsset(assetSubtypeArray, assetIndex);
            if(asset->state == Asset_loaded || asset->state == Asset_preloaded || asset->state == Asset_locked)
            {
				if(IsValid(&asset->textureHandle))
				{
                    DLLIST_REMOVE(&asset->LRU);
					DLLIST_INSERT_AS_LAST(&assets->LRUFreeSentinel, &asset->LRU);
				}
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
}

internal void WriteAssetMarkupDataToStream(Stream* stream, AssetType type, PAKAsset* asset, b32 derivedAsset)
{
    unm rollbackSize = OutputToStream(stream, "\"%s\":", asset->sourceName);
    
    b32 nothingWrote = true;
    for(u32 propertyIndex = 0; propertyIndex < ArrayCount(asset->properties); ++propertyIndex)
    {
        PAKProperty* property = asset->properties + propertyIndex;
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
                if(asset->bitmap.align[0] != 0)
                {
                    OutputToStream(stream, "%s=%f;", IMAGE_PROPERTY_ALIGN_X, asset->bitmap.align[0]);
                    nothingWrote = false;
                }
                if(asset->bitmap.align[1] != 0)
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

internal void DumpColorationToStream(PAKColoration* coloration, Stream* stream)
{
    OutputToStream(stream, "\"%s\";", coloration->imageName);
    OutputToStream(stream, "%f;%f;%f;%f;", 
                   coloration->color.r, 
                   coloration->color.g,
                   coloration->color.b,
                   coloration->color.a);
}

internal void WritebackAssetToFileSystem(Assets* assets, AssetID ID, char* basePath, b32 editorMode)
{
    u16 type = ID.type;
    u16 subtype = ID.subtype;
    u16 index = ID.index;
    
    char* assetType = GetAssetTypeName(type);
    char* assetSubtype = GetAssetSubtypeName(type, subtype);
    
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
            
            DumpStructToStream(structName, &file, asset->data);
            
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
    
    assets->assetSentinel.next = &assets->assetSentinel;
    assets->assetSentinel.prev = &assets->assetSentinel;
    
    assets->loadQueue = loadQueue;
    assets->tasks = tasks;
    assets->taskCount = taskCount;
    
    assets->firstFreeAssetBlock = 0;
    assets->blockPool = pool;
    
    for(u32 assetTypeIndex = 0; assetTypeIndex < AssetType_Count; ++assetTypeIndex)
    {
        MetaAssetType metaType = metaAsset_subTypes[assetTypeIndex];
        AssetArray* assetArray = assets->assets + assetTypeIndex;
        assetArray->subtypeCount = metaType.subtypeCount;
        assetArray->subtypes = PushArray(pool, AssetSubtypeArray, assetArray->subtypeCount);
    }
    
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
        if(InitFileAssetHeader(file))
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
            // TODO(Leonardo): notify user! so that he can download the file again.
            InvalidCodePath;
        }
        
        ++fileIndex;
    }
    platformAPI.GetAllFilesEnd(&fileGroup);
    
    return assets;
}

inline b32 MatchesProperties(Asset* asset, GameProperties* properties)
{
    b32 result = true;
    
    if(properties)
    {
        for(u32 propertyIndex = 0; propertyIndex < ArrayCount(properties->properties); ++propertyIndex)
        {
            PAKProperty* property = properties->properties + propertyIndex;
            if(property->property)
            {
                b32 hasProperty = false;
                for(u32 assetPropertyIndex = 0; assetPropertyIndex < ArrayCount(asset->paka.properties); ++assetPropertyIndex)
                {
                    PAKProperty* assetProperty = asset->paka.properties + assetPropertyIndex;
                    if(assetProperty->property == property->property)
                    {
                        if(property->value == assetProperty->value)
                        {
                            hasProperty = true;
                        }
                        break;
                    }
                }
                if(!hasProperty)
                {
                    result = false;
                    break;
                }
            }
        }
    }
    
    return result;
}

#define QueryBitmaps(assets, subtype, seq, properties) QueryAssets_(assets, AssetType_Image, subtype, seq, properties, true)

#define QuerySkeletons(assets, subtype, seq, properties) QueryAssets_(assets, AssetType_Skeleton, subtype, seq, properties)
#define QueryFonts(assets, subtype, seq, properties) QueryAssets_(assets, AssetType_Font, subtype, seq, properties)
#define QueryDataFiles(assets, type, sub, seq, properties) QueryAssets_(assets, AssetType_##type, sub, seq, properties)

internal AssetID QueryAssets_(Assets* assets, AssetType type, u32 subtype, RandomSequence* seq, GameProperties* properties, b32 derivedAssets = false, u16 startingIndex = 0, u16 onePastEndingIndex = 0)
{
    AssetID result = {};
    if(type)
    {
        AssetArray* array = assets->assets + type;
        if(subtype < array->subtypeCount)
        {
            AssetSubtypeArray* subtypeArray = array->subtypes + subtype;
            if(subtypeArray->standardAssetCount > 0)
            {
                u16 matching = 0;
                u16 matchingAssets[32];
                
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
                    if(MatchesProperties(asset, properties))
                    {
                        matchingAssets[matching++] = assetIndex;
                    }
                }
                
                if(matching > 0)
                {
                    result.type = SafeTruncateToU16(type);
                    result.subtype = SafeTruncateToU16(subtype);
                    
                    u32 choice = RandomChoice(seq, matching);
                    result.index = matchingAssets[choice];
                }
            }
        }
    }
    
    return result;
}


internal AssetID* GetAllSkinBitmaps(MemoryPool* tempPool, Assets* assets, AssetImageType skin, GameProperties* skinProperties, u32* bitmapCount)
{
    AssetID* result = 0;
    *bitmapCount = 0;
    
    AssetArray* array = assets->assets + AssetType_Image;
    if((u32) skin < array->subtypeCount)
    {
        AssetSubtypeArray* skinBitmaps = array->subtypes + skin;
        
        u16 totalAssetCount = skinBitmaps->standardAssetCount + skinBitmaps->derivedAssetCount;
        *bitmapCount = totalAssetCount;
        
        result = PushArray(tempPool, AssetID, totalAssetCount);
        for(u16 assetIndex = 0; assetIndex < totalAssetCount; ++assetIndex)
        {
            AssetID* dest = result + assetIndex;
            dest->type = AssetType_Image;
            dest->subtype = SafeTruncateToU16(skin);
            dest->index = assetIndex;
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
    
    FreeAsset(assets, asset);
    
    Tokenizer tokenizer = {};;
    tokenizer.at = (char*) tempBuffer.ptr;
    
    char* assetType = metaAsset_assetType[ID.type];
    String structName = {};
    structName.ptr = assetType;
    structName.length = StrLen(assetType);
    
    
    u32 finalSize = GetStructSize(structName, &tokenizer);
    asset->data = AcquireAssetMemory(assets, finalSize, asset);
    
    tokenizer.at = (char*) tempBuffer.ptr;
    ParseBufferIntoStruct(structName, &tokenizer, asset->data, finalSize);
    
    
#if 0    
    Stream test = PushStream(&tempPool, sourceBuffer.size + 1000);
    DumpStructToStream(structName, &test, asset->data);
#endif
    
    
    asset->state = Asset_loaded;
    
    Clear(&tempPool);
}

internal void PreloadAll(Assets* assets, u16 type, u16 subtype, b32 immediate)
{
    AssetSubtypeArray* assetArray = GetAssetSubtype(assets, type, subtype);
    Assert(assetArray);
    BitmapId ID = {};
    ID.type = type;
    ID.subtype = subtype;
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

inline void PreloadAllGroundBitmaps(Assets* assets)
{
    PreloadAll(assets, AssetType_Image, AssetImage_default, true);
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
        
        if(asset->state == Asset_loaded || asset->state == Asset_locked)
        {
            result.asset = asset;
            if(IsValid(&asset->textureHandle))
            {
                DLLIST_REMOVE(&asset->LRU);
                DLLIST_INSERT_AS_LAST(&assets->LRUSentinel, &asset->LRU);
            }
            
            DLLIST_REMOVE(asset);
            DLLIST_INSERT(&assets->assetSentinel, asset);
            CompletePastWritesBeforeFutureWrites;
        }
    }
    
    return result;
}

struct ColoredBitmap
{
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
        result.coloration = get.info->coloration.color;
    }
    else
    {
        result.bitmap = get.asset ? &get.asset->bitmap : 0;
        result.coloration = V4(1, 1, 1, 1);
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

#define GetData(assets, type, ID) (type*)GetDataDefinition_(assets, AssetType_##type, ID)
inline void* GetDataDefinition_(Assets* assets, AssetType type, AssetID ID, b32 immediate = true)
{
    Assert(type == ID.type);
    Assert(IsValid(ID));
    
    GetGameAssetResult get = GetGameAsset(assets, ID);
    void* result = get.asset ? get.asset->data : 0;
    
    if(!result && immediate)
    {
        LoadDataFile(assets, ID, immediate);
        get = GetGameAsset(assets, ID);
        Assert(get.asset);
        result = get.asset->data;
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
        result.subtype = fontID.subtype;
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
            
            result = QueryAssets_(assets, (AssetType) skeletonID.type, skeletonID.subtype, seq, properties, true, startingIndex, endingIndex);
            
        }
    }
    
    return result;
}

internal AssetID QueryAnimations(Assets* assets, AssetSkeletonType skeleton, RandomSequence* seq, GameProperties* properties)
{
    AssetID result = {};
    AssetID skeletonID = QuerySkeletons(assets, skeleton, seq, properties);
    if(IsValid(skeletonID))
    {
        result = GetMatchingAnimationForSkeleton(assets, skeletonID, seq, properties);
    }
    
    return result;
}
