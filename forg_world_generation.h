#pragma once

global_variable int perlin_[] = { 151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180, 
    
    151,160,137,91,90,15,
    131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
    190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,
    88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,
    77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,55,46,245,40,244,
    102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
    135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,
    5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,
    223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167, 43,172,9,
    129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,97,228,
    251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
    49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
    138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

inline r32 fade(r32 t) { return t * t * t * (t * (t * 6 - 15) + 10); }

inline r32 grad(int hash, r32 x, r32 y, r32 z) {
    int h = hash & 15;                      // CONVERT LO 4 BITS OF HASH CODE
    r32 u = h<8 ? x : y,                 // INTO 12 GRADIENT DIRECTIONS.
    v = h<4 ? y : h==12||h==14 ? x : z;
    return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

inline r32 lerp(r32 t, r32 a, r32 b) { return a + t * (b - a); }

inline r32 noise(r32 x, r32 y, r32 z, u32 seed)
{
    int X = ((int)floor(x) + seed) & 255,                  // FIND UNIT CUBE THAT
    Y = ((int)floor(y) + seed) & 255,                  // CONTAINS POINT.
    Z = ((int)floor(z) + seed) & 255;
    x -= floorf(x);                                // FIND RELATIVE X,Y,Z
    y -= floorf(y);                                // OF POINT IN CUBE.
    z -= floorf(z);
    r32 u = fade(x),                                // COMPUTE FADE CURVES
    v = fade(y),                                // FOR EACH OF X,Y,Z.
    w = fade(z);
    
    int A = perlin_[X  ]+Y, 
    AA = perlin_[A]+Z, 
    AB = perlin_[A+1]+Z,      // HASH COORDINATES OF
    B = perlin_[X+1]+Y, 
    BA = perlin_[B]+Z, 
    BB = perlin_[B+1]+Z;      // THE 8 CUBE CORNERS,
    
    return lerp(w, lerp(v, lerp(u, grad(perlin_[AA  ], x  , y  , z   ),  // AND ADD
                                grad(perlin_[BA  ], x-1, y  , z   )), // BLENDED
                        lerp(u, grad(perlin_[AB  ], x  , y-1, z   ),  // RESULTS
                             grad(perlin_[BB  ], x-1, y-1, z   ))),// FROM  8
                lerp(v, lerp(u, grad(perlin_[AA+1], x  , y  , z-1 ),  // CORNERS
                             grad(perlin_[BA+1], x-1, y  , z-1 )), // OF CUBE
                     lerp(u, grad(perlin_[AB+1], x  , y-1, z-1 ),
                          grad(perlin_[BB+1], x-1, y-1, z-1 ))));
}

struct GenerationMinMax
{
    r32 min;
    r32 max;
};

enum GenerationBucketType
{
    Bucket_Noise,
    Bucket_MinMax,
    Bucket_Fixed,
};


struct GenerationBucket
{
    r32 referencePoint;
    
    union
    {
        NoiseParams params;
        GenerationMinMax minMax;
        r32 fixed;
    };
};

struct Selector
{
    GenerationBucketType type;
    
    u32 bucketCount;
    GenerationBucket buckets[16];
};

#define WATER_LEVEL 0.2f
struct BiomePyramid
{
    // NOTE(Leonardo): vertical!
    Selector drySelector;
    
    // NOTE(Leonardo): horizontal, one for every "slice"
    u32 rowCount;
    Selector temperatureSelectors[4];
};

inline NoiseParams NoisePar(r32 frequency, u32 octaves, r32 offset, r32 amplitude,r32 persistance = 0.5f)
{
    NoiseParams result = {};
    result.frequency = frequency;
    result.octaves = octaves;
    result.persistance = persistance;
    result.offset = offset;
    result.amplitude = amplitude;
    return result;
}

inline GenerationMinMax MinMax(r32 min, r32 max)
{
    GenerationMinMax result = {};
    result.min = min;
    result.max = max;
    
    return result;
}

struct TaxonomyAssociation
{
    r32 weight;
    u32 taxonomy;
    
    union
    {
        TaxonomyAssociation* next;
        TaxonomyAssociation* nextFree;
    };
};

struct TaxonomyTileAssociations
{
    u32 taxonomy;
    
    r32 totalWeight;
    TaxonomyAssociation* firstAssociation;
    
    union
    {
        TaxonomyTileAssociations* next;
        TaxonomyTileAssociations* nextFree;
    };
};

struct WorldGenerator
{
    NoiseParams landscapeNoise;
    Selector landscapeSelect;
    
    NoiseParams temperatureNoise;
    Selector temperatureSelect;
    
    NoiseParams precipitationNoise;
    
    
    NoiseParams elevationNoise;
    r32 elevationPower;
    r32 beachThreesold;
    u32 beachTaxonomy;
    
    BiomePyramid biomePyramid;
    
    TaxonomyTileAssociations* firstAssociation;
    
    WorldGenerator* nextFree;
};