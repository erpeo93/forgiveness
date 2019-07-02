inline void AddTileBucket(Selector* band, char* tileName, r32 temperature)
{
    TaxonomySlot* tileSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, tileName);
    if(tileSlot)
    {
        AddBucket(band, temperature, tileSlot->taxonomy);
    }
    else
    {
        EditorErrorLog(tileName);
    }
}

inline r32 ParseTaxonomyAssociation(TaxonomyAssociation* dest, EditorElement* root)
{
    r32 weight = ToR32(GetValue(root, "weight"));
    r32 radious = ElemR32(root, "radious");
    
    dest->radious = radious;
    dest->weight = weight;
    
    EditorElement* spawnTaxonomies = GetList(root, "taxonomies");
    
    while(spawnTaxonomies)
    {
        char* taxonomyName = GetValue(spawnTaxonomies, "taxonomyName");
        TaxonomySlot* taxonomySlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomyName);
        if(taxonomySlot)
        {
            SpawnTaxonomy* spawn;
            TAXTABLE_ALLOC(spawn, SpawnTaxonomy);
            
            u32 counter = ToU32(GetValue(spawnTaxonomies, "counter"), 1);
            spawn->taxonomy = taxonomySlot->taxonomy;
            spawn->counter = counter;
            
            FREELIST_INSERT(spawn, dest->firstTaxonomy);
        }
        else
        {
            EditorErrorLog(taxonomyName);
        }
        
        spawnTaxonomies = spawnTaxonomies->next;
    }
    
    return weight;
}

