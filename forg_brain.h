#pragma once

struct BrainComponent
{
	GameCommand currentCommand;
    GameCommand inventoryCommand;
    CommandParameters commandParameters;
    b32 commandCompleted;
    
    u16 type;
    EntityID ID;
};