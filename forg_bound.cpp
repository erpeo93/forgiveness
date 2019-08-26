inline void GetPhysicalProperties(TaxonomyTable* taxTable, u32 taxonomy, u64 identifier, ForgBoundType* type, Rect3* bounds, r32 generationIntensity)
{
    TaxonomySlot* boundSlot = GetSlotForTaxonomy(taxTable, taxonomy);
    if(IsRock(taxTable, taxonomy))
    {
        if(boundSlot->rockDefinition)
        {
            RandomSequence rockSeq = Seed((u32) identifier);
            if(boundSlot->rockDefinition->collides)
            {
                *type = ForgBound_Standard;
            }
            else
            {
                *type = ForgBound_NonPhysical;
            }
            
            Vec3 rockDim = GetRockDim(boundSlot->rockDefinition, &rockSeq);
            *bounds = RectCenterDim(V3(0, 0, 0), rockDim);
        }
    }
    else if(IsPlant(taxTable, taxonomy))
    {
        if(boundSlot->plantDefinition)
        {
            if(boundSlot->plantDefinition->collides)
            {
                *type = ForgBound_Standard;
            }
            else
            {
                *type = ForgBound_NonPhysical;
            }
            
            r32 trunkRadious = Max(0.2f, GetPlantStandardTrunkRadious(boundSlot->plantDefinition));
            r32 trunkLength = GetPlantStandardTrunkLength(boundSlot->plantDefinition);
            
            Vec3 min = V3(-trunkRadious, -trunkRadious, 0);
            Vec3 max = V3(trunkRadious, trunkRadious, trunkLength);
            
            *bounds = RectMinMax(min, max);
        }
    }
    else
    {
        while(boundSlot->taxonomy)
        {
            if(boundSlot->boundType)
            {
                *type = boundSlot->boundType;
                Rect3 slotBound = boundSlot->physicalBounds;
                if(boundSlot->scaleDimBasedOnIntensity)
                {
                    Assert(Normalized(generationIntensity));
                    r32 coeff = UnilateralToBilateral(generationIntensity) * boundSlot->scaleDimGenCoeffV;
                    slotBound = Scale(slotBound, 1.0f + coeff);
                }
                
                *bounds = slotBound;
                break;
            }
            
            boundSlot = GetParentSlot(taxTable, boundSlot);
        }
    }
}
