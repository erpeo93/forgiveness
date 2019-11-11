
STANDARD_ECS_JOB_SERVER(UpdateBrain)
{
    BrainComponent* brain = GetComponent(server, ID, BrainComponent);
    switch(brain->type)
    {
        case Brain_invalid:
        {
            
        } break;
        
        case Brain_test:
        {
            EntityRef sword = EntityReference(server->assets, "default", "sword");
            
            ContainerComponent* container = GetComponent(server, ID, ContainerComponent);
            if(container)
            {
                for(u32 slotIndex = 0; slotIndex < ArrayCount(container->storedObjects); ++slotIndex)
                {
                    InventorySlot* slot = container->storedObjects + slotIndex;
                    if(IsValidID(slot->ID))
                    {
                        PhysicComponent* physic = GetComponent(server, slot->ID, PhysicComponent);
                        if(AreEqual(physic->definitionID, sword))
                        {
                            slot->ID = {};
                        }
                    }
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
