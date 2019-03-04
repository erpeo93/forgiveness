internal Rect2 UIOrthoTextOp(RenderGroup* group, Font* font, PakFont* info, char* string, r32 fontScale, Vec3 P, TextOperation op, Vec4 color)
{
    Rect2 result = InvertedInfinityRect2();
    
    if(font && info)
    {
        u32 prevCodePoint = 0;
        for( char* at = string; *at; at++)
        {
            u8 codePoint = *at;
            P.x += fontScale * GetHorizontalAdvanceForPair( font, info, prevCodePoint, codePoint );
            if( codePoint != ' ' )
            {
                BitmapId ID = GetBitmapForGlyph( group->assets, font, info, codePoint );
                
                if( IsValid( ID ) )
                {
                    PakBitmap* glyphInfo = GetBitmapInfo( group->assets, ID );
                    r32 glyphHeight = fontScale * glyphInfo->dimension[1];
                    if( op == TextOp_draw )
                    {
                        PushBitmap( group, FlatTransform(), ID, P + V3( 2.0f, -2.0f, -0.001f ), glyphHeight, V2( 1.0f, 1.0f ), V4( 0.0f, 0.0f, 0.0f, 1.0f ) );
                        PushBitmap( group, FlatTransform(), ID, P, glyphHeight, V2( 1.0f, 1.0f ), color );
                    }
                    else
                    {
                        Assert( op == TextOp_getSize );
                        Bitmap* bitmap = GetBitmap( group->assets, ID);
                        if( bitmap )
                        {
                            BitmapDim dim = GetBitmapDim( bitmap, P, V3( 1.0f, 0.0f, 0.0f ), V3( 0.0f, 1.0f, 0.0f ), glyphHeight );
                            Rect2 glyphRect = RectMinDim(dim.P.xy, dim.size);
                            result = Union(result, glyphRect);
                            
                        }
                    }
                }
            }
            
            prevCodePoint = codePoint;
        }
        
    }
    
    return result;
}

inline Rect2 GetUIOrthoTextBounds(UIState* UI, char* text, r32 fontScale, Vec2 screenP)
{
    Rect2 bounds = UIOrthoTextOp(UI->group, UI->font, UI->fontInfo, text, fontScale, V3(screenP, 0), TextOp_getSize, V4(1, 1, 1, 1));
    
    return bounds;
}

inline void PushUIOrthoText(UIState* UI, char* text, r32 fontScale, Vec2 screenP, Vec4 color, r32 additionalZ = 0.0f)
{
    UIOrthoTextOp(UI->group, UI->font, UI->fontInfo, text, fontScale, V3(screenP, additionalZ), TextOp_draw, color);
}


internal Rect2 UITextOp(UIState* UI, RenderGroup* group, Font* font, PakFont* info, char* string, r32 fontScale, Vec2 PIn, Vec4 color, b32 computeRect = false)
{
    Vec2 P = PIn;
    Rect2 result = InvertedInfinityRect2();
    if(font && info)
    {
        u8 prevCodePoint = 0;
        
        r32 lightZBias = UI->worldMode->cameraWorldOffset.z - 0.06f;
        r32 darkZBias = UI->worldMode->cameraWorldOffset.z - 0.07f;
        
        for( char* at = string; *at; at++)
        {
            u8 codePoint = *at;
            P.x += fontScale * GetHorizontalAdvanceForPair(font, info, prevCodePoint, codePoint);
            if( codePoint != ' ' )
            {
                BitmapId ID = GetBitmapForGlyph(group->assets, font, info, codePoint);
                if(IsValid(ID))
                {
                    PakBitmap* glyphInfo = GetBitmapInfo(group->assets, ID);
                    r32 glyphHeight = fontScale * glyphInfo->dimension[1];
                    if(computeRect)
                    {
                        Bitmap* bitmap = GetBitmap( group->assets, ID);
                        if(bitmap)
                        {
                            Vec3 worldP = GetWorldP(group, P);
                            BitmapDim dim = GetBitmapDim(bitmap, worldP, V3(1.0f, 0.0f, 0.0f), V3(0.0f, 1.0f, 0.0f), glyphHeight);
                            Rect2 glyphRect = RectMinDim(dim.P.xy, dim.size);
                            result = Union( result, glyphRect );
                        }
                    }
                    else
                    {
                        PushUIBitmap(group, ID, P + V2(fontScale * 4.0f, 0), glyphHeight, 0, darkZBias, V2(1.0f, 1.0f), V4( 0.0f, 0.0f, 0.0f, 1.0f));
                        PushUIBitmap(group, ID, P + V2( 0, 0), glyphHeight, 0, lightZBias, V2( 1.0f, 1.0f), color);
                    }
                }
            }
            
            prevCodePoint = codePoint;
        }
    }
    
    return result;
}

inline Rect2 GetTextRect(UIState* UI, RenderGroup* group, Font* font, PakFont* info, char* string, r32 fontScale)
{
    Rect2 result = UITextOp(UI, group, font, info, string, fontScale, V2(0, 0), V4(1, 1, 1, 1), true);
    
    return result;
}

inline void PushCenteredText(UIState* UI, RenderGroup* group, Font* font, PakFont* info, char* string, r32 fontScale, Vec2 centerOffset, Vec4 color)
{
    Rect2 textRect = GetTextRect(UI, group, font, info, string, fontScale);
    Vec2 textCenter = GetCenter(textRect);
    Vec2 textDim = GetDim(textRect);
    
    UITextOp(UI, group, font, info, string, fontScale, centerOffset - (0.5f * textDim),
             color, false);
}

inline void PushUIText_(UIState* UI, char* text, Vec2 centerOffset, Vec4 color, r32 scale = 1.0f)
{
    PushCenteredText(UI, UI->group, UI->font, UI->fontInfo, text, UI->fontScale * scale, centerOffset, color);
}

inline void PushUITooltip(UIState* UI, char* text, Vec4 color, b32 prefix = false, b32 suffix = false, r32 scale = 1.0f)
{
    UI->prefix = prefix;
    UI->suffix = suffix;
    FormatString(UI->tooltipText, sizeof(UI->tooltipText), "%s", text);
}

