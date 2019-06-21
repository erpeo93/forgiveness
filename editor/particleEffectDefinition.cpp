internal void ImportParticleEffectDefinitionTab(TaxonomySlot* slot, EditorElement* root)
{
    if(slot->particleEffect)
    {
        TAXTABLE_DEALLOC(slot->particleEffect, ParticleEffectDefinition);
    }
    TAXTABLE_ALLOC(slot->particleEffect, ParticleEffectDefinition);
    
    ParticleEffectDefinition* definition = slot->particleEffect;
    
    definition->dAngleSineUpdaters = ElemR32(root, "dAngleSineUpdaters");
    
    EditorElement* emitterEl = GetElement(root, "emitter");
    
    ParticleEmitter* emitter = &definition->emitter;
    emitter->type = ParticleEmitter_Standard;
    
    emitter->particlesPerSec = ElemR32(emitterEl, "particlesPerSec");
    emitter->lifeTime = ElemR32(emitterEl, "lifeTime");
    emitter->lifeTimeV = ElemR32(emitterEl, "lifeTimeV");
    
    emitter->startPV =StructV3(emitterEl, "startPV");
    
    emitter->dP = StructV3(emitterEl, "dP");
    emitter->dPV = StructV3(emitterEl, "dPV");
    
    emitter->C = ColorV4(emitterEl, "color");
    emitter->CV = ColorV4(emitterEl, "colorV");
    
    emitter->dC = ColorV4(emitterEl, "colorVelocity");
    emitter->dCV = ColorV4(emitterEl, "colorVelocityV");
    
    emitter->angle = ElemR32(emitterEl, "angle");
    emitter->angleV = ElemR32(emitterEl, "angleV");
    
    emitter->scaleX = ElemR32(emitterEl, "scaleX");
    emitter->scaleXV = ElemR32(emitterEl, "scaleXV");
    
    emitter->scaleY = ElemR32(emitterEl, "scaleY");
    emitter->scaleYV = ElemR32(emitterEl, "scaleYV");
    
    
    
    
    EditorElement* phases = GetList(root, "phases"); 
    while(phases)
    {
        if(definition->phaseCount < ArrayCount(definition->phases))
        {
            ParticlePhase* phase = definition->phases + definition->phaseCount++;
            phase->ttlMax = ElemR32(phases, "ttlMax");
            phase->ttlMin = ElemR32(phases, "ttlMin");
            
            ParticleUpdater* updater = &phase->updater;
            
            updater->type = (ParticleUpdaterType) GetValuePreprocessor(ParticleUpdaterType, GetValue(phases, "particleUpdType"));
            
            updater->bitmapID = {};
            updater->particleHashID = StringHash(GetValue(phases, "particleName"));
            updater->startDrawingFollowingBitmapAt = ElemR32(phases, "startDrawingFollowingPhaseAt");
            updater->ddP = ToV3_4x(StructV3(phases, "acceleration"));
            updater->UpVector = {};
            updater->ddC = ToV4_4x(ColorV4(phases, "colorAcceleration"));
            
            updater->dScaleX = MMSetExpr(ElemR32(phases, "dScaleX"));
            updater->dScaleY = MMSetExpr(ElemR32(phases, "dScaleY"));
            updater->dAngle = MMSetExpr(ElemR32(phases, "dAngle"));
            
            r32 radiants = DegToRad(180.0f * Max(1, ElemU32(phases, "totalPITimes")));
            updater->totalRadiants = MMSetExpr(radiants);
            
            updater->destPType = (ParticleUpdaterEndPosition) GetValuePreprocessor(ParticleUpdaterEndPosition, GetValue(phases, "endPositionType"));
            updater->startOffset = StructV3(phases, "startOffset");
            updater->endOffset = StructV3(phases, "endOffset");
            
            updater->sineSubdivisions = ElemU32(phases, "sineSubdivisions");
        }
        
        phases = phases->next;
    }
}