internal void Idle(BrainComponent* brain)
{
    brain->currentCommand = {};
    brain->currentCommand.action = idle;
    brain->commandParameters.acceleration = V3(0, 0, 0);
}

internal void Move(BrainComponent* brain, Vec3 direction)
{
    brain->currentCommand = {};
    brain->currentCommand.action = move;
    brain->commandParameters.acceleration = direction;
}


internal void Attack(BrainComponent* brain, EntityID targetID)
{
    brain->currentCommand = {};
    brain->currentCommand.action = attack;
    brain->currentCommand.targetID = targetID;
    brain->commandParameters = {};
}



STANDARD_ECS_JOB_SERVER(UpdateBrain)
{
    DefaultComponent* def = GetComponent(server, ID, DefaultComponent);
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    MiscComponent* misc = GetComponent(server, ID, MiscComponent);
    
    switch(brain->type)
    {
        case Brain_invalid:
        {
            
        } break;
        
		case Brain_Player:
		{
            PlayerComponent* player = GetComponent(server, ID, PlayerComponent);
            Assert(player);
			brain->currentCommand = player->requestCommand;
            brain->inventoryCommand = player->inventoryCommand;
            brain->commandParameters = player->commandParameters;
            player->inventoryCommand = {};
		} break;	
        
        case Brain_MoveLeft:
        {
            brain->currentCommand = {};
            brain->currentCommand.action = move;
            brain->commandParameters.acceleration = V3(-1, 0, 0);
        } break;
        
        case Brain_MoveRight:
        {
            brain->currentCommand = {};
            brain->currentCommand.action = move;
            brain->commandParameters.acceleration = V3(1, 0, 0);
        } break;
        
		case Brain_Portal:
		{
            SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, def->P);
            EntityID playerID = GetCurrent(&playerQuery);
            if(IsValidID(playerID))
            {
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
                        AddEntityFlags(def, EntityFlag_locked);
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
        
        case Brain_AttackPlayersDumb:
        {
            SpatialPartitionQuery playerQuery = QuerySpatialPartitionAtPoint(&server->playerPartition, def->P);
            EntityID playerID = GetCurrent(&playerQuery);
            if(IsValidID(playerID))
            {
                DefaultComponent* playerDef = GetComponent(server, playerID, DefaultComponent);
                InteractionComponent* playerInteraction = GetComponent(server, playerID, InteractionComponent);
                
                Vec3 toPlayer = SubtractOnSameZChunk(playerDef->P, def->P);
                r32 distanceSq = LengthSq(toPlayer);
                r32 targetTime;
                
                u16 oldAction = brain->currentCommand.action;
                if(ActionIsPossibleAtDistance(playerInteraction, attack, oldAction, distanceSq, &targetTime, misc))
                {
                    Attack(brain, playerID);
                }
                else
                {
                    if(ActionIsPossible(playerInteraction, attack))
                    {
                        Move(brain, toPlayer);
                    }
                    else
                    {
                        Idle(brain);
                    }
                }
            }
            else
            {
                Idle(brain);
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
    
    if(HasComponent(ID, ActionComponent))
    {
        DispatchCommand(server, ID, &brain->currentCommand, &brain->commandParameters, elapsedTime, true);
        DispatchCommand(server, ID, &brain->inventoryCommand, &brain->commandParameters, elapsedTime, false);
    }
}
