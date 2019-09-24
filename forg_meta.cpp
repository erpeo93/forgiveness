internal void LoadMetaData()
{
    META_DEFAULT_VALUES_CPP_SUCKS();
    META_HANDLE_ADD_TO_DEFINITION_HASH();
    AddToMetaDefinitions(Vec2, fieldDefinitionOfVec2);
    AddToMetaDefinitions(Vec3, fieldDefinitionOfVec3);
    AddToMetaDefinitions(Vec4, fieldDefinitionOfVec4);
    
    AddToMetaProperties(Invalid, MetaProperties_Invalid);
    META_PROPERTIES_ADD();
    META_ASSET_PROPERTIES_STRINGS();
    
    META_ARCHETYPES_BOTH();
    
#ifdef FORG_SERVER
    META_ARCHETYPES_SERVER();
#else
    META_ARCHETYPES_CLIENT();
#endif
}

