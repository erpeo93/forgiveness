#if 0
void OutputSineWave( PlatformSoundBuffer* soundBuffer, GameState* gameState )
{
    i32 toneVolume = 4000;
    i32 frequency = 256;
    i32 wavePeriod = soundBuffer->samplesPerSecond / frequency;
    
    i16* samples = soundBuffer->samples;
    for( u32 counter = 0; counter < soundBuffer->sampleCount; counter++ )
    {
        r32 sineValue = sinf( gameState->tSine );
        i16 sampleValue = ( i16 ) ( sineValue * toneVolume );
        *samples++ = sampleValue;
        *samples++ = sampleValue;
        gameState->tSine += 3.14f * 2.0f * ( 1.0f / wavePeriod );
    }
}
#endif

internal void InitializeSoundState( SoundState* soundState, MemoryPool* pool )
{
    soundState->firstFreeSound = 0;
    soundState->firstPlayingSound = 0;
    soundState->pool = pool;
    soundState->masterVolume = V2(1.0f, 1.0f);
}

internal PlayingSound* PlaySound(SoundState* soundState, Assets* assets, SoundId ID, r32 distanceFromPlayer, r32 decibelOffset = 0, r32 frequency = 1.0, r32 delay = 0.0f, r32 toleranceDistance = 1.0f,r32 distanceFalloffCoeff = 1.0f)
{
    if(!soundState->firstFreeSound)
    {
        soundState->firstFreeSound = PushStruct(soundState->pool, PlayingSound);
        soundState->firstFreeSound;
    }
    PAKSound* info = GetSoundInfo(assets, ID);
    r32 decibel = Min(info->decibelLevel + decibelOffset, 0);
    
    if(distanceFromPlayer > toleranceDistance)
    {
        decibel *= (distanceFalloffCoeff * Square(distanceFromPlayer));
    }
    
    
    
    r32 desiredSampleValue = (i16) (Pow(10, decibel / 20) * (r32) I16_MAX);
    r32 volume = desiredSampleValue / (r32) info->maxSampleValue;
    
    
    PlayingSound* newSound = soundState->firstFreeSound;
    newSound->currentVolume = V2(volume, volume);
    newSound->playingIndex = 0;
    newSound->ID = ID;
    
    
    
    
    newSound->delay = delay;
    newSound->dSample = frequency;
    
    soundState->firstFreeSound = newSound->next;
    newSound->next = soundState->firstPlayingSound;
    soundState->firstPlayingSound = newSound;
    return newSound;
}

internal void ChangeVolume(SoundState* soundState, PlayingSound* sound, r32 fadeDuration, Vec2 targetVolume)
{
    if( fadeDuration <= 0 )
    {
        sound->currentVolume = targetVolume;
    }
    else
    {
        r32 oneOverFadeDuration = 1.0f / fadeDuration;
        sound->targetVolume = targetVolume;
        sound->dVolume = (  sound->targetVolume - sound->currentVolume ) * oneOverFadeDuration;
    }
}

