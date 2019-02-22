NecessaryAPI:
DescribeQuest();
GenQuest();
AdvanceQuestStatus();


USE CASE:
A farmer break its tool. A stimulus is generated and perceived.
The farmer doesnt know how to craft one, neither knows a place where to find one.
So he decides to create a Quest:
GenQuest( BringMeObject, Tool );
The quest is generated and now the npc goes to the ill to put an announcment.
But the tool is too important for the farmer: alongside creating the quest, the farmer decides to go to a person that _could_ give him the recipe for the object.
The farmer is able to get the recipe and build the tool before any player finds one: the quest is "invalidated"
So we need the ability to "bild" certain events to the quest: "if the farmer founds a tool, invalidate the quest"

Another clearer example: a quest "destroy city" is generated.
As soon as the information that the city has been destroyed arrives at the npc, the quest is "closed" in the sense that from now on no one can take it.

In most cases, the player can't lie to the npc, saying that he did destroy the city while in reality it didn't. But when the quest is generated, a random choice determines what is the minimum level of 
oratory skill that allows the player to lie.
Otherwise the player can't tell the npc something that he didn't really did. 


struct Quest
{
NOTE: the npc can still forget stuff while the quest is assigned!
	MemoryNode* nodes[16];
	u64 assignedTo;
	NecessaryThingsToCompleteTheQuest;
	Reward;
};

void GenQuest()
{
	b32 generated = false;
	while( !generated)
	{
		generated = true;
		QuestTemplate* template = PickTemplate();
		for( u32 reqCount = 0; reqCount < template->reqCount; ++reqCount )
		{
			QuestReq* req = template->req + reqIndex;
			MemorySlot* slot = GetSlotFor( entity->memory, req->type );
			if( slot && !slot->usedInQuest && MatchesCriteria( slot, req->criteria ) )
			{
				slot->usedInQuest = true;
			}
			else
			{
				generated = false;
				break;
			}
		}
	}
}

internal void DescribeQuest( Quest, DialogueState)
{
	MemoryNode* node = quest->nodes + dialogueState->questNodeCount++;

	DialogueLine line = Describe( node );
	return line;
}


void FillQuestTemplatesTable
{
	AddRule( AllQuests, allTaskCompleted, GiveRewards );
	AddRule( SomeQuests, allTaskCompleted, CheckQuest, 50% );
	...
}

internal void AdvanceQuestStatus( entity, newStuff)
{
	...
	Assert( RelevantToQuest( quest, newStuff );
	QuestState* questState = quest->state;

	switch( quest->state )
	{
		
	}

	if( changedState )
	{
		DispatchChange( quest->state );
	}
}