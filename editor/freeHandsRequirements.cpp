
inline void AddFreeHandReq(TaxonomySlot* slot, char* slotName, char* taxonomy)
{
    NakedHandReq* req;
    TAXTABLE_ALLOC(req, NakedHandReq);
    TaxonomySlot* taxonomyslot = NORUNTIMEGetTaxonomySlotByName(taxTable_, taxonomy);
    
    if(taxonomyslot)
    {
        req->slotIndex = SafeTruncateToU8(GetValuePreprocessor(SlotName, slotName));
        req->taxonomy = taxonomyslot->taxonomy;
        
        req->next = slot->nakedHandReq;
        slot->nakedHandReq = req;
    }
    else
    {
        EditorErrorLog(taxonomy);
    }
}


internal void ImportFreeHandsRequirementsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    FREELIST_FREE(slot->nakedHandReq, NakedHandReq, taxTable_->firstFreeNakedHandReq);
    EditorElement* freeHandsReq = root->firstInList;
    while(freeHandsReq)
    {
        char* slotName = GetValue(freeHandsReq, "slot");
        char* taxonomy = GetValue(freeHandsReq, "name");
        
        AddFreeHandReq(slot, slotName, taxonomy);
        
        freeHandsReq = freeHandsReq->next;
    }
}