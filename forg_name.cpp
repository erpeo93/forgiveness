enum NameElementType
{
	MainNoun,
	Noun
	Adjective
};

struct NameElement
{
	NameElementType type;
	char text[32];
};

struct NameRule
{
	rarity;
	u32 elementCount;
	NameElement elements[8];
};


internal void GenerateName(Essence* essences, u32 essenceCount, rarity, char* output, u32 outputCount, NameRule* rules, NameNoun possibleNouns, NameAdjectives possibleAdjectives)
{
	PickRandomRule(rules, rarity);
	for(u32 elementIndex = 0; elementIndex < rule->elementCount)
	{
		while(true)
		{
			u32 essenceCount = PickRandomEssences(essences);
			PickRandomCompatibleElement(essenceIndexes, element->type);
			if(element)
			{
				ConcatString(output);
				RemoveEssences();
				break;
			}
		}
	}
}


internal void FillNameRules()
{
	Start("weapons");
	AddSuffixNoun("fire", Essence_Fire, 2);
	AddSuffixNoun("water", Essence_Water, 1);
	AddSuffixNoun("ocean", EssenceWater, 2);

	AddRule(Common, uncommon, rare, "Noun of the AdjectiveNoArticle SuffixNoun");
	AddRule(Common, uncommon, rare, Adjective, "Noun of SuffixNoun");
	AddRule(Unique, "Adjective Noun");

	Start("swords");
	AddNoun("spada");
	AddNoun("lama");
	AddNoun("sciabola");
}