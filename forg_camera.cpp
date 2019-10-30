inline Vec3 GetRelativeP(GameModeWorld* worldMode, BaseComponent* base)
{
    Vec3 result = SubtractOnSameZChunk(base->universeP, worldMode->player.universeP);
    return result;
}

inline void MoveTowards_(GameModeWorld* worldMode, Vec2 cameraWorldOffset, Vec2 cameraEntityOffset, r32 zoomCoeff = 1.0f)
{
}

inline void MoveCameraTowards(GameModeWorld* worldMode, BaseComponent* base, r32 cameraSpeed, Vec2 cameraWorldOffset = V2(0, 0), Vec2 cameraEntityOffset = V2(0, 0), r32 zoomCoeff = 1.0f)
{
    cameraWorldOffset += GetRelativeP(worldMode, base).xy;
    worldMode->destCameraEntityOffset = cameraEntityOffset;
    worldMode->destCameraEntityOffset += 0.5f * GetDim(base->worldBounds).xy;
    
    
    worldMode->destCameraWorldOffset = V3(cameraWorldOffset, worldMode->defaultCameraZ / zoomCoeff);
    worldMode->destCameraWorldOffset += worldMode->editorCameraOffset;
    worldMode->cameraSpeed = cameraSpeed;
}

internal void SetupGameCamera(GameModeWorld* worldMode, RenderGroup* group, PlatformInput* input)
{
#if FORGIVENESS_INTERNAL
    Vec2 dMouseP = worldMode->deltaMouseP;
    if(input->altDown && IsDown(&input->mouseLeft))
    {
        r32 rotationSpeed = 0.001f * PI32;
        worldMode->debugCameraOrbit += rotationSpeed * dMouseP.x;
        worldMode->debugCameraPitch += rotationSpeed * dMouseP.y;
    }
    else if(input->altDown && IsDown(&input->mouseRight))
    {
        r32 zoomSpeed = (worldMode->debugCameraDolly) * 0.01f;
        worldMode->debugCameraDolly -= zoomSpeed * dMouseP.y;
    }
#endif
    
    worldMode->cameraPitch = 0.32f * PI32;
    worldMode->cameraDolly = 0.0f;
    worldMode->cameraOrbit = 0.0f;
    
    Vec2 finalXYOffset = worldMode->cameraWorldOffset.xy;
    
    m4x4 cameraO = ZRotation(worldMode->cameraOrbit) * XRotation(worldMode->cameraPitch);
    Vec3 cameraOffsetFinal = cameraO * V3(worldMode->cameraEntityOffset, worldMode->cameraWorldOffset.z + worldMode->cameraDolly) + V3(finalXYOffset, 0);
    
    SetCameraTransform(group, 0, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal, worldMode->cameraEntityOffset);
    
    if(input->altDown && Pressed(&input->mouseRight))
    {
        worldMode->useDebugCamera = !worldMode->useDebugCamera;
    }
    
    if(worldMode->useDebugCamera)
    {
        cameraO = ZRotation(worldMode->debugCameraOrbit) * XRotation(worldMode->debugCameraPitch);
        cameraOffsetFinal = cameraO * (V3(worldMode->cameraEntityOffset, worldMode->cameraWorldOffset.z + worldMode->debugCameraDolly)) + V3(finalXYOffset, 0);
        
        SetCameraTransform(group, Camera_Debug, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal, worldMode->cameraEntityOffset);
    }
}


internal void UpdateGameCamera(GameModeWorld* worldMode, PlatformInput* input)
{
    worldMode->cameraWorldOffset += worldMode->cameraSpeed * input->timeToAdvance * (worldMode->destCameraWorldOffset - worldMode->cameraWorldOffset);
    
    worldMode->cameraEntityOffset += worldMode->cameraSpeed * input->timeToAdvance * (worldMode->destCameraEntityOffset - worldMode->cameraEntityOffset);
}