inline void PushUITextWithDimension(UIState* UI, char* text, Vec2 centerOffset, Vec2 dim, Vec4 color)
{
    Vec2 standardDim = GetDim(GetTextRect(UI, UI->group, UI->font, UI->fontInfo, text, UI->fontScale));
    r32 coeffX = dim.x / standardDim.x;
    r32 coeffY = dim.y / standardDim.y;
    r32 coeff = Min(coeffX, coeffY);
    PushUIText_(UI, text, centerOffset, color, coeff);
}

inline b32 UIElementActive(UIState* UI, RenderGroup* group, Vec2 elementCenterP, Vec2 elementDim)
{
    Rect2 elementRect = RectCenterDim(V2(0, 0), elementDim);
    
    Vec3 worldP = GetWorldP(group, elementCenterP);
    Rect2 elementScreenRect = ProjectOnScreenCameraAligned(group, worldP, elementRect);
    b32 activeSkill = PointInRect(elementScreenRect, UI->screenMouseP);
    
    return activeSkill;
}


inline void EquipmentRemovedPrediction(ClientEntity* playerC, EquipInfo slot)
{
    playerC->prediction.type = Prediction_EquipmentRemoved;
    playerC->prediction.slot = slot;
    playerC->prediction.timeLeft = 0.6f;
}

inline void EquipmentPresentPrediction(GameModeWorld* worldMode, ClientEntity* playerC, EquipInfo slot, ClientEntity* entity)
{
    playerC->prediction.type = Prediction_EquipmentAdded;
    playerC->prediction.slot = slot;
    playerC->prediction.taxonomy = entity->taxonomy;
    playerC->prediction.recipeIndex = entity->recipeIndex;
    playerC->prediction.identifier = entity->identifier;
    if(!entity->identifier)
    {
        u64 fakeID = 0xffffffffffffffff;
        ClientEntity* fakeEntity = GetEntityClient(worldMode, fakeID, true);
        *fakeEntity = *entity;
        playerC->prediction.identifier = fakeID;
        fakeEntity->identifier = fakeID;
    }
    playerC->prediction.timeLeft = 0.6f;
}

inline void ActionBeganPrediciton(ClientEntity* playerC, EntityAction action)
{
    playerC->prediction.type = Prediction_ActionBegan;
    playerC->prediction.action = action;
    playerC->prediction.timeLeft = R32_MAX;
    
}
inline BitmapId GetRecursiveIconId(TaxonomyTable* table, Assets* assets, u32 taxonomy)
{
    BitmapId result = {};
    Assert(taxonomy);
    u32 currentTaxonomy = taxonomy;
    
    TagVector match = {};
    TagVector weight = {};
    while(currentTaxonomy)
    {
        TaxonomySlot* slot = GetSlotForTaxonomy(table, currentTaxonomy);
        BitmapId iconID = GetMatchingBitmap(assets, Asset_Icon, slot->stringHashID, &match, &weight);
        if(IsValid(iconID))
        {
            result = iconID;
            break;
        }
        
        currentTaxonomy = GetParentTaxonomy(table, currentTaxonomy);
    }
    
    Assert(IsValid(result));
    return result;
}

inline void GetUIName_(char* output, u32 outputSize, TaxonomyTable* table, u32 taxonomy, u32 recipeTaxonomy)
{
    char* slotName = GetSlotForTaxonomy(table, taxonomy)->name;
    
    if(taxonomy == table->recipeTaxonomy)
    {
        TaxonomySlot* recipeSlot = GetSlotForTaxonomy(table, recipeTaxonomy);
        FormatString(output, outputSize, "%s %s", recipeSlot->name, slotName);
    }
    else
    {
        FormatString(output, outputSize, "%s", slotName);
    }
}
inline void GetUIName(char* output, u32 outputSize, TaxonomyTable* table, ClientEntity* entity)
{
    GetUIName_(output, outputSize, table, entity->taxonomy, entity->recipeTaxonomy);
}
inline void GetUIName(char* output, u32 outputSize, TaxonomyTable* table, Object* object)
{
    u32 taxonomy = GetObjectTaxonomy(table, object);
    GetUIName_(output, outputSize, table, taxonomy, object->taxonomy);
}


struct PickInfo
{
    SlotName slot;
    ObjectReference reference;
};

inline b32 IsOnFocus(ClientEntity* entityC, u32 slotIndex)
{
    b32 result = false;
    
    EquipInfo focusSlots = entityC->animation.output.focusSlots;
    for(u32 focusSlotIndex = 0; focusSlotIndex < focusSlots.slotCount; ++focusSlotIndex)
    {
        if(slotIndex == (u32) focusSlots.slots[focusSlotIndex])
        {
            result = true;
            break;
        }
    }
    
    return result;
}

struct ObjectReference HasFreeSpace(ClientEntity* container)
{
    ObjectReference result = {};
    ContainedObjects* objects = &container->objects;
    if(objects->objectCount < objects->maxObjectCount)
    {
        result.containerID = container->identifier;
        for(u8 objectIndex = 0; objectIndex < objects->maxObjectCount; ++objectIndex)
        {
            Object* object = objects->objects + objectIndex;
            if(!object->taxonomy)
            {
                result.objectIndex = objectIndex;
                break;
            }
        }
    }
    
    return result;
}

inline PickInfo PossibleToPick(GameModeWorld* worldMode, TaxonomyTable* table, ClientEntity* entity, u32 objectTaxonomy, b32 excludeOnFocus)
{
    PickInfo result;
    result.slot = Slot_None;
    for(u32 slotIndex = 0; slotIndex < Slot_Count; ++slotIndex)
    {
        EquipmentSlot* slot = entity->equipment + slotIndex;
        if(slot->ID)
        {
            if(!excludeOnFocus || !IsOnFocus(entity, slotIndex))
            {
                
                ClientEntity* container = GetEntityClient(worldMode, slot->ID);
                if(container)
                {
                    ObjectReference reference = HasFreeSpace(container);
                    if(reference.containerID)
                    {
                        result.slot = (SlotName) slotIndex;
                        result.reference = reference;
                        break;
                    }
                }
            }
        }
    }
    
    return result;
}








#define UIStandardInventoryRequest(request, containerID, objectIndex) UIStandardInventoryRequest_(UIRequest_##request, containerID, objectIndex)
inline UIRequest UIStandardInventoryRequest_(UIRequestType requestCode, u64 containerID, u8 objectIndex)
{
    UIRequest result;
    result.requestCode = requestCode;
    result.containerEntityID = containerID;
    result.objectIndex = objectIndex;
    
    return result;
}

