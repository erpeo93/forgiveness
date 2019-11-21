
STANDARD_ECS_JOB_SERVER(UpdateBrain)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    switch(brain->type)
    {
        case Brain_invalid:
        {
            
        } break;
        
		case Brain_Portal:
		{
            SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, def->P);
            if(IsValid(&playerQuery))
            {
                EntityID playerID = GetCurrent(&playerQuery);
                DefaultComponent* playerDef = GetComponent(server, playerID, DefaultComponent);
                r32 maxDistanceSq = Square(2.0f);
                
                r32 distanceSq = LengthSq(SubtractOnSameZChunk(playerDef->P, def->P));
                if(distanceSq < maxDistanceSq)
                {
                    if(!IsValidID(brain->ID))
                    {
                        AddEntityParams params = DefaultAddEntityParams();
                        params.targetBrainID = ID;
                        EntityRef type = EntityReference(server->assets, "default", "wolf");
                        def->flags = AddFlags(def->flags, EntityFlag_locked);
                        AddEntity(server, def->P, &server->entropy, type, params);
                    }
				}
                else
                {
                    if(IsValidID(brain->ID))
                    {
                        DeleteEntity(server, brain->ID);
                        brain->ID = {};
                    }
                }
            }
            else
            {
                if(IsValidID(brain->ID))
                {
                    DeleteEntity(server, brain->ID);
                    brain->ID = {};
                }
            }
		} break;
        
#if 0        
        case campfire_brain:
        {
            if(isonfire && enoughTimePassed)
            {
                setStatus(status_onfire);
            }
        } break;
#endif
        
    }
}
