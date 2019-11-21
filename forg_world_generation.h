#pragma once
introspection() struct NoiseParams
{
    r32 frequency MetaDefault("1");
    u32 octaves MetaDefault("1");
    r32 persistance;
    
    r32 min MetaDefault("0");
    r32 max MetaDefault("1");
};

introspection() struct NoiseBucket
{
    AssetLabel name;
    r32 referencePoint;
    NoiseParams params;
};

introspection() struct PropertyBucket
{
    AssetLabel name;
    r32 referencePoint;
    GameProperty property;
};




introspection() struct NoiseSelector
{
    ArrayCounter bucketCount MetaCounter(buckets);
    NoiseBucket* buckets;
};

introspection() struct PropertySelector
{
    ArrayCounter bucketCount MetaCounter(buckets);
    PropertyBucket* buckets;
};

introspection() struct DrynessSelector
{
    NoiseSelector drynessSelector;
    ArrayCounter rowCount MetaCounter(temperatureSelectors);
    PropertySelector* temperatureSelectors;
};

introspection() struct BiomePyramid
{
    // NOTE(Leonardo): vertical!
    NoiseSelector darknessSelector;
    
    ArrayCounter drynessCount MetaCounter(drynessSelectors);
    DrynessSelector* drynessSelectors;
};

introspection() struct ZSlice
{
    r32 referenceZ;
    
    NoiseParams precipitationNoise;
    NoiseParams darknessNoise;
};

introspection() struct world_generator
{
    NoiseParams landscapeNoise;
    NoiseSelector landscapeSelect;
    
    NoiseParams temperatureNoise;
    NoiseSelector temperatureSelect;
    
    NoiseParams elevationNoise;
    r32 elevationPower MetaDefault("1");
    r32 elevationNormOffset MetaDefault("1");
    
    r32 waterSafetyMargin MetaDefault("0.05f");
    BiomePyramid biomePyramid;
    
    u32 maxDeepness MetaDefault("1");
    
    ArrayCounter zSlicesCount MetaCounter(zSlices);
    ZSlice* zSlices;
};

inline NoiseParams NoisePar(r32 frequency, u32 octaves, r32 min, r32 max,r32 persistance = 0.5f)
{
    NoiseParams result = {};
    result.frequency = frequency;
    result.octaves = octaves;
    result.persistance = persistance;
    result.min = min;
    result.max = max;
    return result;
}

introspection() struct SpawnerEntity
{
    EntityRef type;
    Vec2 maxClusterOffset;
    Vec2 maxOffset;
    r32 minClusterDistance MetaDefault("1");
    r32 minEntityDistance MetaDefault("1");
    i32 clusterCount MetaDefault("1");
    i32 clusterCountV MetaDefault("0");
    i32 count MetaDefault("1");
    i32 countV;
    
    b32 attachedBrainEntity;
    EntityRef attachedBrainType;
};

introspection() struct SpawnerOption
{
    r32 weight MetaDefault("1.0f");
    ArrayCounter entityCount MetaCounter(entities);
    SpawnerEntity* entities;
};

introspection() struct Spawner
{
    r32 time MetaUneditable();
    r32 targetTime;
    r32 cellDim MetaDefault("1.0f");
    r32 percentageOfStartingCells MetaDefault("1.0f");
    
    ArrayCounter optionCount MetaCounter(options);
    SpawnerOption* options;
};