internal void PlayMixedAudio(SoundState* soundState, r32 timeToAdvance, PlatformSoundBuffer* soundBuffer, Assets* assets, MemoryPool* tempPool)
{
    TempMemory MixerMemory = BeginTemporaryMemory(tempPool);
    
    Assert((soundBuffer->sampleCount & 3) == 0);
    u32 ChunkCount = soundBuffer->sampleCount / 4;
    
    __m128 *RealChannel0 = PushArray(tempPool, __m128, ChunkCount, AlignClear( 16 ) );
    __m128 *RealChannel1 = PushArray(tempPool, __m128, ChunkCount, AlignClear( 16 ) );
    
    r32 SecondsPerSample = 1.0f / (r32)soundBuffer->samplesPerSecond;
#define AudioStateOutputChannelCount 2
    
    __m128 One = _mm_set1_ps(1.0f);
    __m128 Zero = _mm_set1_ps(0.0f);
    
    // NOTE(leonardo): Clear out the mixer channels   
    {
        __m128 *Dest0 = RealChannel0;
        __m128 *Dest1 = RealChannel1;
        for(u32 SampleIndex = 0;
            SampleIndex < ChunkCount;
            ++SampleIndex)
        {
            _mm_store_ps((float *)Dest0++, Zero);
            _mm_store_ps((float *)Dest1++, Zero);
        }
    }
    
    // NOTE(leonardo): Sum all sounds
    for(PlayingSound **PlayingSoundPtr = &soundState->firstPlayingSound; *PlayingSoundPtr; )
    {
        PlayingSound *PlayingSound = *PlayingSoundPtr;
        
        b32 SoundFinished = false;
        if(PlayingSound->delay <= 0)
        {
            
            u32 TotalChunksToMix = ChunkCount;
            __m128 *Dest0 = RealChannel0;
            __m128 *Dest1 = RealChannel1;
            
            while(TotalChunksToMix && !SoundFinished)
            {
                Sound*LoadedSound = GetSound(assets, PlayingSound->ID);
                if(LoadedSound)
                {
                    Vec2 Volume = PlayingSound->currentVolume;
                    Vec2 dVolume = SecondsPerSample*PlayingSound->dVolume;
                    Vec2 dVolumeChunk = 4.0f*dVolume;
                    r32 dSample = PlayingSound->dSample;
                    r32 dSampleChunk = 4.0f*dSample;
                    
                    // NOTE(leonardo): Channel 0
                    __m128 MasterVolume0 = _mm_set1_ps(soundState->masterVolume.E[0]);
                    __m128 Volume0 = _mm_setr_ps(Volume.E[0] + 0.0f*dVolume.E[0],
                                                 Volume.E[0] + 1.0f*dVolume.E[0],
                                                 Volume.E[0] + 2.0f*dVolume.E[0],
                                                 Volume.E[0] + 3.0f*dVolume.E[0]);
                    __m128 dVolume0 = _mm_set1_ps(dVolume.E[0]);
                    __m128 dVolumeChunk0 = _mm_set1_ps(dVolumeChunk.E[0]);
                    
                    // NOTE(leonardo): Channel 1
                    __m128 MasterVolume1 = _mm_set1_ps(soundState->masterVolume.E[1]);
                    __m128 Volume1 = _mm_setr_ps(Volume.E[1] + 0.0f*dVolume.E[1],
                                                 Volume.E[1] + 1.0f*dVolume.E[1],
                                                 Volume.E[1] + 2.0f*dVolume.E[1],
                                                 Volume.E[1] + 3.0f*dVolume.E[1]);
                    __m128 dVolume1 = _mm_set1_ps(dVolume.E[1]);
                    __m128 dVolumeChunk1 = _mm_set1_ps(dVolumeChunk.E[1]);
                    
                    Assert(PlayingSound->playingIndex >= 0.0f);
                    
                    u32 ChunksToMix = TotalChunksToMix;
                    r32 RealChunksRemainingInSound =
                        (LoadedSound->countSamples - RoundReal32ToI32(PlayingSound->playingIndex)) / dSampleChunk;
                    u32 ChunksRemainingInSound = RoundReal32ToI32(RealChunksRemainingInSound);
                    if(ChunksToMix > ChunksRemainingInSound)
                    {
                        ChunksToMix = ChunksRemainingInSound;
                    }
                    
                    u32 VolumeEndAt[AudioStateOutputChannelCount] = {};
                    for(u32 ChannelIndex = 0;
                        ChannelIndex < ArrayCount(VolumeEndAt);
                        ++ChannelIndex)
                    {
                        // TODO(leonardo): Fix the "both volumes end at the same time" bug
                        if(dVolumeChunk.E[ChannelIndex] != 0.0f)
                        {
                            r32 DeltaVolume = (PlayingSound->targetVolume.E[ChannelIndex] -
                                               Volume.E[ChannelIndex]);
                            u32 VolumeChunkCount = (u32)(((DeltaVolume / dVolumeChunk.E[ChannelIndex]) + 0.5f));
                            if(ChunksToMix > VolumeChunkCount)
                            {
                                ChunksToMix = VolumeChunkCount;
                                VolumeEndAt[ChannelIndex] = VolumeChunkCount;
                            }
                        }
                    }
                    
                    // TODO(leonardo): Handle stereo!
                    r32 BeginSamplePosition = PlayingSound->playingIndex;
                    r32 EndSamplePosition = BeginSamplePosition + ChunksToMix*dSampleChunk;
                    r32 LoopIndexC = (EndSamplePosition - BeginSamplePosition) / (r32)ChunksToMix;
                    for(u32 LoopIndex = 0;
                        LoopIndex < ChunksToMix;
                        ++LoopIndex)
                    {
                        r32 SamplePosition = BeginSamplePosition + LoopIndexC*(r32)LoopIndex;
                        // TODO(leonardo): Move volume up here to explicit.
#if 1
                        __m128 SamplePos = _mm_setr_ps(SamplePosition + 0.0f*dSample,
                                                       SamplePosition + 1.0f*dSample,
                                                       SamplePosition + 2.0f*dSample,
                                                       SamplePosition + 3.0f*dSample);
                        __m128i SampleIndex = _mm_cvttps_epi32(SamplePos);
                        __m128 Frac = _mm_sub_ps(SamplePos, _mm_cvtepi32_ps(SampleIndex));
                        
                        __m128 SampleValueF = _mm_setr_ps(LoadedSound->samples[0][((i32 *)&SampleIndex)[0]],
                                                          LoadedSound->samples[0][((i32 *)&SampleIndex)[1]],
                                                          LoadedSound->samples[0][((i32 *)&SampleIndex)[2]],
                                                          LoadedSound->samples[0][((i32 *)&SampleIndex)[3]]);
                        __m128 SampleValueC = _mm_setr_ps(LoadedSound->samples[0][((i32 *)&SampleIndex)[0] + 1],
                                                          LoadedSound->samples[0][((i32 *)&SampleIndex)[1] + 1],
                                                          LoadedSound->samples[0][((i32 *)&SampleIndex)[2] + 1],
                                                          LoadedSound->samples[0][((i32 *)&SampleIndex)[3] + 1]);
                        
                        __m128 SampleValue = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(One, Frac), SampleValueF),
                                                        _mm_mul_ps(Frac, SampleValueC));
#else                   
                        __m128 SampleValue = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 0.0f*dSample)],
                                                         LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 1.0f*dSample)],
                                                         LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 2.0f*dSample)],
                                                         LoadedSound->Samples[0][RoundReal32ToInt32(SamplePosition + 3.0f*dSample)]);
