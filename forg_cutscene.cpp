internal void RenderLayeredScene( Assets* assets, RenderGroup* group, LayeredScene* scene, r32 tNormal )
{
    Vec3 cameraPosition = Lerp( scene->cameraStart, tNormal, scene->cameraEnd );
    
    TagVector match = {};
    TagVector weight = {};
    weight.E[Tag_shotIndex] = 100.0f;
    weight.E[Tag_layerIndex] = 1.0f;
    
    r32 fadeCoeff = 1.0f;
    if( tNormal < scene->tFade )
    {
        fadeCoeff = Clamp01MapToRange( 0.0f, tNormal, scene->tFade );
    }
    Vec4 color = V4( fadeCoeff, fadeCoeff, fadeCoeff, 1.0f );
    
    for( u32 layerIndex = 0; layerIndex < scene->layerCount; ++layerIndex )
    {
        SceneLayer* layer = scene->layers + layerIndex;
        match.E[Tag_layerIndex] = ( r32 ) ( layerIndex + 1 );
        match.E[Tag_shotIndex] = ( r32 ) scene->shotIndex;
        BitmapId BID = GetMatchingBitmap( assets, Asset_openingCutscene, &match, &weight );
        
        if( group )
        {
            b32 active = true;
            if( layer->flags & SceneLayerFlag_Transient )
            {
                active = ( tNormal >= layer->minTime && tNormal < layer->maxTime );
            }
            
            if( active )
            {
                Vec3 finalP = layer->P;
                if( layer->flags & SceneLayerFlag_AtInfinity )
                {
                    finalP.z += cameraPosition.z;
                }
                
                if( layer->flags & SceneLayerFlag_CounterX )
                {
                    finalP.x += cameraPosition.x;
                }
                else
                {
                    finalP.x -= cameraPosition.x;
                }
                
                if( layer->flags & SceneLayerFlag_CounterY )
                {
                    finalP.y += cameraPosition.y;
                }
                else
                {
                    finalP.y -= cameraPosition.y;
                }
                
                if( layer->flags & SceneLayerFlag_CounterZ )
                {
                    finalP.z += cameraPosition.z;
                }
                else
                {
                    finalP.z -= cameraPosition.z;
                }
                PushBitmap( group, UprightTransform(), BID, finalP, layer->height, V2( 1.0f, 1.0f ), color );
            }
        }
        else
        {
            PrefetchBitmap( assets, BID );
        }
    }
}


global_variable SceneLayer layers1[] = 
{
    { 0, 0,-220.0f, 350.0f, SceneLayerFlag_AtInfinity },
    { 0, 0, -220.0f, 350.0f, 0 },
    { 0, 0, -120.0f, 60.0f, 0 },
    { 0, 0, -90.0f, 80.0f, 0 },
    { 0, 0, -60.0f, 90.0f, 0 },
    { 30.0f, 0, -50.0f, 65.0f, 0 },
    { 0, -4.0f, -40.0f, 45.0f, 0 },
    { 2.0f, 0, -27.0f, 28.0f, 0 },
};

global_variable SceneLayer layers2[] = 
{
    { 3.0f, 0,-40.0f, 32.0f, 0 },
    { 0, 0.0f, -35.0f, 30.0f, 0 },
    { 0, 0, -25.0f, 8.0f, 0 },
};

global_variable SceneLayer layers3[] = 
{
    { 0, 0,-140.0f, 450.0f, SceneLayerFlag_AtInfinity },
    { 0, -60.0f, -100.0f, 250.0f, 0 },
    { 0, -23.0f, -15.0f, 70.0f, 0 },
    { 0, 0, -24.0f, 20.0f, SceneLayerFlag_CounterY },
};

global_variable SceneLayer layers4[] = 
{
    { 0, 0,-38.0f, 35.0f, 0 },
    { 0, 0.0f, -38.0f, 30.0f, SceneLayerFlag_Transient, 0.0f, 0.5f },
    { 0, 0.0f, -38.0f, 30.0f, SceneLayerFlag_Transient, 0.5f, 1.0f },
    { 15.0f, -30.0f, -13.0f, 15.0f, 0 },
    { 0, 3.0f, -25.0f, 10.0f, 0 },
};