#define UIStandardSlotRequest(request, slot, containerID, objectIndex) UIStandardSlotRequest_(UIRequest_##request, slot, containerID, objectIndex)
inline UIRequest UIStandardSlotRequest_(UIRequestType requestCode, u32 slotIndex, u64 containerID, u8 objectIndex)
{
    UIRequest result;
    result.requestCode = requestCode;
    result.slot = slotIndex;
    result.destContainerID = containerID;
    result.destObjectIndex = objectIndex;
    
    return result;
}

inline UIRequest StandardActionRequest(u32 actionIndex, u64 identifier)
{
    UIRequest result;
    result.requestCode = UIRequest_None;
    result.action = actionIndex;
    result.identifier = identifier;
    
    return result;
}

inline UIRequest EditRequest(u32 taxonomy)
{
    UIRequest result;
    result.requestCode = UIRequest_Edit;
    result.taxonomy = taxonomy;
    
    return result;
}

inline UIRequest DeleteRequest(u64 identifier)
{
    UIRequest result;
    result.requestCode = UIRequest_DeleteEntity;
    result.identifier = identifier;
    
    return result;
}

inline UIRequest ImpersonateRequest(u64 identifier)
{
    UIRequest result;
    result.requestCode = UIRequest_ImpersonateEntity;
    result.identifier = identifier;
    
    return result;
}

inline UIRequest SendDataFileRequest()
{
    UIRequest result = {};
    result.requestCode = UIRequest_EditTab;
    
    return result;
}

inline UIRequest AddTaxonomyRequest(u32 parentTaxonomy, char* name)
{
    UIRequest result = {};
    result.requestCode = UIRequest_AddTaxonomy;
    result.parentTaxonomy = parentTaxonomy;
    FormatString(result.taxonomyName, sizeof(result.taxonomyName), "%s", name);
    
    return result;
}

inline UIRequest DeleteTaxonomyRequest(u32 taxonomy)
{
    UIRequest result = {};
    result.requestCode = UIRequest_DeleteTaxonomy;
    result.taxonomy = taxonomy;
    
    return result;
}
inline UIRequest InstantiateTaxonomyRequest(u32 taxonomy, Vec3 offset)
{
    UIRequest result = {};
    result.requestCode = UIRequest_InstantiateTaxonomy;
    result.taxonomy = taxonomy;
    result.offset = offset;
    
    return result;
}

inline UIRequest SaveAssetFadFileRequest(char* fileName, EditorWidget* widget)
{
    UIRequest result = {};
    result.requestCode = UIRequest_SaveAssetFile;
    FormatString(result.fileName, sizeof(result.fileName), "%s", fileName);
    result.widget = widget;
    
    return result;
}

inline UIRequest ReloadAssetsRequest()
{
    UIRequest result = {};
    result.requestCode = UIRequest_ReloadAssets;
    
    return result;
}

inline UIRequest PatchServerRequest()
{
    UIRequest result = {};
    result.requestCode = UIRequest_PatchServer;
    
    return result;
}

inline UIRequest SaveTaxonomyTabRequest()
{
    UIRequest result = {};
    result.requestCode = UIRequest_SaveTaxonomyTab;
    
    return result;
}


