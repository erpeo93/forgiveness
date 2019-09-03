// NOTE(Leonardo): all the functions here can be called just by the main thread!
inline Assets* DEBUGGetGameAssets(PlatformClientMemory* memory)
{
    GameState* gameState = (GameState*) memory->gameState;
    Assets* result = gameState->assets;
    return result;
}

inline PlatformFileHandle* GetHandleFor(Assets* assets, u32 fileIndex)
{
    Assert(fileIndex < assets->fileCount);
    AssetFile* file = assets->files + fileIndex;
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
    Assert(asset->state == Asset_loaded || asset->state == Asset_preloaded);
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

inline Asset* GetAssetRaw(Assets* assets, AssetID ID)
{
    Assert(ID.type);
    Assert(ID.type < AssetType_Count);
    AssetArray* array = assets->assets + ID.type;
    
    Assert(ID.subtype < array->subtypeCount);
    AssetSubtypeArray* subtype = array->subtypes + ID.subtype;
    
    Assert(ID.index < (subtype->assetCount));
    Asset* asset = subtype->assets + ID.index;
    
    return asset;
}

inline void LockAssetForCurrentFrame(Assets* assets, AssetID ID)
{
    Asset* asset = GetAssetRaw(assets, ID);
    asset->state = Asset_locked;
    DLLIST_REMOVE(&asset->LRU);
    DLLIST_INSERT_AS_LAST(&assets->lockedLRUSentinel, &asset->LRU);
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

internal void* AcquireAssetMemory(Assets* assets, u32 size, Asset* destAsset, AssetID ID)
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
    
    destAsset->ID = ID;
    destAsset->dataSize = size;
    
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
        AssetLRULink* sentinel = &assets->LRUSentinel;
        Assert(!DLLIST_ISEMPTY(sentinel));
        
        AssetLRULink* free = sentinel->next;
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
        
        SpecialTexture* LRU = (SpecialTexture*) first;
        result = LRU->textureHandle.index;
        Assert(result >= MAX_TEXTURE_COUNT);
        Clear(&LRU->textureHandle);
    }
    
    return result;
}

inline void RefreshSpecialTexture(Assets* assets, AssetLRULink* LRU)
{
    DLLIST_REMOVE(LRU);
    DLLIST_INSERT_AS_LAST(&assets->specialLRUSentinel, LRU);
}

struct AssetBoilerplate
{
    TaskWithMemory* task;
    Asset* asset;
};

