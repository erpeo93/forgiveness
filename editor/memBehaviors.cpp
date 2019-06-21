inline void BeginMemoryBehavior(char* behaviorName)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName);
    
    if(slot)
    {
        FREELIST_FREE(slot->criteria, MemCriteria, taxTable_->firstFreeMemCriteria);
        
        for(MemSynthesisRule* rule = slot->synthRules; rule; rule = rule->next)
        {
            FreeMemSynthRuleTree(taxTable_, rule->tree.root);
        }
        FREELIST_FREE(slot->synthRules, MemSynthesisRule, taxTable_->firstFreeMemSynthesisRule);
        
        Assert(IsBehavior(taxTable_, slot->taxonomy));
        currentSlot_ = slot;
    }
    else
    {
        EditorErrorLog(behaviorName);
    }
}

inline MemCriteria* AddCriteria(char* criteriaName)
{
    MemCriteria* result = 0;
    TaxonomySlot* criteriaTax = NORUNTIMEGetTaxonomySlotByName(taxTable_, criteriaName);
    if(criteriaTax)
    {
        MemCriteria* newCriteria;
        TAXTABLE_ALLOC(newCriteria, MemCriteria);
        newCriteria->taxonomy = criteriaTax->taxonomy;
        
        FREELIST_INSERT(newCriteria, currentSlot_->criteria);
        result = newCriteria;
    }
    else
    {
        EditorErrorLog(criteriaName);
    }
    
    return result;
}

inline void AddMemConsideration(MemCriteria* criteria, char* requiredConceptName)
{
    TaxonomySlot* slot = NORUNTIMEGetTaxonomySlotByName(taxTable_, requiredConceptName);
    if(slot)
    {
        if(criteria)
        {
            Assert(criteria->possibleTaxonomiesCount < ArrayCount(criteria->requiredConceptTaxonomy));
            criteria->requiredConceptTaxonomy[criteria->possibleTaxonomiesCount++] = slot->taxonomy;
        }
    }
    else
    {
        EditorErrorLog(requiredConceptName);
    }
}

inline MemSynthesisRule* AddSynthesisRule(EntityAction action)
{
    MemSynthesisRule* rule;
    TAXTABLE_ALLOC(rule, MemSynthesisRule);
    rule->action = action;
    
    FREELIST_INSERT(rule, currentSlot_->synthRules);
    
    return rule;
}

inline TaxonomyNode* AddNode(MemSynthesisRule* rule, char* name)
{
    TaxonomyNode* result = 0;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(target)
    {
        if(rule)
        {
            result = AddToTaxonomyTree(&rule->tree, target);
        }
    }
    else
    {
        EditorErrorLog(name);
    }
}

inline MemSynthOption* AddOption(TaxonomyNode* node, char* conceptName, u32 lastingtimeUnits, u32 refreshTimeUnits)
{
    MemSynthOption* result = 0;
    TaxonomySlot* conceptSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, conceptName);
    
    if(conceptSlot)
    {
        MemSynthOption* option;
        TAXTABLE_ALLOC(option, MemSynthOption);
        option->lastingtimeUnits = SafeTruncateToU16(lastingtimeUnits);
        option->refreshTimeUnits = SafeTruncateToU16(refreshTimeUnits);
        option->outputConcept = conceptSlot->taxonomy;
        
        FREELIST_INSERT(option, node->data.firstOption);
        result = option;
    }
    else
    {
        EditorErrorLog(conceptName);
    }
    
    return result;
}

#if 0
inline void OnCondition(char* requiredAssociationConcept)
{
    CriteriaOption* option;
    TAXTABLE_ALLOC(option, CriteriaOption);
    TaxonomySlot* conceptSlot = NORUNTIMEGetTaxonomySlotByName(behaviorsTaxTable_, requiredAssociationConcept);
    option->requiredAssociationConcept = conceptSlot->taxonomy;
    
    FREELIST_INSERT(option, currentCriteriaNode_->data.firstOption);
}
#endif

inline void AddMemBehavior(char* behaviorName)
{
    TaxonomySlot* behaviorSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, behaviorName);
    
    if(behaviorSlot)
    {
        TaxonomyMemBehavior* newBehavior;
        TAXTABLE_ALLOC(newBehavior, TaxonomyMemBehavior);
        newBehavior->taxonomy = behaviorSlot->taxonomy;
        
        FREELIST_INSERT(newBehavior, currentSlot_->firstMemBehavior);
    }
    else
    {
        EditorErrorLog(behaviorName);
    }
}

#define Seconds(value) value
#define Minutes(value) 60 * Seconds(value)
#define Hours(value) 60 * Minutes(value)

internal void ReadSynthesisRules()
{
    
#if 0    
    BeginMemoryBehavior("idiot memory behavior");
    AddCriteria("enemy");
    AddMemConsideration("attacker");
    AddMemConsideration("hostile");
    
    AddSynthesisRule(Action_Attack);
    AddNode("monsters");
    AddOption("attacker", Minutes(40), Minutes(2));
    //AddBooleanConsideration();
    
    AddCriteria("foodCrit");
    AddMemConsideration("foodCandidate");
    
    AddSynthesisRule(Action_None);
    AddNode("apple");
    AddOption("foodCandidate", Minutes(10), Minutes(1));
    
    
    
    
    
    
    BeginMemoryBehavior("idiot memory behavior 2");
    AddCriteria("enemy");
    AddMemConsideration("attacker");
    AddMemConsideration("hostile");
    
    AddSynthesisRule(Action_Attack);
    AddNode("root");
    AddOption("attacker", Minutes(40), Minutes(2));
    //AddBooleanConsideration();
    
    AddSynthesisRule(Action_Idle);
    AddNode("monsters");
    AddOption("hostile", Minutes(20), Minutes(1));
    
    AddCriteria("foodCrit");
    AddMemConsideration("foodCandidate");
    
    AddSynthesisRule(Action_None);
    AddNode("apple");
    AddOption("foodCandidate", Minutes(10), Minutes(1));
#endif
    
}


internal void ImportMemBehaviorsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    FREELIST_FREE(slot->firstMemBehavior, TaxonomyMemBehavior, taxTable_->firstFreeTaxonomyMemBehavior);
    EditorElement* behaviors = root->firstInList;
    while(behaviors)
    {
        char* behavior = GetValue(behaviors, "name");
        AddMemBehavior(behavior);
        
        behaviors = behaviors->next;
    }
}