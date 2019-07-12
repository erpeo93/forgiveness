inline LayoutPiece* AddLayoutPiece(ObjectLayout* layout, char* componentName, u8 index)
{
    u64 componentHashID = StringHash(componentName);
    
    LayoutPiece* dest;
    TAXTABLE_ALLOC(dest, LayoutPiece);
    
    dest->componentHashID = componentHashID;
	dest->index = index;
    
    FormatString(dest->name, sizeof(dest->name), "%s", componentName);
    dest->ingredientCount = 0;
    dest->parent = 0;
    
    FREELIST_INSERT(dest, layout->firstPiece);
    
    ++layout->pieceCount;
    
    return dest;
}

inline void AddLayoutPieceParams(LayoutPiece* piece, ObjectState state, Vec3 parentOffset, r32 parentAngle, Vec4 color, Vec2 scale, r32 alpha, Vec2 pivot)
{
    Assert(state < ObjectState_Count);
    LayoutPieceParams* params = piece->params + state;
    params->valid = true;
    params->parentOffset = parentOffset;
    params->parentAngle = parentAngle;
    params->scale = scale;
    params->alpha = alpha;
    params->pivot = pivot;
    params->color = color;
}

inline void AddIngredient(LayoutPiece* piece, char* name, u32 quantity)
{
    Assert(piece->ingredientCount < ArrayCount(piece->ingredientTaxonomies));
    TaxonomySlot* ingredientSlot = NORUNTIMEGetTaxonomySlotByName(taxTable_, name);
    if(ingredientSlot)
    {
		u32 ingredientIndex = piece->ingredientCount++;
        piece->ingredientTaxonomies[ingredientIndex] = ingredientSlot->taxonomy;
		piece->ingredientQuantities[ingredientIndex] = Max(quantity, 1);
    }
    else
    {
        EditorErrorLog(name);
    }
}


internal void ImportLayoutsTab(TaxonomySlot* slot, EditorElement* root)
{
    
    for(ObjectLayout* toDelete = slot->firstLayout; toDelete; toDelete = toDelete->next)
    {
        FREELIST_FREE(toDelete->firstPiece, LayoutPiece, taxTable_->firstFreeLayoutPiece);
    }
    FREELIST_FREE(slot->firstLayout, ObjectLayout, taxTable_->firstFreeObjectLayout);
    slot->layoutCount = 0;
    
    
    
    EditorElement* layouts = root->firstInList;
    while(layouts)
    {
        ObjectLayout* newLayout;
        TAXTABLE_ALLOC(newLayout, ObjectLayout);
        
        newLayout->nameHashID = StringHash(layouts->name);
        FormatString(newLayout->name, sizeof(newLayout->name), "%s", layouts->name);
        
        EditorElement* pieces = GetList(layouts, "pieces");
        while(pieces)
        {
            char* pieceName = GetValue(pieces, "component");
            u64 pieceHash = StringHash(pieceName);
            
            u8 index = 0;
            for(LayoutPiece* test = newLayout->firstPiece; test; test = test->next)
            {
                if(test->componentHashID == pieceHash)
                {
                    ++index;
                }
            }
            LayoutPiece* piece = AddLayoutPiece(newLayout, pieceName, index);
            
            
            for(u32 state = ObjectState_Default; state < ObjectState_Count; ++state)
            {
                piece->params[state].valid = false;
            }
            EditorElement* params = GetList(pieces, "params");
            while(params)
            {
                ObjectState type = (ObjectState) GetValuePreprocessor(ObjectState, GetValue(params, "objectState"));
                EditorElement* offset = GetStruct(params, "offset");
                r32 x = ToR32(GetValue(offset, "x"));
                r32 y = ToR32(GetValue(offset, "y"));
                r32 z = ToR32(GetValue(offset, "z"));
                
                r32 angle = ToR32(GetValue(params, "angle"));
                
                EditorElement* scale = GetStruct(params, "scale");
                r32 scaleX = ToR32(GetValue(scale, "x"));
                r32 scaleY = ToR32(GetValue(scale, "y"));
                r32 pieceAlpha = ToR32(GetValue(params, "alpha"));
                Vec2 pivot = ToV2(GetStruct(params, "pivot"), V2(0.5f, 0.5f));
                Vec4 color = ToV4Color(GetStruct(params, "color"), V4(1, 1, 1, 1));
                
                AddLayoutPieceParams(piece, type, V3(x, y, z), angle, color, V2(scaleX, scaleY), pieceAlpha, pivot);
                
                params = params->next;
            }
            
            EditorElement* ingredient = GetList(pieces, "ingredients");
            while(ingredient)
            {
                char* ingredientName = GetValue(ingredient, "ingredient");
                u32 quantity = ToU32(GetValue(ingredient, "quantity"));
                AddIngredient(piece, ingredientName, quantity);
                
                ingredient = ingredient->next;
            }
            
            EditorElement* decorationPieces = GetList(pieces, "childPieces");
            while(decorationPieces)
            {
                char* childName = GetValue(decorationPieces, "component");
                u64 childHash = StringHash(childName);
                
                u8 childIndex = 0;
                for(LayoutPiece* test = newLayout->firstPiece; test; test = test->next)
                {
                    if(test->componentHashID == childHash)
                    {
                        ++childIndex;
                    }
                }
                
                LayoutPiece* childPiece = AddLayoutPiece(newLayout, childName, childIndex);
                childPiece->parent = piece;
                
                for(u32 state = ObjectState_Default; state < ObjectState_Count; ++state)
                {
                    childPiece->params[state].valid = false;
                }
                
                EditorElement* childParams = GetList(decorationPieces, "params");
                while(childParams)
                {
                    ObjectState childType = (ObjectState) GetValuePreprocessor(ObjectState, GetValue(childParams, "objectState"));
                    EditorElement* childOffset = GetStruct(childParams, "offset");
                    r32 childX = ToR32(GetValue(childOffset, "x"));
                    r32 childY = ToR32(GetValue(childOffset, "y"));
                    r32 childZ = ToR32(GetValue(childOffset, "z"));
                    
                    r32 childAngle = ToR32(GetValue(childParams, "angle"));
                    
                    EditorElement* scale = GetStruct(decorationPieces, "scale");
                    r32 childScaleX = ToR32(GetValue(scale, "x"));
                    r32 childScaleY = ToR32(GetValue(scale, "y"));
                    r32 childAlpha = ToR32(GetValue(childParams, "alpha"));
                    
                    Vec2 childPivot = ToV2(GetStruct(childParams, "pivot"), V2(0.5f, 0.5f));
                    Vec4 childColor = ToV4Color(GetStruct(childParams, "color"), V4(1, 1, 1, 1));
                    
                    AddLayoutPieceParams(childPiece, childType, V3(childX, childY, childZ), childAngle, childColor, V2(childScaleX, childScaleY), childAlpha, childPivot); 
                    
                    childParams = childParams->next;
                }
                
                decorationPieces = decorationPieces->next;
                
            }
            
            pieces = pieces->next;
        }
        
        ++slot->layoutCount;
        FREELIST_INSERT(newLayout, slot->firstLayout);
        
        layouts = layouts->next;
    }
}