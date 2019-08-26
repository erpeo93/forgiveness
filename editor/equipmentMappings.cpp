
inline EquipmentMapping* AddEquipmentMapping(char* equipmentName)
{
    TaxonomySlot* slot = currentSlot_;
    TaxonomySlot* target = NORUNTIMEGetTaxonomySlotByName(taxTable_, equipmentName);
    
    EquipmentMapping* result = 0;
    if(target)
    {
        EquipmentMapping* mapping = PushStruct(&currentSlot_->pool, EquipmentMapping);
        
        TaxonomyNode* node = AddToTaxonomyTree(&currentSlot_->equipmentMappings, target);
        node->data.equipmentMapping = mapping;
        
        result = mapping;
    }
    else
    {
        EditorErrorLog(equipmentName);
    }
    
    return result;
}


inline void AddPiece(EquipmentLayout* equipmentLayout, u32 assIndex, char* pieceName, u8 index, Vec2 assOffset, r32 zOffset, r32 angle, Vec2 scale)
{
    EquipmentAss* equipmentAss = PushStruct(&currentSlot_->pool, EquipmentAss);
    
    equipmentAss->assIndex = assIndex;
    equipmentAss->stringHashID = StringHash(pieceName);
    equipmentAss->index = index;
    equipmentAss->assOffset = assOffset;
    equipmentAss->zOffset = zOffset;
    equipmentAss->angle = angle;
    equipmentAss->scale = scale;
    
    equipmentAss->next = 0;
    
    FREELIST_INSERT(equipmentAss, equipmentLayout->firstEquipmentAss);
}

internal void ImportEquipmentMappingsTab(TaxonomySlot* slot, EditorElement* root)
{
    slot->equipmentMappings.root = 0;
    
    EditorElement* singleSlot = root->firstInList;
    while(singleSlot)
    {
        char* equipmentName = GetValue(singleSlot, "equipment");
        EquipmentMapping* mapping = AddEquipmentMapping(equipmentName);
        
        EditorElement* layouts = GetList(singleSlot, "layouts");
        while(layouts)
        {
            char* layoutName = GetValue(layouts, "layoutName");
            char* slotName = GetValue(layouts, "slot");
            
            EquipmentLayout* equipmentLayout = PushStruct(&slot->pool, EquipmentLayout);
            equipmentLayout->layoutHashID = StringHash(layoutName);
            equipmentLayout->slot = {(SlotName) GetValuePreprocessor(SlotName, slotName)};
            
            FREELIST_INSERT(equipmentLayout, mapping->firstEquipmentLayout);
            
            EditorElement* pieces = GetList(layouts, "pieces");
            while(pieces)
            {
                u32 assIndex = ToU32(GetValue(pieces, "assIndex"));
                
                
                char* pieceName = GetValue(pieces, "pieceName");
                u8 index = ToU8(GetValue(pieces, "index"));
                
                if(StrEqual(pieceName, "all"))
                {
                    index = 0xff;
                }
                Vec2 assOffset = ToV2(GetStruct(pieces, "assOffset"));
                r32 zOffset = ToR32(GetValue(pieces, "zOffset"));
                r32 angle = ToR32(GetValue(pieces, "angle"));
                Vec2 scale = ToV2(GetStruct(pieces, "scale"));
                
                AddPiece(equipmentLayout, assIndex, pieceName, index, assOffset, zOffset, angle, scale);
                
                pieces = pieces->next;
            }
            
            
            layouts = layouts->next;
        }
        
        singleSlot = singleSlot->next;
    }
}