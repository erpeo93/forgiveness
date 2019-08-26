
inline void AddFreeHandReq(TaxonomySlot* slot, char* slotName, char* taxonomy)
{
    NakedHandReq* req = PushStruct(&currentSlot_->pool, NakedHandReq);
    TaxonomySlot* taxonomyslot = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomy);
    
    if(taxonomyslot)
    {
        req->slotIndex = SafeTruncateToU8(GetValuePreprocessor(SlotName, slotName));
        req->taxonomy = taxonomyslot->taxonomy;
        
        req->next = slot->firstNakedHandReq;
        slot->firstNakedHandReq = req;
    }
    else
    {
        EditorErrorLog(taxonomy);
    }
}


internal void ImportFreeHandsRequirementsTab(TaxonomySlot* slot, EditorElement* root)
{
    EditorElement* freeHandsReq = root->firstInList;
    while(freeHandsReq)
    {
        char* slotName = GetValue(freeHandsReq, "slot");
        char* taxonomy = GetValue(freeHandsReq, "taxonomyName");
        
        AddFreeHandReq(slot, slotName, taxonomy);
        
        freeHandsReq = freeHandsReq->next;
    }
}