internal void ImportGeneratorParamsTab(TaxonomySlot* slot, EditorElement* root)
{
    if(slot->generatorDefinition)
    {
        for(TaxonomyTileAssociations* toFree = slot->generatorDefinition->firstAssociation; toFree;)
        {
            TaxonomyTileAssociations* next = toFree->next;
            
            
            for(TaxonomyAssociation* assToFree = toFree->firstAssociation; assToFree;)
            {
                FREELIST_FREE(assToFree->firstTaxonomy, SpawnTaxonomy, taxTable_->firstFreeSpawnTaxonomy);
                
                TaxonomyAssociation* assNext = assToFree->next;
                TAXTABLE_DEALLOC(assToFree, TaxonomyAssociation);
                assToFree = assNext;
            }
            
            TAXTABLE_DEALLOC(toFree, TaxonomyTileAssociations);
            
            toFree = next;
        }
        
        for(TaxonomyTileTimerSpawn* toFree = slot->generatorDefinition->firstTimerSpawn; toFree;)
        {
            TaxonomyTileTimerSpawn* next = toFree->next;
            
            
            for(TaxonomyAssociation* assToFree = toFree->firstAssociation; assToFree;)
            {
                FREELIST_FREE(assToFree->firstTaxonomy, SpawnTaxonomy, taxTable_->firstFreeSpawnTaxonomy);
                
                TaxonomyAssociation* assNext = assToFree->next;
                TAXTABLE_DEALLOC(assToFree, TaxonomyAssociation);
                assToFree = assNext;
            }
            
            TAXTABLE_DEALLOC(toFree, TaxonomyTileTimerSpawn);
            
            toFree = next;
        }
        
        
        TAXTABLE_DEALLOC(slot->generatorDefinition, WorldGeneratorDefinition);
        slot->generatorDefinition = 0;
    }
    TAXTABLE_ALLOC(slot->generatorDefinition, WorldGeneratorDefinition);
    WorldGeneratorDefinition* generator = slot->generatorDefinition;
    
    
    generator->landscapeNoise = ParseNoiseParams(GetStruct(root, "landscapeNoise"));
    generator->temperatureNoise = ParseNoiseParams(GetStruct(root, "temperatureNoise"));
    generator->precipitationNoise = ParseNoiseParams(GetStruct(root, "precipitationNoise"));
    generator->elevationNoise = ParseNoiseParams(GetStruct(root, "elevationNoise"));
    generator->elevationPower = ToR32(GetValue(root, "elevationPower"), 1.0f);
    generator->beachThreesold = ToR32(GetValue(root, "beachThreesold"), 0.05f);
    
    generator->beachTaxonomy = 0;
    char* beachTaxonomy = GetValue(root, "beachTaxonomy");
    if(beachTaxonomy)
    {
        TaxonomySlot* beachSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, beachTaxonomy);
        if(beachSlot)
        {
            generator->beachTaxonomy = beachSlot->taxonomy;
        }
    }
    
    generator->landscapeSelect = {};
    generator->temperatureSelect = {};
    EditorElement* landscape = GetList(root, "landscapes");
    while(landscape)
    {
        r32 threesold = ToR32(GetValue(landscape, "threesold"));
        
        NoiseParams noise = ParseNoiseParams(GetStruct(landscape, "noiseParams"));
        AddBucket(&generator->landscapeSelect, threesold, noise);
        
        
        r32 minTemperature = ToR32(GetValue(landscape, "minTemperature"));
        r32 maxTemperature = ToR32(GetValue(landscape, "maxTemperature"));
        AddBucket(&generator->temperatureSelect, threesold,MinMax(minTemperature, maxTemperature)); 
        
        landscape = landscape->next;
    }
    
    generator->biomePyramid = {};
    
    EditorElement* precipitationBand = GetList(root, "precipitationBands");
    while(precipitationBand)
    {
        r32 threesold = ToR32(GetValue(precipitationBand, "threesold"));
        Selector* band = AddSelectorForDryness(&generator->biomePyramid, threesold);
        
        EditorElement* tiles = GetList(precipitationBand, "tileTypes");
        while(tiles)
        {
            r32 temperature = ToR32(GetValue(tiles, "temperature"));
            
            AddTileBucket(band, tiles->name, temperature);
            
            tiles = tiles->next;
        }
        
        precipitationBand = precipitationBand->next;
    }
    
    
    EditorElement* tileAssociations = GetList(root, "tileAssociations");
    while(tileAssociations)
    {
        char* tileName = GetValue(tileAssociations, "tileType");
        TaxonomySlot* tileSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, tileName);
        
        if(tileSlot)
        {
            TaxonomyTileAssociations* tileAssoc;
            TAXTABLE_ALLOC(tileAssoc, TaxonomyTileAssociations);
            
            tileAssoc->taxonomy = tileSlot->taxonomy;
            tileAssoc->totalWeight = 0;
            tileAssoc->firstAssociation = 0;
            
            
            EditorElement* taxonomyAssociations = GetList(tileAssociations, "taxonomies");
            while(taxonomyAssociations)
            {
                TaxonomyAssociation* ass;
                TAXTABLE_ALLOC(ass, TaxonomyAssociation);
                r32 weight = ParseTaxonomyAssociation(ass, taxonomyAssociations);
                tileAssoc->totalWeight += weight;
                FREELIST_INSERT(ass, tileAssoc->firstAssociation);
                
                taxonomyAssociations = taxonomyAssociations->next;
            }
            FREELIST_INSERT(tileAssoc, generator->firstAssociation);
            
        }
        else
        {
            EditorErrorLog(tileName);
        }
        
        tileAssociations = tileAssociations->next;
    }
    
    EditorElement* spawnTimers = GetList(root, "spawnTimers");
    
    while(spawnTimers)
    {
        char* tileName = GetValue(spawnTimers, "tileType");
        TaxonomySlot* tileSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, tileName);
        
        if(tileSlot)
        {
            TaxonomyTileTimerSpawn* tileSpawn;
            TAXTABLE_ALLOC(tileSpawn, TaxonomyTileTimerSpawn);
            
            tileSpawn->taxonomy = tileSlot->taxonomy;
            tileSpawn->timer = 0;
            tileSpawn->destTimer = ElemR32(spawnTimers, "timer");
            tileSpawn->totalWeight = 0;
            tileSpawn->firstAssociation = 0;
            
            
            EditorElement* taxonomyAssociations = GetList(spawnTimers, "taxonomies");
            while(taxonomyAssociations)
            {
                TaxonomyAssociation* ass;
                TAXTABLE_ALLOC(ass, TaxonomyAssociation);
                r32 weight = ParseTaxonomyAssociation(ass, taxonomyAssociations);
                tileSpawn->totalWeight += weight;
                FREELIST_INSERT(ass, tileSpawn->firstAssociation);
                
                taxonomyAssociations = taxonomyAssociations->next;
            }
            FREELIST_INSERT(tileSpawn, generator->firstTimerSpawn);
            
        }
        else
        {
            EditorErrorLog(tileName);
        }
        
        spawnTimers = spawnTimers->next;
    }
}