inline void UIAddUndoRedoCommand(UIState* UI, UndoRedoCommand command);
inline void UIHandleRequest(UIState* UI, UIRequest* request)
{
    switch(request->requestCode)
    {
        case UIRequest_Equip:
        {
            SendEquipRequest(request->containerEntityID, request->objectIndex);
        } break;
        
        case UIRequest_Pick:
        {
            SendMoveRequest(request->sourceContainerID, request->sourceObjectIndex, request->destContainerID, request->destObjectIndex);
        } break;
        
        case UIRequest_Disequip:
        {
            SendDisequipRequest(request->slot, request->destContainerID, request->destObjectIndex);
            
            if(UI->player->equipment[request->slot].ID == UI->lockedInventoryID1)
            {
                UI->lockedInventoryID1 = 0;
            }
            if(UI->player->equipment[request->slot].ID == UI->lockedInventoryID2)
            {
                UI->lockedInventoryID2 = 0;
            }
        } break;
        
        case UIRequest_Drop:
        {
            SendDropRequest(request->containerEntityID, request->objectIndex);
        } break;
        
        case UIRequest_Swap:
        {
            SendSwapRequest(request->containerEntityID, request->objectIndex);
        } break;
        
        case UIRequest_EquipDragging:
        {
            SendEquipDraggingRequest(request->slotIndex);
            EquipmentPresentPrediction(UI->worldMode, UI->player, UI->player->animation.output.focusSlots, &UI->draggingEntity);
        } break;
        
        case UIRequest_DragEquipment:
        {
            SendDragEquipmentRequest(request->slotIndex);
            EquipmentRemovedPrediction(UI->player, UI->player->animation.output.focusSlots);
            UI->animationGhostAllowed = false;
            
            if(UI->player->equipment[request->slotIndex].ID == UI->lockedInventoryID1)
            {
                UI->lockedInventoryID1 = 0;
            }
            
            if(UI->player->equipment[request->slotIndex].ID == UI->lockedInventoryID2)
            {
                UI->lockedInventoryID2 = 0;
            }
        } break;
        
        case UIRequest_Learn:
        {
            b32 alreadyLearned = false;
            
            BookElementsBlock* recipeBlock = UI->bookModes[UIBook_Recipes].elements;
            
            while(recipeBlock && !alreadyLearned)
            {
                for(u32 recipeIndex = 0; recipeIndex < recipeBlock->elementCount; ++recipeIndex)
                {
                    BookElement* element = recipeBlock->elements + recipeIndex;
                    if(element->type == Book_Recipe)
                    {
                        if(element->taxonomy == request->taxonomy && element->recipeIndex == request->recipeIndex)
                        {
                            alreadyLearned = true;
                            break;
                        }
                    }
                }
                recipeBlock = recipeBlock->next;
            }
            
            if(!alreadyLearned)
            {
                SendLearnRequest(request->containerID, request->objectIndex);
            }
            else
            {
                // NOTE(Leonardo): play sound, write something
            }
        } break;
        
        case UIRequest_Consume:
        {
            SendConsumeRequest(request->containerID, request->objectIndex);
        } break;
        
        case UIRequest_Craft:
        {
            SendCraftFromInventoryRequest(request->containerID, request->objectIndex);
            //ActionBeganPrediciton(UI->player, Action_Craft);
            UI->nextMode = UIMode_None;
        } break;
        
        case UIRequest_Open:
        {
            if(!UI->lockedInventoryID1)
            {
                UI->lockedInventoryID1 = request->containerID;
            }
            else
            {
                Assert(!UI->lockedInventoryID2);
                UI->lockedInventoryID2 = request->containerID;
            }
        }break;
        
        case UIRequest_Close:
        {
            if(request->IDToClose == UI->lockedInventoryID1)
            {
                UI->lockedInventoryID1 = 0;
            }
            else
            {
                Assert(request->IDToClose == UI->lockedInventoryID2);
                UI->lockedInventoryID2 = 0;
            }
        }break;
        
        case UIRequest_Edit:
        {
            if(request->taxonomy != UI->editingTaxonomy)
            {
                UIAddUndoRedoCommand(UI, UndoRedoEditTaxonomy(UI->editingTaxonomy, UI->editingTabIndex, request->taxonomy));
                
                UI->editingTaxonomy = request->taxonomy;
                UI->editingTabIndex = 0;
            }
        } break;
        
        case UIRequest_DeleteEntity:
        {
            SendDeleteRequest(request->identifier);
        } break;
        
        case UIRequest_ImpersonateEntity:
        {
            SendImpersonateRequest(request->identifier);
        } break;
        
        case UIRequest_EditTab:
        {
            TaxonomySlot* editingSlot = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
            u32 sendingIndex = UI->editingTabIndex;
            EditorTab* toSend = editingSlot->tabs + sendingIndex;
            if(toSend->root)
            {
                SendNewTabMessage();
                SendEditorElements(toSend->root);
                SendReloadEditingMessage(UI->editingTaxonomy, sendingIndex);
            }
        } break;
        
        case UIRequest_AddTaxonomy:
        {
            SendAddTaxonomyRequest(request->parentTaxonomy, request->taxonomyName);
        } break;
        
        case UIRequest_DeleteTaxonomy:
        {
            SendDeleteTaxonomyRequest(request->taxonomy);
        } break;
        
        case UIRequest_InstantiateTaxonomy:
        {
            SendInstantiateTaxonomyRequest(request->taxonomy, request->offset);
        } break;
        
        case UIRequest_SaveAssetFile:
        {
            request->widget->changeCount = 0;
            SendSaveAssetDefinitionFile(request->fileName, request->widget->root);
        } break;
        
        case UIRequest_ReloadAssets:
        {
            UI->reloadingAssets = true;
            SendReloadAssetsRequest();
        } break;
        
        case UIRequest_PatchServer:
        {
            UI->patchingLocalServer = true;
            SendPatchServerRequest();
        } break;
        
        case UIRequest_SaveTaxonomyTab:
        {
            TaxonomySlot* editing = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
            editing->editorChangeCount = 0;
            
            SendSaveTabRequest(UI->editingTaxonomy);
        } break;
        InvalidDefaultCase;
    }
}


#define UIResetListPossibility(UI, listName) UI->listName.possibilityCount = 0;
#define UIResetListIndex(UI, listName) UI->listName.currentIndex = 0;
#define UIMarkListToUpdateAndRender(UI, listName) UIMarkListToUpdate(UI, listName); UIMarkListToRender(UI, listName);
#define UIMarkListToUpdate(UI, listName) UI->toUpdateList = &UI->listName
#define UIMarkListToRender(UI, listName) UI->toRenderList = &UI->listName

inline void UIAddPossibility(UIScrollableList* list, char* actionName, char* entityName, UIRequest request)
{
    UIScrollableElement element;
    FormatString(element.name, sizeof(element.name), "%s %s", actionName, entityName);
    element.request = request;
    
    Assert(list->possibilityCount < ArrayCount(list->possibilities));
    list->possibilities[list->possibilityCount++] = element;
    
}

inline void UIAddPossibility(UIScrollableList* list, ClientEntity* entity)
{
    UIScrollableElement element;
    element.name[0] = 0;
    element.entity = entity;
    
    Assert(list->possibilityCount < ArrayCount(list->possibilities));
    list->possibilities[list->possibilityCount++] = element;
    
}

inline void WrapScrollableList(UIScrollableList* list)
{
    if(list->currentIndex >= list->possibilityCount)
    {
        list->currentIndex = 0;
    }
    
    Assert(list->currentIndex < 10);
}

inline void UpdateScrollableList(UIState* UI, UIScrollableList* list, i32 offset)
{
    if(list)
    {
        i32 newIndex = (i32) list->currentIndex + offset;
        newIndex = Wrap(0, newIndex, (i32) list->possibilityCount);
        list->currentIndex = (u32) newIndex;
    }
}

inline UIScrollableElement* GetActiveElement(UIScrollableList* list)
{
    UIScrollableElement* result = 0;
    if(list->currentIndex < list->possibilityCount)
    {
        result = list->possibilities + list->currentIndex;
    }
    return result;
}

inline void UIRenderList(UIState* UI, UIScrollableList* list)
{
    if(list && list->possibilityCount)
    {
        if(list->currentIndex >= list->possibilityCount)
        {
            list->currentIndex = 0;
        }
        
        UIScrollableElement* element = list->possibilities + list->currentIndex;
        
        if(element->name[0])
        {
            char tooltipText[64];
            
            b32 prefix = (list->currentIndex < (list->possibilityCount - 1));
            b32 suffix = (list->currentIndex > 0);
            
            
            
            FormatString(tooltipText, sizeof(tooltipText), "%s", element->name);
            
            PushUITooltip(UI, tooltipText, V4(1, 1, 1, 1), prefix, suffix, 1.0f);
        }
    }
}



#define UIAddInteraction(UI, input, buttonName, interaction, ...) UIAddInteraction_(UI, (u32) (&(input)->buttonName - (input)->buttons), interaction, __VA_ARGS__)
inline void UIAddInteraction_(UIState* UI, u32 buttonIndex, UIInteraction newInteraction, UIInteractionPriority priority = UIPriority_Standard, UIInteractionButtonGroup* group = 0)
{
    Assert(buttonIndex < MAX_BUTTON_COUNT);
    UIInteraction* interaction = UI->hotInteractions + buttonIndex;
    if(priority >= interaction->priority)
    {
        *interaction = newInteraction;
        interaction->priority = priority;
        interaction->excludeFromReset = group;
    }
}