#endif
                        
                        __m128 D0 = _mm_load_ps((float *)&Dest0[0]);
                        __m128 D1 = _mm_load_ps((float *)&Dest1[0]);
                        
                        D0 = _mm_add_ps(D0, _mm_mul_ps(_mm_mul_ps(MasterVolume0, Volume0), SampleValue));
                        D1 = _mm_add_ps(D1, _mm_mul_ps(_mm_mul_ps(MasterVolume1, Volume1), SampleValue));
                        
                        _mm_store_ps((float *)&Dest0[0], D0);
                        _mm_store_ps((float *)&Dest1[0], D1);
                        
                        ++Dest0;
                        ++Dest1;
                        Volume0 = _mm_add_ps(Volume0, dVolumeChunk0);
                        Volume1 = _mm_add_ps(Volume1, dVolumeChunk1);
                    }
                    
                    PlayingSound->currentVolume.E[0] = ((r32 *)&Volume0)[0];
                    PlayingSound->currentVolume.E[1] = ((r32 *)&Volume1)[1];
                    for(u32 ChannelIndex = 0;
                        ChannelIndex < ArrayCount(VolumeEndAt);
                        ++ChannelIndex)
                    {
                        if(VolumeEndAt[ChannelIndex] == ChunksToMix)
                        {
                            PlayingSound->currentVolume.E[ChannelIndex] =
                                PlayingSound->targetVolume.E[ChannelIndex];
                            PlayingSound->dVolume.E[ChannelIndex] = 0.0f;
                        }
                    }
                    
                    PlayingSound->playingIndex = EndSamplePosition;
                    Assert(TotalChunksToMix >= ChunksToMix);
                    TotalChunksToMix -= ChunksToMix;
                    
                    if(ChunksToMix == ChunksRemainingInSound)
                    {
                        SoundFinished = true;
                    }
                }
                else
                {
                    LoadSound(assets, PlayingSound->ID);
                    break;
                }
            }
        }
        else
        {
            PlayingSound->delay -= timeToAdvance;
        }
        
        if(SoundFinished)
        {
            if(PlayingSound == soundState->music)
            {
                soundState->music = 0;
            }
            
            *PlayingSoundPtr = PlayingSound->next;
            PlayingSound->next = soundState->firstFreeSound;
            soundState->firstFreeSound = PlayingSound;
        }
        else
        {
            PlayingSoundPtr = &PlayingSound->next;
        }
    }
    
    // NOTE(leonardo): Convert to 16-bit
    {
        __m128 *Source0 = RealChannel0;
        __m128 *Source1 = RealChannel1;
        
        __m128i *SampleOut = (__m128i *)soundBuffer->samples;
        for(u32 SampleIndex = 0;
            SampleIndex < ChunkCount;
            ++SampleIndex)
        {
            __m128 S0 = _mm_load_ps((float *)Source0++);
            __m128 S1 = _mm_load_ps((float *)Source1++);
            
            __m128i L = _mm_cvtps_epi32(S0);
            __m128i R = _mm_cvtps_epi32(S1);
            
            __m128i LR0 = _mm_unpacklo_epi32(L, R);
            __m128i LR1 = _mm_unpackhi_epi32(L, R);
            
            __m128i S01 = _mm_packs_epi32(LR0, LR1);
            
            *SampleOut++ = S01;
        }
    }
    
    EndTemporaryMemory(MixerMemory);
}