#define INTRO_LAYER( n ) ArrayCount( layers##n ), n, layers##n
global_variable LayeredScene introShots[] = 
{
    { CUTSCENE_WARMUP_TIME, 0, 0, 0 },
    { 5.0f, INTRO_LAYER( 1 ), { 0.0f, 0.0f, 10.0f }, { -3.0f, -1.0f, 5.0f }, 0.6f },
    { 10.0f, INTRO_LAYER( 2 ), { 0.0f, 0.0f, 0.0f }, { 1.0f, -1.0f, -1.0f } },
    { 20.0f, INTRO_LAYER( 3 ), { 0.0f, 0.0f, 3.0f }, { 0.0f, -20.0f, -0.0f } },
    { 5.0f, INTRO_LAYER( 4 ), { 0.0f, 0.0f, 3.0f }, { 0.0f, 0.0f, -0.0f } }
};

global_variable Cutscene cutscenes[] =
{
    { ArrayCount( introShots ), introShots },
};

internal b32 RenderCutSceneAtTime( Assets* assets, RenderGroup* group, GameModeScene* cutscene, r32 cutsceneTime )
{
    b32 ended = false;
    b32 rendered = false;
    r32 tBase = 0.0f;
    
    Cutscene info = cutscenes[cutscene->ID];
    for( u32 shotIndex = 0; shotIndex < info.shotCount; ++shotIndex )
    {
        LayeredScene* shot = info.shots + shotIndex;
        
        r32 start = tBase;
        r32 end = start + shot->duration;
        
        if( ( cutsceneTime >= start ) && ( cutsceneTime < end ) )
        {
            r32 tNormal = Clamp01MapToRange( start, cutsceneTime, end );
            RenderLayeredScene( assets, group, shot, tNormal );
            rendered = true;
            break;
        }
        tBase = end;
    }
    
    if( !rendered && cutsceneTime > tBase )
    {
        ended = true;
    }
    return ended;
}

internal b32 CheckForMetaInput(GameState* gameState, PlatformInput* input)
{
    b32 result = false;
    if(Pressed(&input->mouseLeft))
    {
        result = true;
        PlayGame(gameState, input);
    }
    
    return result;
}


internal void PlayIntroCutscene( GameState* gameState)
{
    SetGameMode( gameState, GameMode_Cutscene );
    GameModeScene* result = PushStruct(&gameState->modePool, GameModeScene);
    
    result->t = 0;
    result->ID = CutsceneID_Intro;
    
    gameState->cutscene = result;
}


internal void PlayTitleScreen( GameState* gameState)
{
    SetGameMode( gameState, GameMode_TitleScreen );
    GameModeTitleScreen* result = PushStruct( &gameState->modePool, GameModeTitleScreen );
    
    result->t = 0;
    gameState->titleScreen = result;
}

internal b32 UpdateAndRenderTitleScreen( GameState* gameState, GameModeTitleScreen* title, RenderGroup* group, PlatformInput* input )
{
    b32 result = CheckForMetaInput( gameState, input );
    if( !result )
    {
        Clear( group, V4( 1.0f, 0.25f, 0.25f, 1.0f ) );
        if( title->t >= 10.0f )
        {
            PlayIntroCutscene( gameState);
        }
        else
        {
            title->t += input->timeToAdvance;
        }
    }
    return result;
}

internal b32 UpdateAndRenderCutscene( GameState* gameState, GameModeScene* scene, RenderGroup* group, PlatformInput* input )
{
    b32 result = CheckForMetaInput( gameState, input );
    if( !result )
    {
        m4x4 I = Identity();
        SetCameraTransform( group, false, 0.6f );
        
        RenderCutSceneAtTime( group->assets, 0, scene, scene->t + CUTSCENE_WARMUP_TIME );
        b32 ended = RenderCutSceneAtTime( group->assets, group, scene, scene->t );
        if( ended )
        {
            PlayTitleScreen( gameState);
        }
        else
        {
            scene->t += input->timeToAdvance;
        }
    }
    return result;
}