inline UIInteractionAction* UIGetFreeAction(UIInteraction* interaction)
{
    Assert(interaction->actionCount < ArrayCount(interaction->actions));
    UIInteractionAction* result = interaction->actions + interaction->actionCount++;
    return result;
}

inline void UIAddAction(UIInteraction* interaction, UIInteractionAction source)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    *dest = source;
}

#define UIAddStandardAction(interaction, flags, structure, dest, source) UIAddStandardAction_(interaction, flags, sizeof(structure), dest, source)
inline void UIAddStandardAction_(UIInteraction* interaction, u32 flags, u32 size, UIMemoryReference dest, UIMemoryReference source)
{
    UIInteractionAction action;
    action.type = UIInteractionAction_Copy;
    action.flags = flags;
    
    UIMemoryPair* copy = &action.copy;
    copy->dest = dest;
    copy->source = source;
    copy->size = size;
    
    UIAddAction(interaction, action);
}

inline void UIAddRequestAction(UIInteraction* interaction, u32 flags, UIRequest request)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    Assert(interaction->data.request.requestCode == UIRequest_None);
    interaction->data.request = request;
    dest->type = UIInteractionAction_SendRequest;
    dest->flags = flags;
    dest->request = UIDataPointer(request);
}

inline void UIAddObjectToEntityAction(UIInteraction* interaction, u32 flags, UIMemoryReference entityDest, Object* object)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    Assert(interaction->data.object.taxonomy == 0);
    interaction->data.object = *object;
    dest->type = UIInteractionAction_ObjectToEntity;
    dest->flags = flags;
    dest->objectToEntity = entityDest;
}

inline void UIAddClearAction(UIInteraction* interaction, u32 flags, UIMemoryReference toClear, u32 size)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    dest->type = UIInteractionAction_Clear;
    dest->flags = flags;
    UIMemoryPair* pair = &dest->clear;
    pair->dest = toClear;
    pair->size = size;
}

inline void UIAddOffsetV2Action(UIInteraction* interaction, u32 flags, UIMemoryReference value, UIMemoryReference offset, r32 speed)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    dest->type = UIInteractionAction_OffsetV2;
    dest->flags = flags;
    dest->value = value;
    dest->offset = offset;
    dest->speed = speed;
}

inline UIInteraction UIAddEmptyElementToListInteraction(u32 flags, EditorWidget* widget, EditorElement* list)
{
    UIInteraction result = {};
    UIInteractionAction* dest = UIGetFreeAction(&result);
    dest->type = UIInteractionAction_AddEmptyEditorElement;
    dest->flags = flags;
    dest->list = ColdPointer(list);
    dest->widget = ColdPointer(widget);
    
    return result;
}

inline UIInteraction UIPlaySoundInteraction(u32 flags, u64 soundTypeHash, u64 soundNameHash)
{
    UIInteraction result = {};
    UIInteractionAction* dest = UIGetFreeAction(&result);
    dest->type = UIInteractionAction_PlaySound;
    dest->flags = flags;
    dest->soundTypeHash = soundTypeHash;
    dest->soundNameHash = soundNameHash;
    
    return result;
}


inline UIInteraction UIPlaySoundEventInteraction(u32 flags, u64 eventNameHash)
{
    UIInteraction result = {};
    UIInteractionAction* dest = UIGetFreeAction(&result);
    dest->type = UIInteractionAction_PlaySoundEvent;
    dest->flags = flags;
    dest->eventNameHash = eventNameHash;
    
    return result;
}

inline void UIAddReloadElementAction(UIInteraction* interaction, u32 flags, EditorElement* toReload)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    dest->type = UIInteractionAction_ReloadElement;
    dest->flags = flags;
    dest->toReload = ColdPointer(toReload);
}

inline void UIAddReleaseDragAction(UIInteraction* interaction, u32 flags)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    dest->type = UIInteractionAction_ReleaseDragging;
    dest->flags = flags;
}

inline void UIAddUndoRedoAction(UIInteraction* interaction, u32 flags, UndoRedoCommand command)
{
    UIInteractionAction* dest = UIGetFreeAction(interaction);
    dest->type = UIInteractionAction_UndoRedoCommand;
    dest->flags = flags;
    dest->undoRedo = command;
}

inline UIInteraction NullInteraction()
{
    UIInteraction result;
    result.priority = UIPriority_NotValid;
    result.flags = 0;
    result.actionCount = 0;
    result.checkCount = 0;
    result.excludeFromReset = 0;
    
    return result;
}

inline UIInteraction UIUndoInteraction(u32 flags)
{
    UIInteraction result = NullInteraction();
    result.actionCount = 1;
    result.actions[0].type = UIInteractionAction_Undo;
    result.actions[0].flags = flags;
    
    return result;
}

inline UIInteraction UIRedoInteraction(u32 flags)
{
    UIInteraction result = NullInteraction();
    result.actionCount = 1;
    result.actions[0].type = UIInteractionAction_Redo;
    result.actions[0].flags = flags;
    
    return result;
}

#define UIAddSetValueActionDefinition(type)\
inline void UIAddSetValueAction(UIInteraction* interaction, u32 flags, type* destination, type value)\
{\
    UIMemoryReference source = Fixed_(&value, sizeof(type));\
    UIMemoryReference dest = ColdPointer(destination);\
    UIAddStandardAction(interaction, flags, value, dest, source);\
}


UIAddSetValueActionDefinition(r32);
UIAddSetValueActionDefinition(u32);
UIAddSetValueActionDefinition(b32);
UIAddSetValueActionDefinition(UIMode);
UIAddSetValueActionDefinition(char);
UIAddSetValueActionDefinition(EditorElement*);