internal PlayingSound* SoundTrigger(GameModeWorld* worldMode, SoundTrig* trigger)
{
    PlayingSound* result = 0;
    
    GameState* gameState = worldMode->gameState;
    SoundState* soundState = &gameState->soundState;
    u32 subtype = GetAssetSubtype(gameState->assets, AssetType_Sound, trigger->subtypeHash);
    SoundId sound = QuerySounds(gameState->assets, subtype, &worldMode->entropy, &trigger->properties);
    if(IsValid(sound))
    {
        result = PlaySound(soundState, gameState->assets, sound, 0);
    }
    
    return result;
}

internal void MusicTrigger(GameModeWorld* worldMode, char* musicType, u32 priority)
{
    GameState* gameState = worldMode->gameState;
    SoundState* soundState = &gameState->soundState;
    
    if(priority > gameState->musicPriority || !soundState->music)
    {
        gameState->musicPriority = priority;
        if(soundState->music)
        {
            ChangeVolume(soundState, soundState->music, 1.0f, V2(0.0f, 0.0f));
            soundState->music = 0;
        }
        
        SoundTrig trig = {};
        trig.subtypeHash = StringHash(musicType);
        AddGameProperty(&trig.properties, season, worldMode->season);
        soundState->music = SoundTrigger(worldMode, &trig);
    }
}

internal SoundMapping InitSoundMapping(SoundMappingDefinition* mapping, RandomSequence* seq)
{
    SoundMapping result = {};
    result.count = 0;
    result.maxCount = mapping->maxRepeatCount;
    result.time = 0;
    result.targetTime = mapping->targetTime;
    
    if(seq)
    {
        result.time = RandomUni(seq) * result.targetTime;
    }
    
    result.trig.subtypeHash = mapping->asset.subtypeHash;
    
    for(u32 propertyIndex = 0; propertyIndex < mapping->propertyCount; ++propertyIndex)
    {
        AddGamePropertyRaw(&result.trig.properties, mapping->properties[propertyIndex]);
    }
    return result;
}

