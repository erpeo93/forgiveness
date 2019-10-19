#pragma once
struct EquipmentComponent
{
    EntityID IDs[Count_equipmentSlot];
};

struct EquipmentMapping
{
    u64 nameHash;
    EntityID ID;
};

struct EquipmentMappingComponent
{
    EquipmentMapping mappings[Count_equipmentSlot];
};

enum EntityFlags
{
    EntityFlag_equipment = (1 << 0),
};