#define UISetValueInteractionDefinition(type)\
inline UIInteraction UISetValueInteraction(u32 flags, type* destination, type value)\
{\
    UIInteraction result;\
    result.flags = 0;\
    result.actionCount = 0;\
    result.checkCount = 0;\
    UIAddSetValueAction(&result, flags, destination, value);\
    return result;\
}
UISetValueInteractionDefinition(r32);
UISetValueInteractionDefinition(u32);
UISetValueInteractionDefinition(b32);
UISetValueInteractionDefinition(UIMode);
UISetValueInteractionDefinition(char);
UISetValueInteractionDefinition(EditorElement*);


#define ScrollableList(list, field) ScrollableList_(list, OffsetOf(UIScrollableElement, field))
inline UIMemoryReference ScrollableList_(UIScrollableList* list, u32 possibilityDataOffset)
{
    UIMemoryReference result = ColdPointerDynamicOffset(list->possibilities, &list->currentIndex, possibilityDataOffset);					
    return result;
}

inline UIInteraction SendRequestInteraction(u32 flags, UIRequest request)
{
    UIInteraction result = {};
    
    UIInteractionAction requestAction;
    requestAction.type = UIInteractionAction_SendRequestDirectly;
    requestAction.flags = flags;
    requestAction.directRequest = request;
    UIAddAction(&result, requestAction);
    
    return  result;
}

inline UIInteraction ScrollableListRequestInteraction(u32 flags, UIScrollableList* list)
{
    UIInteraction result = {};
    
    UIInteractionAction requestAction;
    requestAction.type = UIInteractionAction_SendRequest;
    requestAction.flags = flags;
    requestAction.request = ScrollableList(list, request);
    UIAddAction(&result, requestAction);
    
    return result;
}

inline void UIAddScrollableTargetInteraction(UIInteraction* interaction, UIScrollableList* list, UIOutput* output)
{
    UIAddStandardAction(interaction, UI_Trigger, u32, UIDataPointer(actionIndex), ScrollableList(list, request.action));
    UIAddStandardAction(interaction, UI_Idle | UI_Retroactive, u32, ColdPointer(&output->desiredAction), UIDataPointer(actionIndex));
    
    UIAddStandardAction(interaction, UI_Trigger, u64, UIDataPointer(identifier), ScrollableList(list, request.identifier));
    UIAddStandardAction(interaction, UI_Idle | UI_Retroactive, u64, ColdPointer(&output->targetEntityID), UIDataPointer(identifier)); 
    
    UIAddStandardAction_(interaction, UI_Trigger, sizeof(myPlayer->targetPossibleActions[0]) * Action_Count, ColdPointer(myPlayer->targetPossibleActions), ColdPointer(myPlayer->overlappingPossibleActions));     
    UIAddStandardAction(interaction, UI_Trigger, u64, ColdPointer(&myPlayer->targetIdentifier), ColdPointer(&myPlayer->overlappingIdentifier));     
}

inline void UIAddStandardTargetInteraction(UIInteraction* interaction, UIOutput* out, u32 actionIndex, u64 identifier)
{
    UIAddStandardAction(interaction, UI_Trigger, u32, UIDataPointer(actionIndex), Fixed(actionIndex));
    UIAddStandardAction(interaction, UI_Trigger | UI_Retroactive, u32, ColdPointer(&out->desiredAction), UIDataPointer(actionIndex));
    
    UIAddStandardAction(interaction, UI_Trigger, u64, UIDataPointer(identifier), Fixed(identifier));
    UIAddStandardAction(interaction, UI_Trigger | UI_Retroactive, u64, ColdPointer(&out->targetEntityID), UIDataPointer(identifier)); 
    
    UIAddStandardAction_(interaction, UI_Trigger, sizeof(myPlayer->targetPossibleActions[0]) * Action_Count, ColdPointer(myPlayer->targetPossibleActions), ColdPointer(myPlayer->overlappingPossibleActions));     
    UIAddStandardAction(interaction, UI_Trigger, u64, ColdPointer(&myPlayer->targetIdentifier), ColdPointer(&myPlayer->overlappingIdentifier));     
}

#define UIAddInvalidCondition(interaction, type, ref1, ref2, ...) UIAddInvalidCondition_(interaction, sizeof(type), ref1, ref2, __VA_ARGS__)
inline void UIAddInvalidCondition_(UIInteraction* interaction, u32 size, UIMemoryReference ref1, UIMemoryReference ref2, u32 flags = 0)
{
    Assert(interaction->checkCount < ArrayCount(interaction->checks));
    UIInteractionCheck* check = interaction->checks + interaction->checkCount++;
    check->flags = flags;
    
    UIMemoryPair* pair = &check->check;
    pair->source = ref1;
    pair->dest = ref2;
    pair->size = size;
}








inline void UIResetActiveInteractions(UIState* UI, u32 triggeredButton, UIInteractionButtonGroup* exclude)
{
    for(u32 buttonIndex = 0; buttonIndex < MAX_BUTTON_COUNT; ++buttonIndex)
    {
        b32 reset = (buttonIndex == triggeredButton);
        if(!reset && exclude)
        {
            for(u32 groupButtonIndex = 0; groupButtonIndex < exclude->buttonCount; ++groupButtonIndex)
            {
                if(buttonIndex == exclude->buttonIndexes[groupButtonIndex])
                {
                    reset = false;
                    break;
                }
            }
        }
        
        if(reset)
        {
            UI->activeInteractions[buttonIndex].priority = UIPriority_NotValid;
        }
    }
}

inline EditorElement* CopyEditorElement(TaxonomyTable* table, EditorElement* source)
{
    EditorElement* result;
    FREELIST_ALLOC(result, table->firstFreeElement, PushStruct(&table->pool, EditorElement));
    
    *result = *source;
    
    
    switch(source->type)
    {
        case EditorElement_String:
        case EditorElement_Real:
        case EditorElement_Signed:
        case EditorElement_Unsigned:
        {
            
        } break;
        
        case EditorElement_List:
        {
            if(source->emptyElement)
            {
                result->emptyElement = CopyEditorElement(table, source->emptyElement);
            }
            
            if(source->firstChild)
            {
                result->firstChild = CopyEditorElement(table, source->firstChild);
            }
        } break;
        
        case EditorElement_Struct:
        {
            result->firstValue = CopyEditorElement(table, source->firstValue);
        } break;
        
        InvalidDefaultCase;
    }
    
    if(source->next)
    {
        result->next = CopyEditorElement(table, source->next);
    }
    
    
    return result;
}