internal void UpdateSoundMapping(GameModeWorld* worldMode, SoundMapping* mapping, r32 elapsedTime)
{
    if(!mapping->maxCount || mapping->count < mapping->maxCount)
    {
        mapping->time += elapsedTime;
        if(mapping->time >= mapping->targetTime)
        {
            SoundTrigger(worldMode, &mapping->trig);
            mapping->time = 0;
            ++mapping->count;
        }
    }
}

internal void AddSoundEffectIfNotPresent(SoundEffectComponent* sounds, SoundEffectDefinition* sound, u32 index)
{
    b32 found = false;
    for(u32 soundIndex = 0; soundIndex < sounds->soundCount; ++soundIndex)
    {
        SoundEffect* effect = sounds->sounds + soundIndex;
        if(effect->ID == index)
        {
            found = true;
            break;
        }
    }
    
    if(!found && sounds->soundCount < ArrayCount(sounds->sounds))
    {
        SoundEffect* dest = sounds->sounds + sounds->soundCount++;
        dest->ID = index;
        dest->time = sound->time;
        dest->sound = InitSoundMapping(&sound->sound, 0);
    }
}

internal void AddSoundIfMatches(SoundEffectComponent* mappings, BaseComponent* base, SoundEffectDefinition* sound, u32 index, b32 defaultMatches)
{
    
#if 0    
    b32 matches = defaultMatches;
    for(u32 testIndex = 0; testIndex < sound->propertyCount; ++testIndex)
    {
        matches = false;
        GameProperty* testProperty = sound->properties + testIndex;
        for(u32 propertyIndex = 0; propertyIndex < ArrayCount(base->properties); ++propertyIndex)
        {
            GameProperty* entityProperty = base->properties + propertyIndex;
            if(AreEqual(*entityProperty, *testProperty))
            {
                matches = true;
                break;
            }
        }
        
        if(!matches)
        {
            break;
        }
    }
    
    if(matches)
    {
        AddSoundEffectIfNotPresent(mappings, sound, index);
    }
#endif
    
}

internal void SoundEventTrigger(GameModeWorld* worldMode, EntityID ID, GameProperty trigger)
{
    if(HasComponent(ID, SoundEffectComponent))
    {
        BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
        SoundEffectComponent* mappings = GetComponent(worldMode, ID, SoundEffectComponent);
        
        EntityDefinition* definition = GetData(worldMode->gameState->assets, EntityDefinition, EntityTypeToAssetID(base->type));
        if(definition)
        {
            for(ArrayCounter soundIndex = 0; soundIndex < definition->client.soundEffectsCount; ++soundIndex)
            {
                SoundEffectDefinition* sound = definition->client.soundEffects + soundIndex;
                if(AreEqual(sound->triggerType, trigger))
                {
                    AddSoundIfMatches(mappings, base, sound, soundIndex, true);
                }
            }
        }
    }
}

STANDARD_ECS_JOB_CLIENT(UpdateEntitySoundEffects)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    SoundEffectComponent* mappings = GetComponent(worldMode, ID, SoundEffectComponent);
    
    EntityDefinition* definition = GetData(worldMode->gameState->assets, EntityDefinition, EntityTypeToAssetID(base->type));
    if(definition)
    {
        for(ArrayCounter soundIndex = 0; soundIndex < definition->client.soundEffectsCount; ++soundIndex)
        {
            SoundEffectDefinition* sound = definition->client.soundEffects + soundIndex;
            
            if(!IsValid(sound->triggerType))
            {
                AddSoundIfMatches(mappings, base, sound, soundIndex, false);
            }
        }
    }
    
    for(u32 soundIndex = 0; soundIndex < mappings->soundCount; ++soundIndex)
    {
        SoundEffect* sound = mappings->sounds + soundIndex;
        UpdateSoundMapping(worldMode, &sound->sound, elapsedTime);
        
        if(sound->time >= 0)
        {
            sound->time -= elapsedTime;
            if(sound->time <= 0)
            {
                *sound = mappings->sounds[--mappings->soundCount];
            }
        }
    }
}
