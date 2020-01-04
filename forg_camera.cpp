inline Vec3 GetRelativeP(GameModeWorld* worldMode, UniversePos P)
{
    Vec3 result = SubtractOnSameZChunk(P, worldMode->player.universeP);
    return result;
}

inline Vec3 GetRelativeP(GameModeWorld* worldMode, BaseComponent* base, EntityAnimationParams params)
{
    Vec3 result = GetRelativeP(worldMode, base->universeP) + params.offsetAccumulated + params.offsetComputed;
    return result;
}

inline Vec3 GetRelativeP(GameModeWorld* worldMode, EntityID ID)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    EntityAnimationParams params = GetEntityAnimationParams(worldMode, ID);
    
    Vec3 result = GetRelativeP(worldMode, base, params);
    return result;
}

inline void MoveCameraTowards(GameModeWorld* worldMode, EntityID ID, r32 cameraSpeed, Vec2 cameraWorldOffset, r32 zoomCoeff)
{
    BaseComponent* base = GetComponent(worldMode, ID, BaseComponent);
    cameraWorldOffset += GetRelativeP(worldMode, ID).xy;
    worldMode->destCameraEntityOffset = 0.5f * V2(GetDim(base->bounds).x, GetDim(base->bounds).z);
    worldMode->destCameraWorldOffset = V3(cameraWorldOffset, worldMode->defaultCameraZ / zoomCoeff);
    worldMode->destCameraWorldOffset += worldMode->editorCameraOffset;
    worldMode->cameraSpeed = cameraSpeed;
}



internal void ResetGameCamera(GameModeWorld* worldMode, RenderGroup* group)
{
    Vec2 finalXYOffset = worldMode->cameraWorldOffset.xy;
    
    m4x4 cameraO = ZRotation(cameraOrbit) * XRotation(cameraPitch);
    magicLateralVector = GetColumn(cameraO, 0);
    magicUpVector = GetColumn(cameraO, 1);
    
    Vec3 cameraOffsetFinal = cameraO * V3(0, 0, worldMode->cameraWorldOffset.z + cameraDolly) + V3(finalXYOffset, 0);
    
    SetCameraBasics(group, 0, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal, worldMode->cameraEntityOffset);
    
    if(worldMode->useDebugCamera)
    {
        cameraO = ZRotation(worldMode->debugCameraOrbit) * XRotation(worldMode->debugCameraPitch);
        cameraOffsetFinal = cameraO * (V3(worldMode->cameraEntityOffset, worldMode->cameraWorldOffset.z + worldMode->debugCameraDolly)) + V3(finalXYOffset, 0);
        SetCameraBasics(group, Camera_Debug, 3.5f, GetColumn(cameraO, 0), GetColumn(cameraO, 1), GetColumn(cameraO, 2), cameraOffsetFinal, worldMode->cameraEntityOffset);
    }
}

internal void UpdateGameCamera(GameModeWorld* worldMode, PlatformInput* input)
{
    worldMode->cameraWorldOffset += worldMode->cameraSpeed * input->timeToAdvance * (worldMode->destCameraWorldOffset - worldMode->cameraWorldOffset);
    worldMode->cameraEntityOffset += worldMode->cameraSpeed * input->timeToAdvance * (worldMode->destCameraEntityOffset - worldMode->cameraEntityOffset);
    
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
	if(input->altDown && Pressed(&input->mouseRight))
    {
        worldMode->useDebugCamera = !worldMode->useDebugCamera;
    }
#endif
}