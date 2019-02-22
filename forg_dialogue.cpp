enum DialogueLineType
{
	Dialogue_sayHello,
	Dialogue_ask,
	Dialogue_describe,
	...
};

struct DialogueRuleParams
{
	u32 taxonomy;
	personalityParams;
};

struct DialogueLine
{
	DialogueLineType type;
	r32 params[16];
}

FillDialogueRules()
{
	AddLine( sayHello, "hello sir", normal );
	AddLine( sayHello, "stay away!", angry );

	...

	AddRule( Dialogue_ask, DialogueDescribe );
	AddRule( Dialogue_ask, Params( "dragon", angry ), Stimulus_Attack );
	AddRule( Dialogue_describe, Params( "", normal, Thanks );
	AddRule( Dialogue_ask, "quest", normal, ..., Describe, "quest" );
	AddRule( Dialogue_Describe, "quest", normal, Stimulus_AcceptQuest );
	AddRule( Dialogue_Give, Params( "quest" ), Stimulus_CheckQuestObject );

	SortRuleFromMoreSpecificToMoreGeneral();
}

both rules and the real lines are imported from file


internal DialogueLine* GetBestDialogueLineFor( DialogueLineType type, params )
{
	hashIndex = GetHashIndex( type, taxonomy );
	for( everyRule )
	{
		if( match )
		{
			result = rule;
			break;
		}
	}

	return result;
}


//Stimuli has the possiblity to "initiate" new dialogues
PerceiveStimulus()
{
	case xxx:
	{
		entity->dialogue = InitiateDialogue( entity, destEntity, stimulus->firstLine );
	} break;

	case yyy:
	{
		entity->dialogue = InitiateDialogue( entity, destEntity, stimulus->taxonomy, Ask );
		destEntity->dialogue = entity->dialogue;
	} break;

	...
}

internal void Dispatch( DialogueLine* line )
{
	for( everyReaction )
	{
		switch( reaction )
		{
			case Stimulus:
			{
				...
			}

			case NewLine:
			{
				...
			}
		}
	}
}

internal void ReactTo( Dialogue* dialogue, DialogueLine* line )
{
	...
	Personality, dialogueStatus, DestEntityOpinion -> Reaction;
	dialogue->currentLine = GetBestDialogueLineFor( dialogue, rule, personality, ecc ecc );
	Dispatch( dialogue->currentLine );
	...
}

internal HandleDialogue( Dialogue, Entity, otherEntity)
{
	if( dialogue->myTurn )
	{
		ReactTo( dialogue->lastThing );
	}
	else
	{
		... logic here
		if( tooMuchTimePassed )
		{
				GoAway();
		}
	}
}