inline void UpdateWidgetChangeCount(UIState* UI, EditorWidget* widget, i32 delta)
{
    if(widget)
    {
        if(StrEqual(widget->name, "Editing Tabs"))
        {
            TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
            slot->editorChangeCount += delta;
        }
        else
        {
            widget->changeCount += delta;
        }
    }
}

inline void UIAddUndoRedoCommand(UIState* UI, UndoRedoCommand command)
{
    UpdateWidgetChangeCount(UI, command.widget, 1);
    
    UndoRedoCommand* current = UI->current;
    while(current != &UI->undoRedoSentinel)
    {
        UndoRedoCommand* nextToFree = current->next;
        DLLIST_REMOVE(current);
        FREELIST_DEALLOC(current, UI->firstFreeUndoRedoCommand);
        current = nextToFree;
    }
    
    
    
    UndoRedoCommand* newCommand;
    FREELIST_ALLOC(newCommand, UI->firstFreeUndoRedoCommand, PushStruct(&UI->undoRedoPool, UndoRedoCommand));
    *newCommand = command;
    
    DLLIST_INSERT_AS_LAST(&UI->undoRedoSentinel, newCommand);
    UI->current = &UI->undoRedoSentinel;
    UI->canRedo = false;
}


inline void UIDispatchInteraction(UIState* UI, UIInteraction* interaction, u32 flag, r32 timeToAdvance, b32 onlyNotActivated = false)
{
    for(u32 actionIndex = 0; actionIndex < interaction->actionCount; ++actionIndex)
    {
        UIInteractionAction* action = interaction->actions + actionIndex;
        if(!onlyNotActivated || !(action->flags & UI_Activated))
        {
            if((action->flags & flag) == flag)
            {
                switch(action->type)
                {
                    case UIInteractionAction_Copy:
                    {
                        UIMemoryPair* copy = &action->copy;
                        void* source = GetValue(copy->source, &interaction->data);
                        void* dest = GetValue(copy->dest, &interaction->data);
                        memcpy(dest, source, copy->size);
                    } break;
                    
                    case UIInteractionAction_SendRequest:
                    {
                        UIRequest* request = (UIRequest*) GetValue(action->request, &interaction->data);
                        UIHandleRequest(UI, request);
                    } break;
                    
                    case UIInteractionAction_ObjectToEntity:
                    {
                        ClientEntity* entity = (ClientEntity*) GetValue(action->objectToEntity, &interaction->data);
                        Object* object = &interaction->data.object;
                        
                        if(IsRecipe(object))
                        {
                            entity->taxonomy = UI->table->recipeTaxonomy;
                            entity->recipeIndex = 0;
                            
                            entity->recipeTaxonomy = object->taxonomy;
                            entity->recipeRecipeIndex = object->recipeIndex;
                        }
                        else
                        {
                            entity->taxonomy = object->taxonomy;
                            entity->recipeIndex = object->recipeIndex;
                            
                            entity->recipeTaxonomy = 0;
                            entity->recipeRecipeIndex = 0;
                            
                            //entity->quantity = (r32) object->quantity;
                            entity->status = (r32) object->status;
                        }
                    } break;
                    
                    case UIInteractionAction_Clear:
                    {
                        UIMemoryPair* clear = &action->clear;
                        void* toClear = GetValue(clear->dest, &interaction->data);
                        ZeroSize(clear->size, toClear);
                    }break;
                    
                    case UIInteractionAction_SendRequestDirectly:
                    {
                        UIRequest* request = (UIRequest*) &action->directRequest;
                        UIHandleRequest(UI, request);
                    }break;
                    
                    case UIInteractionAction_OffsetV2:
                    {
                        r32 speed = action->speed;
                        Vec2* value = (Vec2*) GetValue(action->value, &interaction->data);
                        Vec2* offset = (Vec2*) GetValue(action->offset, &interaction->data);
                        
                        *value = *value + speed * *offset;
                    }break;
                    
                    case UIInteractionAction_AddEmptyEditorElement:
                    {
                        EditorWidget* widget = (EditorWidget*) GetValue(action->widget, &interaction->data);
                        
                        EditorElement* list = (EditorElement*) GetValue(action->list, &interaction->data);
                        EditorElement* newElement = CopyEditorElement(UI->table, list->emptyElement);
                        
                        if(list->flags & EditorElem_RecursiveEmpty)
                        {
                            EditorElement* firstValue = newElement->firstValue;
                            while(firstValue)
                            {
                                if(firstValue->type == EditorElement_List)
                                {
                                    firstValue->emptyElement = CopyEditorElement(UI->table, list->emptyElement);
                                    firstValue->flags |= EditorElem_RecursiveEmpty;
                                    break;
                                }
                                
                                firstValue = firstValue->next;
                            }
                        }
                        
                        if(list->flags & EditorElem_LabelsEditable)
                        {
                            FormatString(newElement->name, sizeof(newElement->name), "unnamed");
                        }
                        else
                        {
                            newElement->name[0] = 0;
                        }
                        
                        UIAddUndoRedoCommand(UI, UndoRedoAdd(widget, list, newElement));
                        
                        newElement->next = list->firstChild;
                        list->firstChild = newElement;
                    } break;
                    
                    
                    case UIInteractionAction_PlaySound:
                    {
                        u64 soundTypeHash = action->soundTypeHash;
                        u64 soundNameHash = action->soundNameHash;
                        
                        SoundId ID = FindSoundByName(UI->group->assets, soundTypeHash, soundNameHash);
                        
                        if(IsValid(ID))
                        {
                            PlaySound(UI->worldMode->soundState, ID);
                        }
                    } break;
                    
                    case UIInteractionAction_PlaySoundEvent:
                    {
                        SoundEvent* event = GetSoundEvent(UI->table, action->eventNameHash);
                        
                        EditorWidget* widget = UI->widgets + EditorWidget_SoundEvents;
                        EditorElement* activeLabels = widget->root->next->firstInList;
                        
                        u32 labelCount = 0;
                        SoundLabel labels[32];
                        
                        while(activeLabels)
                        {
                            char* labelName = GetValue(activeLabels, "name");
                            char* labelValue = GetValue(activeLabels, "value");
                            
                            Assert(labelCount < ArrayCount(labels));
                            SoundLabel* label = labels + labelCount++;
                            
                            label->hashID = StringHash(labelName);
                            label->value = ToR32(labelValue);
                            
                            activeLabels = activeLabels->next;
                        }
                        
                        
                        PickSoundResult pick = PickSoundFromEvent(UI->group->assets, event, labelCount, labels, &UI->table->eventSequence);
                        
                        for(u32 pickIndex = 0; pickIndex < pick.soundCount; ++pickIndex)
                        {
                            SoundId toPlay = pick.sounds[pickIndex];
                            r32 delay = pick.delays[pickIndex];
                            
                            if(IsValid(toPlay))
                            {
                                PlaySound(UI->worldMode->soundState, toPlay, delay);
                            }
                        }
                    } break;
                    
                    case UIInteractionAction_ReloadElement:
                    {
                        EditorElement* root = (EditorElement*) GetValue(action->toReload, &interaction->data);
                        TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, UI->editingTaxonomy);
                        Import(slot, root);
                    } break;
                    
                    case UIInteractionAction_ReleaseDragging:
                    {
                        if(UI->hotStructThisFrame)
                        {
                            EditorElement* hot = UI->hotStruct;
                            EditorWidget* hotWidget = UI->hotWidget;
                            
                            if(hot->type == EditorElement_List)
                            {
                                if(StrEqual(hot->elementName, "soundType"))
                                {
                                    if(UI->draggingParent)
                                    {
                                        char* soundName = GetValue(UI->dragging, "soundName");
                                        char* soundType = UI->draggingParent->name;
                                        if(soundName)
                                        {
                                            EditorElement* newSound = CopyEditorElement(UI->table, hot->emptyElement);
                                            newSound->name[0] = 0;
                                            
                                            for(EditorElement* value = newSound->firstValue; value; value = value->next)
                                            {
                                                if(StrEqual(value->name, "soundType"))
                                                {
                                                    FormatString(value->value, sizeof(value->value), "%s", soundType);
                                                }
                                                else if(StrEqual(value->name, "sound"))
                                                {
                                                    FormatString(value->value, sizeof(value->value), "%s", soundName);
                                                }
                                            }
                                            
                                            newSound->next = hot->firstInList;
                                            hot->firstInList = newSound;
                                            
                                            hotWidget->needsToBeReloaded = true;
                                        }
                                    }
                                }
                            }
                        }
                        
                        UI->dragging = 0;
                        UI->draggingParent = 0;
                    } break;
                    
                    case UIInteractionAction_UndoRedoCommand:
                    {
                        UIAddUndoRedoCommand(UI, action->undoRedo);
                    } break;
                    
                    case UIInteractionAction_Undo:
                    {
                        UndoRedoCommand* current = UI->current;
                        if(current->prev != &UI->undoRedoSentinel)
                        {
                            UndoRedoCommand* toExec = current->prev;
                            
                            UpdateWidgetChangeCount(UI, toExec->widget, -1);
                            
                            switch(toExec->type)
                            {
                                case UndoRedo_StringCopy:
                                {
                                    FormatString(toExec->ptr, toExec->maxPtrSize, "%s", toExec->oldString);
                                } break;
                                
                                case UndoRedo_AddElement:
                                {
                                    Assert(toExec->added == toExec->list->firstInList);
                                    toExec->list->firstInList = toExec->added->next;
                                    toExec->added->next = 0;
                                } break;
                                
                                case UndoRedo_DeleteElement:
                                {
                                    ClearFlags(toExec->deleted, EditorElem_Deleted);
                                    toExec->deleted->next = *toExec->prevNextPtr;
                                    *toExec->prevNextPtr = toExec->deleted;
                                } break;
                                
                                case UndoRedo_Paste:
                                {
                                    *toExec->dest = toExec->oldElem;
                                } break;
                                
                                case UndoRedo_EditTaxonomy:
                                {
                                    UI->editingTaxonomy = toExec->oldTaxonomy;
                                    UI->editingTabIndex = toExec->oldTabIndex;
                                } break;
                            }
                            
                            UI->current = toExec;
                            UI->canRedo = true;
                        }
                    } break;
                    
                    case UIInteractionAction_Redo:
                    {
                        if(UI->canRedo)
                        {
                            UndoRedoCommand* current = UI->current;
                            if(current != &UI->undoRedoSentinel)
                            {
                                UndoRedoCommand* toExec = current;
                                UpdateWidgetChangeCount(UI, toExec->widget, 1);
                                
                                switch(toExec->type)
                                {
                                    case UndoRedo_StringCopy:
                                    {
                                        FormatString(toExec->ptr, toExec->maxPtrSize, "%s", toExec->newString);
                                    } break;
                                    
                                    case UndoRedo_AddElement:
                                    {
                                        FREELIST_INSERT(toExec->added, toExec->list->firstInList);
                                    } break;
                                    
                                    case UndoRedo_DeleteElement:
                                    {
                                        AddFlags(toExec->deleted, EditorElem_Deleted);
                                        *toExec->prevNextPtr = toExec->deleted->next;
                                        toExec->deleted->next = 0;
                                    } break;
                                    
                                    case UndoRedo_Paste:
                                    {
                                        *toExec->dest = toExec->newElem;
                                    } break;
                                    
                                    case UndoRedo_EditTaxonomy:
                                    {
                                        UI->editingTaxonomy = toExec->newTaxonomy;
                                        UI->editingTabIndex = 0;
                                    } break;
                                }
                                
                                UndoRedoCommand* next = toExec->next;
                                if(next == &UI->undoRedoSentinel)
                                {
                                    UI->canRedo = false;
                                }
                                UI->current = next;
                            }
                        }
                    } break;
                }
                
                action->flags |= UI_Activated;
            }
        }
    }
}

inline b32 ConditionsSatisfied(UIInteraction* interaction)
{
    b32 result = true;
    for(u32 checkIndex = 0; checkIndex < interaction->checkCount; ++checkIndex)
    {
        UIInteractionCheck* check = interaction->checks + checkIndex;
        if((check->flags & interaction->flags) == check->flags)
        {
            UIMemoryPair* checkPair = &check->check;
            void* ref1 = GetValue(checkPair->source, &interaction->data);
            void* ref2 = GetValue(checkPair->dest, &interaction->data);
            if(!memcmp(ref1, ref2, checkPair->size))
            {
                result = false;
                break;
            }
        }
    }
    
    return result;
}