internal AssetBoilerplate BeginAssetBoilerplate(Assets* assets, AssetID ID)
{
    AssetBoilerplate result = {};
    Asset* asset = GetAssetRaw(assets, ID);
    if(asset && asset->fileIndex < assets->fileCount)
    {
        if(AtomicCompareExchangeUint32((u32*) &asset->state, Asset_queued, Asset_unloaded) == Asset_unloaded)
        {
            result.task = BeginTaskWithMemory(assets->tasks, assets->taskCount, false);
            if(result.task)
            {
                result.asset = asset;
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
    work->handle = GetHandleFor(assets, asset->fileIndex);
    work->dest = asset->data;
    work->memorySize = size;
    work->dataOffset = asset->paka.dataOffset;
    work->asset = asset;
    work->finalState = state;
    work->finalizeOperation = finalize;
    
    ++assets->threadsReading;
    work->threadsReading = &assets->threadsReading;
    
    work->task = task;
    
    
    platformAPI.PushWork(assets->loadQueue, LoadAssetThreaded, work);
}


void LoadBitmap(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.task)
    {
        Asset* asset = boilerplate.asset;
        PAKBitmap* info = &asset->paka.bitmap;
        u32 size = info->dimension[0] * info->dimension[1] * 4;
        
        asset->data = AcquireAssetMemory(assets, size, asset, ID);
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
        
        asset->data = AcquireAssetMemory(assets, size, asset, ID);
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
        asset->data = AcquireAssetMemory(assets, size, asset, ID);
        
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
        
        asset->data = AcquireAssetMemory(assets, size, asset, ID);
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


void LoadSkeleton(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.task)
    {
        Asset* asset = boilerplate.asset;
        PAKSkeleton* info = &asset->paka.skeleton;
        
        asset->skeleton.animationCount = info->animationCount;
        
        asset->state = Asset_loaded;
        EndTaskWithMemory(boilerplate.task);
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
        
        asset->data = AcquireAssetMemory(assets, size, asset, ID);
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

void LoadDataFile(Assets* assets, AssetID ID)
{
    AssetBoilerplate boilerplate = BeginAssetBoilerplate(assets, ID);
    if(boilerplate.task)
    {
        Asset* asset = boilerplate.asset;
        u32 size = asset->paka.dataFile.rawSize;
        asset->data = AcquireAssetMemory(assets, size, asset, ID);
        EndAssetBoilerplate(assets, boilerplate, size, Asset_preloaded);
    }
}


internal AssetSubtypeArray* GetAssetSubtypeForFile(Assets* assets, PAKFileHeader* header)
{
    Assert(header->assetType < AssetType_Count);
    AssetArray* assetTypeArray = assets->assets + header->assetType;
    
    Assert(header->assetSubType < assetTypeArray->subtypeCount);
    AssetSubtypeArray* assetSubtypeArray = assetTypeArray->subtypes + header->assetSubType;
    
    return assetSubtypeArray;
}

internal void LoadAssetFile(Assets* assets, AssetFile* file, u32 fileIndex, AssetSubtypeArray* assetSubtypeArray, MemoryPool* pool)
{
    TempMemory assetMemory = BeginTemporaryMemory(pool);
    
    u16 assetCount = file->header.standardAssetCount + file->header.derivedAssetCount;
    u32 pakAssetArraySize = sizeof(PAKAsset) * assetCount;
    PAKAsset* pakAssetArray = (PAKAsset*) PushSize(pool, pakAssetArraySize, NoClear());
    
    u64 assetOffset = sizeof(PAKFileHeader);
    platformAPI.ReadFromFile(&file->handle, assetOffset, pakAssetArraySize, pakAssetArray);
    
    
    for(u32 assetIndex = 0; assetIndex < assetCount; assetIndex++)
    {
        Asset* dest = assetSubtypeArray->assets + assetIndex;
        dest->paka = pakAssetArray[assetIndex];
        dest->fileIndex = fileIndex;
        dest->state = Asset_unloaded;
    }
    
    EndTemporaryMemory(assetMemory);
}


internal void ReloadAssetFile(Assets* assets, AssetFile* file, u32 fileIndex, MemoryPool* pool)
{
    AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeForFile(assets, &file->header);
    for(u32 assetIndex = 0; assetIndex < assetSubtypeArray->assetCount; ++assetIndex)
    {
        Asset* asset = assetSubtypeArray->assets + assetIndex;
        FreeAsset(assets, asset);
    }
    
    LoadAssetFile(assets, file, fileIndex, assetSubtypeArray, pool);
}

internal void ReplaceChangedAssetFiles(Assets* assets)
{
    PlatformFileGroup changed = platformAPI.GetAllFilesBegin(PlatformFile_reloadedAsset, ASSETS_PATH);
    if(changed.fileCount)
    {
        for(u32 fileIndex = 0; fileIndex < changed.fileCount; ++fileIndex)
        {
            InvalidCodePath;
#if 0            
            GetCorrenspondingFileHandle();
            CloseHandle();
            file->handle = thisNewHandle;
            char assetName[128];
            platformAPI.substituteFileContentAtomically(assetName, fg.name);
            assets->file.fileIndex = fgFile;
            ReloadAssetFile(assets, fg);
#endif
            
        }
    }
    platformAPI.GetAllFilesEnd(&changed);
    platformAPI.DeleteFiles(PlatformFile_reloadedAsset, ASSETS_PATH);
}

internal void ComputeAssetFilePath(AssetFile* file, char* basePath, char* buffer, u32 bufferSize)
{
    char* assetType = GetAssetTypeName(file->header.assetType);
    char* assetSubtype = GetAssetSubtypeName(file->header.assetType, file->header.assetSubType);
    
    FormatString(buffer, bufferSize, "%s/%s/%s", basePath, assetType, assetSubtype);
}

internal void WriteAssetLabelsToBuffer(Buffer* buffer, AssetType type, PAKAsset* asset, b32 derivedAsset)
{
    unm rollbackSize = OutputToBuffer(buffer, "%s:", asset->name);
    u32 nameSize = SafeTruncateUInt64ToU32(rollbackSize);
    
    b32 nothingWrote = true;
    
    for(u32 labelIndex = 0; labelIndex < ArrayCount(asset->labels); ++labelIndex)
    {
        PAKLabel* label = asset->labels + labelIndex;
        if(label->label)
        {
            char* labelType = GetMetaLabelTypeName(label->label);
            char* labelValue = GetMetaLabelValueName(label->label, label->value);
            
            if(labelType && labelValue)
            {
                OutputToBuffer(buffer, "%s=%s,", labelType, labelValue);
                nothingWrote = false;
            }
        }
    }
    
    switch(type)
    {
        case AssetType_Image:
        {
            if(asset->bitmap.align[0] != 0)
            {
                OutputToBuffer(buffer, "%s=%f,", IMAGE_PROPERTY_ALIGN_X, asset->bitmap.align[0]);
                nothingWrote = false;
            }
            if(asset->bitmap.align[1] != 0)
            {
                OutputToBuffer(buffer, "%s=%f,", IMAGE_PROPERTY_ALIGN_Y, asset->bitmap.align[1]);
                nothingWrote = false;
            }
        } break;
        
        case AssetType_Skeleton:
        {
            if(derivedAsset)
            {
                if(asset->animation.syncThreesoldMS != 0)
                {
                    OutputToBuffer(buffer, "%s=%f,", ANIMATION_PROPERTY_SYNC_THREESOLD, asset->animation.syncThreesoldMS);
                    nothingWrote = false;
                }
                
                if(asset->animation.preparationThreesoldMS != 0)
                {
                    OutputToBuffer(buffer, "%s=%f,", ANIMATION_PROPERTY_PREPARATION_THREESOLD, asset->animation.preparationThreesoldMS);
                    nothingWrote = false;
                }
            }
        } break;
    }
    
    
    if(nothingWrote)
    {
        buffer->b -= nameSize;
        buffer->size += nameSize;
    }
    else
    {
        OutputToBuffer(buffer, ";");
    }
}

internal b32 IsDataFile(AssetType type)
{
    b32 result = (type != AssetType_Font &&
                  type != AssetType_Image &&
                  type != AssetType_Sound &&
                  type != AssetType_Model &&
                  type != AssetType_Skeleton);
    return result;
}

internal void WritebackAssetFileToFileSystem(Assets* assets, AssetFile* file, char* basePath)
{
    char path[128];
    ComputeAssetFilePath(file, basePath, path, sizeof(path));
    
    AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeForFile(assets, &file->header);
    Assert(IsDataFile((AssetType) file->header.assetType));
    
    MemoryPool tempPool = {};
    
    Buffer metaDataBuffer = PushBuffer(&tempPool, MegaBytes(1));
    
    for(u32 assetIndex = 0; assetIndex < assetSubtypeArray->assetCount; ++assetIndex)
    {
        Asset* asset = assetSubtypeArray->assets + assetIndex;
        
        b32 derivedAsset = (assetIndex >= file->header.standardAssetCount);
        
        WriteAssetLabelsToBuffer(&metaDataBuffer, (AssetType) file->header.assetType, &asset->paka, derivedAsset);
        
        TempMemory fileMemory = BeginTemporaryMemory(&tempPool);
        Buffer fileBuffer = PushBuffer(&tempPool, asset->dataFile.rawSize);
        
        
        MetaAsset assetType = metaAsset_assetType[file->header.assetType];
        String structName = {};
        structName.b = (u8*) assetType.name;
        structName.size = StrLen(assetType.name);
        
        DumpStructToBuffer(structName, &fileBuffer, asset->data);
        
        char* filename = asset->paka.name;
        platformAPI.ReplaceFile(PlatformFile_data, path, filename, fileBuffer.b, fileBuffer.size);
        EndTemporaryMemory(fileMemory);
    }
    
    platformAPI.ReplaceFile(PlatformFile_markup, path, LABELS_FILE_NAME, metaDataBuffer.b, metaDataBuffer.size);
    Clear(&tempPool);
}

internal void UpdateAssetFileContent(Assets* assets, Asset* asset, void* newContent, u32 newContentSize)
{
    Assert(IsDataFile((AssetType) asset->ID.type));
    if(newContentSize != asset->dataSize)
    {
        FreeAsset(assets, asset);
        asset->data = AcquireAssetMemory(assets, newContentSize, asset, asset->ID);
        asset->state = Asset_loaded;
    }
    Copy(newContentSize, asset->data, newContent);
}


internal Assets* InitAssets(PlatformWorkQueue* loadQueue, TaskWithMemory* tasks, u32 taskCount, MemoryPool* pool, PlatformTextureOpQueue* textureQueue, memory_index size)
{
    Assets* assets = PushStruct(pool, Assets); 
    
    DLLIST_INIT(&assets->LRUSentinel);
    DLLIST_INIT(&assets->specialLRUSentinel);
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
    
    
    for(u32 assetTypeIndex = 0; assetTypeIndex < AssetType_Count; ++assetTypeIndex)
    {
        MetaAssetType metaType = metaAsset_subTypes[assetTypeIndex];
        
        AssetArray* assetArray = assets->assets + assetTypeIndex;
        assetArray->subtypeCount = metaType.subtypeCount;
        assetArray->subtypes = PushArray(pool, AssetSubtypeArray, assetArray->subtypeCount);
    }
    
    PlatformFileGroup fileGroup = platformAPI.GetAllFilesBegin(PlatformFile_uncompressedAsset, ASSETS_PATH);
    
    assets->fileCount = fileGroup.fileCount;
    assets->files = PushArray(pool, AssetFile, assets->fileCount);
    
    u32 fileIndex = 0;
    for(PlatformFileInfo* info = fileGroup.firstFileInfo; info; info = info->next)
    {
        AssetFile* file = assets->files + fileIndex;
        file->handle = platformAPI.OpenFile(&fileGroup, info);
        
        ZeroStruct(file->header);
        platformAPI.ReadFromFile(&file->handle, 0, sizeof(file->header), &file->header);
        PAKFileHeader* header = &file->header;
        if(header->magicValue != PAK_MAGIC_NUMBER)
        {
            platformAPI.FileError(&file->handle, "magic number not valid");
        }
        
        if(header->version != PAK_VERSION)
        {
            platformAPI.FileError(&file->handle, "pak file version not valid");
        }
        
        if(PlatformNoFileErrors(&file->handle))
        {
            u16 assetCount = (header->standardAssetCount + header->derivedAssetCount);
            AssetSubtypeArray* assetSubtypeArray = GetAssetSubtypeForFile(assets, header);
            assetSubtypeArray->assetCount = assetCount;
            assetSubtypeArray->assets = PushArray(pool, Asset, assetCount, NoClear());
            
            LoadAssetFile(assets, file, fileIndex, assetSubtypeArray, pool);
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

inline b32 MatchesLabels(Asset* asset, AssetLabels* labels)
{
    b32 result = true;
    
    for(u32 labelIndex = 0; labelIndex < ArrayCount(labels->labels); ++labelIndex)
    {
        PAKLabel* label = labels->labels + labelIndex;
        if(label->label)
        {
            b32 hasLabel = false;
            for(u32 assetLabelIndex = 0; assetLabelIndex < ArrayCount(asset->paka.labels); ++labelIndex)
            {
                PAKLabel* assetLabel = asset->paka.labels + assetLabelIndex;
                if(assetLabel->label == label->label)
                {
                    if(label->value == assetLabel->value)
                    {
                        hasLabel = true;
                    }
                    break;
                }
            }
            if(!hasLabel)
            {
                result = false;
                break;
            }
        }
    }
    
    return result;
}

internal AssetID QueryAssets(Assets* assets, AssetType type, u32 subtype, RandomSequence* seq, AssetLabels* labels, u32 startingIndex = 0, u32 endingIndex = 0)
{
    AssetID result = {};
    if(type)
    {
        AssetArray* array = assets->assets + type;
        
        if(subtype < array->subtypeCount)
        {
            AssetSubtypeArray* subtypeArray = array->subtypes + subtype;
            
            if(subtypeArray->assetCount > 0)
            {
                u32 matching = 0;
                u32 matchingAssets[32];
                
                if(!endingIndex)
                {
                    endingIndex = subtypeArray->assetCount;
                }
                Assert(startingIndex <= endingIndex);
                
                for(u32 assetIndex = startingIndex; assetIndex < endingIndex; ++assetIndex)
                {
                    Asset* asset = subtypeArray->assets + assetIndex;
                    if(MatchesLabels(asset, labels))
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

inline void LoadAssetDataStructure(Assets* assets, Asset* asset, AssetID ID)
{
    MemoryPool tempPool = {};
    
    Buffer sourceBuffer;
    sourceBuffer.b = (u8*) asset->data;
    sourceBuffer.size = asset->paka.dataFile.rawSize;
    
    Buffer tempBuffer = CopyBuffer(&tempPool, sourceBuffer);
    
    FreeAsset(assets, asset);
    
    Tokenizer tokenizer = {};;
    tokenizer.at = (char*) tempBuffer.b;
    
    MetaAsset assetType = metaAsset_assetType[ID.type];
    String structName = {};
    structName.b = (u8*) assetType.name;
    structName.size = StrLen(assetType.name);
    
    
    u32 finalSize = GetStructSize(structName, &tokenizer);
    asset->data = AcquireAssetMemory(assets, finalSize, asset, ID);
    
    tokenizer.at = (char*) tempBuffer.b;
    ParseBufferIntoStruct(structName, &tokenizer, asset->data);
    
    asset->state = Asset_loaded;
    
    Clear(&tempPool);
}

inline Asset* GetGameAsset(Assets* assets, AssetID ID, b32 insertAtLRUList = false)
{
    Asset* result = 0;
    Asset* asset = GetAssetRaw(assets, ID);
    if(asset)
    {
        if(asset->state == Asset_preloaded)
        {
            Assert(IsDataFile((AssetType)ID.type));
            LoadAssetDataStructure(assets, asset, ID);
            asset->state = Asset_loaded;
        }
        
        if(asset->state == Asset_loaded)
        {
            result = asset;
            if(insertAtLRUList)
            {
                if(IsValid(&asset->textureHandle))
                {
                    DLLIST_REMOVE(&asset->LRU);
                    DLLIST_INSERT_AS_LAST(&assets->LRUSentinel, &asset->LRU);
                }
            }
            
            DLLIST_INSERT(&assets->assetSentinel, result);
            
            CompletePastWritesBeforeFutureWrites;
        }
    }
    
    
    return result;
}

inline Bitmap* GetBitmap(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    Asset* asset = GetGameAsset(assets, ID, true);
    Bitmap* result = asset ? &asset->bitmap : 0;
    
    return result;
}

inline Font* GetFont(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    Asset* asset = GetGameAsset(assets, ID);
    Font* result = asset ? &asset->font : 0;
    
    return result;
}

inline Sound* GetSound(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    Asset* asset = GetGameAsset(assets, ID);
    Sound* result = asset ? &asset->sound : 0;
    
    return result;
}

inline Skeleton* GetSkeleton(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    Asset* asset = GetGameAsset(assets, ID);
    Skeleton* result = asset ? &asset->skeleton : 0;
    
    return result;
}

inline Animation* GetAnimation(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    Asset* asset = GetGameAsset(assets, ID);
    Animation* result = asset ? &asset->animation : 0;
    
    return result;
}

inline VertexModel* GetModel(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    
    Asset* asset = GetGameAsset(assets, ID);
    VertexModel* result = asset ? &asset->model : 0;
    
    return result;
}

#define GetData(type, assets, ID) (type*)GetDataDefinition_(assets, AssetType_##type, ID)
inline void* GetDataDefinition_(Assets* assets, AssetType type, AssetID ID)
{
    Assert(type == ID.type);
    Assert(IsValid(ID));
    
    Asset* asset = GetGameAsset(assets, ID);
    void* result = asset ? asset->data : 0;
    
    return result;
}

inline PAKAsset* GetPakAsset(Assets* assets, AssetID ID, b32 insertAtLRUList = false)
{
    PAKAsset* result = 0;
    Asset* asset = GetGameAsset(assets, ID, insertAtLRUList);
    if(asset)
    {
        result = &asset->paka;
    }
    
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

inline PAKModel* GetModelInfo(Assets* assets, AssetID ID)
{
    Assert(IsValid(ID));
    PAKAsset* asset = GetPakAsset(assets, ID);
    PAKModel* result = &asset->model;
    return result;
}



inline u32 GetGlyphFromCodePoint(Font* font, PAKFont* info, u32 desired)
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
    u32 glyph = GetGlyphFromCodePoint(font, info, desiredCodePoint);
    u32 prevGlyph = GetGlyphFromCodePoint(font, info, desiredPrevCodePoint);
    
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
        Asset* asset = GetAssetRaw(assets, fontID);
        
        u16 index = SafeTruncateToU16(GetGlyphFromCodePoint(font, info, desiredCodePoint));
        
        result.type = fontID.type;
        result.subtype = fontID.subtype;
        result.index = info->glyphAssetsFirstIndex + index;
    }
    
    return result;
    
}

inline AssetID GetMatchingAnimationForSkeleton(Assets* assets, AssetID skeletonID, RandomSequence* seq, AssetLabels* labels)
{
    AssetID result = {};
    
    Asset* asset = GetAssetRaw(assets, skeletonID);
    
    if(asset)
    {
        PAKSkeleton* info = GetSkeletonInfo(assets, skeletonID);
        u32 startingIndex = info->animationAssetsFirstIndex;
        u32 endingIndex = startingIndex + info->animationCount;
        
        result = QueryAssets(assets, (AssetType) skeletonID.type, skeletonID.subtype, seq, labels, startingIndex,endingIndex);
    }
    
    return result;
}