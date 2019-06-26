internal void ImportBoltDefinitionTab(TaxonomySlot* slot, EditorElement* root)
{
    if(slot->boltEffectDefinition)
    {
        TAXTABLE_DEALLOC(slot->boltEffectDefinition, BoltDefinition);
    }
    TAXTABLE_ALLOC(slot->boltEffectDefinition, BoltDefinition);
    
    BoltDefinition* definition = slot->boltEffectDefinition;
    
    definition->animationTick = ElemR32(root, "animationTick");
    definition->ttl = ElemR32(root, "ttl");
    definition->ttlV = ElemR32(root, "ttlV");
    definition->fadeinTime = ElemR32(root, "fadeinTime");
    definition->fadeoutTime = ElemR32(root, "fadeoutTime");
    definition->color = ColorV4(root, "color");
    definition->thickness = ElemR32(root, "thickness");
    definition->magnitudoStructure = ElemR32(root, "magnitudoStructure");
    definition->magnitudoAnimation = ElemR32(root, "magnitudoAnimation");
    definition->subdivisions = ElemU32(root, "subdivisions");
    definition->subdivisionsV = ElemU32(root, "subdivisionsV");
    
    definition->lightColor = ColorV4(root, "lightColor").rgb;
    definition->lightIntensity = ElemR32(root, "lightIntensity");
    definition->lightStartTime = ElemR32(root, "lightStartTime");
    definition->lightEndTime = ElemR32(root, "lightEndTime");
    
    definition->trailerSoundEffect = StringHash(GetValue(root, "trailerSoundEvent"));
}