inline UIBookmark* UIAddBookmark(UIState* UI, UIBookmarkPosition position, UIBookmarkCondition condition)
{
    Assert(UI->totalBookmarkCount < ArrayCount(UI->bookmarks));
    ++UI->bookmarkCount[position];
    UIBookmark* bookmark = UI->bookmarks + UI->totalBookmarkCount++;
    bookmark->position = position;
    bookmark->active = false;
    bookmark->condition = condition;
    
    return bookmark;
}

inline UIBookmarkCondition BookModeCondition(UIBookMode mode)
{
    UIBookmarkCondition result;
    result.type = UICondition_Mode;
    result.mode = mode;
    
    return result;
}

inline void UIDispatchBookmarkCondition(UIState* UI, UIBookmark* bookmark)
{
    b32 active = !bookmark->active;
    
    UIBookmarkCondition* condition = &bookmark->condition;
    switch(condition->type)
    {
        case UICondition_Mode:
        {
            UI->currentBookModeIndex = condition->mode;
            for(u32 bookmarkIndex = 0; bookmarkIndex < UI->totalBookmarkCount; ++bookmarkIndex)
            {
                UIBookmark* toDeactivate = UI->bookmarks + bookmarkIndex;
                if(toDeactivate->active && toDeactivate->position == bookmark->position)
                {
                    toDeactivate->active = false;
                    break;
                }
            }
            
            bookmark->active = true;
        } break;
        
        InvalidDefaultCase;
    }
}

inline void AddToFilteredElements(UIState* UI, BookMode* bookMode, BookElement* element)
{
    BookReference* reference;
    FREELIST_ALLOC(reference, UI->firstFreeReference, PushStruct(&UI->bookReferencePool, BookReference));
    reference->element = element;
    reference->next = bookMode->filteredElements;
    bookMode->filteredElements = reference;
    
    ++bookMode->filteredElementCount;
}

inline b32 UIConditionSatisfied(UIState* UI, BookMode* bookMode, BookElement* element)
{
    u32 parentTaxonomy = GetParentTaxonomy(UI->table, element->taxonomy);
    b32 result = parentTaxonomy == bookMode->filterTaxonomy;
    
    return result;
}

inline u16 UIRequestAndGetQuantityOf(UIState* UI, u32 taxonomy)
{
    u16 result = 0;
    
    UIOwnedIngredient* target = 0;
    for(u32 slotIndex = 0; slotIndex < ArrayCount(UI->ingredientHashMap); ++slotIndex)
    {
        UIOwnedIngredient* hashSlot = UI->ingredientHashMap + slotIndex;
        if(!hashSlot->taxonomy || (hashSlot->taxonomy == taxonomy))
        {
            target = hashSlot;
            break;
        }
    }
    
    Assert(target);
    target->needToBeDrawn = true;
    target->taxonomy = taxonomy;
    result = target->quantity;
    return result;
}

inline u32 GetEssenceQuantity(UIState* UI, u32 essenceTaxonomy)
{
    u32 result = 0;
    for(u32 essenceIndex = 0; essenceIndex < MAX_DIFFERENT_ESSENCES; ++essenceIndex)
    {
        EssenceSlot* slot = myPlayer->essences + essenceIndex;
        if(slot->taxonomy == essenceTaxonomy)
        {
            result = slot->quantity;
            break;
        }
    }
    
    return result;
}

inline b32 HasObjectOfKind(UIState* UI, ClientEntity* entity, u32 objectTaxonomy)
{
    GameModeWorld* worldMode = UI->worldMode;
    b32 result = false;
    for(u32 slotIndex = 0; slotIndex < Slot_Count && !result; ++slotIndex)
    {
        u64 equipmentID = entity->equipment[slotIndex].ID;
        if(equipmentID)
        {
            ClientEntity* container = GetEntityClient(worldMode, equipmentID);
            Assert(container);
            for(u32 objectIndex = 0; objectIndex < container->objects.maxObjectCount; ++objectIndex)
            {
                Object* object = container->objects.objects + objectIndex;
                if(object->taxonomy)
                {
                    if(IsRecipe(object))
                    {
                        if(objectTaxonomy == UI->table->recipeTaxonomy)
                        {
                            result = true;
                            break;
                        }
                    }
                    else
                    {
                        TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, object->taxonomy);
                        if(IsSubTaxonomy(objectTaxonomy, slot))
                        {
                            result = true;
                            break;
                        }
                    }
                }
            }
        }
    }
    
    return result;
}


internal b32 UIDrawRecipeElement(UIState* UI, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* recipeSlot, GenerationData gen, PlatformInput* input)
{
    b32 result = false;
    
    RenderGroup* group = UI->group;
    
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    
    Object object = {};
    object.taxonomy = recipeSlot->taxonomy;
    object.gen = gen;
    r32 additionalZBias = 20.1f;
    Vec3 objectP = GetWorldP(group, elementCenterP + V2(0.3f * elementDim.x, 0));
    RenderObject(group, UI->worldMode, &object, objectP, V2(0.4f * elementDim.x, 0.6f * elementDim.y), additionalZBias);
    
    Vec2 objectTextP = elementCenterP + V2(-0.1f * elementDim.x, 0.25f * elementDim.y);
    PushUITextWithDimension(UI, recipeSlot->name, objectTextP, V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
    RecipeIngredients ingredients;
    GetRecipeIngredients(&ingredients, UI->table, recipeSlot->taxonomy, gen);
    
    Vec2 ingredientDim;
    ingredientDim.y = 0.2f * elementDim.y;
    ingredientDim.x = (1.0f / (ingredients.count)) * 0.5f * elementDim.x;
    Vec2 ingredientP = elementCenterP - 0.25f * V2(elementDim.x, elementDim.y);
    
    b32 canCraft = true;
    b32 renderCraftTooltip = true;
    
    Vec2 toolTextP = elementCenterP;
    Vec2 toolTextDim = V2(0.4f * elementDim.x, 0.3f);
    
    for(u32 toolIndex = 0; toolIndex < ArrayCount(recipeSlot->neededToolTaxonomies); ++toolIndex)
    {
        u32 toolTaxonomy = recipeSlot->neededToolTaxonomies[toolIndex];
        if(toolTaxonomy)
        {
            TaxonomySlot* toolSlot = GetSlotForTaxonomy(UI->table, toolTaxonomy);
            Vec4 color = V4(1, 0, 0, 1);
            
            if(HasObjectOfKind(UI, UI->player, toolTaxonomy))
            {
                color = V4(0, 1, 0, 1);
            }
            
            PushUITextWithDimension(UI, toolSlot->name, toolTextP, toolTextDim, color);
            toolTextP.y -= 1.2f * toolTextDim.y;
        }
    }
    
    for(u32 ingredientIndex = 0; ingredientIndex < ingredients.count; ++ingredientIndex)
    {
        u32 ingredientTaxonomy = ingredients.taxonomies[ingredientIndex];
        u32 necessary = ingredients.quantities[ingredientIndex];
        
        u32 owned;
        
        Vec3 ingredientWorldP = GetWorldP(group, ingredientP);
        if(IsEssence(UI->table, ingredientTaxonomy))
        {
            ClientEntity* player = UI->player;
            
            TaxonomySlot* essenceSlot = GetSlotForTaxonomy(UI->table, ingredientTaxonomy);
            BitmapId iconID = GetRecursiveIconId(UI->table, group->assets, essenceSlot->taxonomy);
            
            PushUIBitmap(group, iconID, ingredientP, ingredientDim.y, 0, 20.2f, V2(1, 1), V4(1, 1, 1, 1));
            
            owned = 0;
            for(u32 essenceIndex = 0; essenceIndex < MAX_DIFFERENT_ESSENCES; ++essenceIndex)
            {
                EssenceSlot* essence = myPlayer->essences + essenceIndex;
                if(essence->taxonomy == ingredientTaxonomy)
                {
                    owned = SafeTruncateToU8(essence->quantity);
                    break;
                }
            }
        }
        else
        {
            Object ingredient = {};
            ingredient.taxonomy = ingredientTaxonomy;
            
            RenderObject(group, UI->worldMode, &ingredient, ingredientWorldP, ingredientDim, additionalZBias);
            owned = UIRequestAndGetQuantityOf(UI, ingredient.taxonomy);
        }
        
        Vec2 ingredientQuantityP = ingredientP;
        ingredientQuantityP.y -= 0.8f * ingredientDim.y;
        
        Vec4 quantitiesColor = V4(1, 0, 0, 1);
        if(owned >= necessary)
        {
            quantitiesColor = V4(0, 1, 0, 1);
        }
        else
        {
            canCraft = false;
        }
        
        Rect2 objectRect = RectCenterDim(V2(0, 0), ingredientDim);
        Rect2 objectScreenRect = ProjectOnScreenCameraAligned(group, ingredientWorldP, objectRect);
        
        if(PointInRect(objectScreenRect, UI->screenMouseP))
        {
            TaxonomySlot* ingredientSlot = GetSlotForTaxonomy(UI->table, ingredientTaxonomy);
            PushUITooltip(UI, ingredientSlot->name, V4(1, 0, 0, 1));
            renderCraftTooltip = false;
        }
        
        
        char quantities[16];
        FormatString(quantities, sizeof(quantities), "%d / %d", owned, necessary);
        PushUITextWithDimension(UI, quantities, ingredientQuantityP, V2(0.5f * ingredientDim.x, 0.3f * ingredientDim.y), quantitiesColor);
        
        ingredientP.x += ingredientDim.x;
    }
    
    BitmapId elementID = GetFirstBitmap(group->assets, Asset_BookElement);
    ObjectTransform elementTransform = UprightTransform();
    elementTransform.additionalZBias = 10.5f;
    
    Vec4 elementColor = V4(1, 1, 1, 0.5f);
    
    if(activeElement && canCraft)
    {
        if(renderCraftTooltip)
        {
            PushUITooltip(UI, "craft", V4(1, 0, 0, 1));
            elementColor = V4(1, 1, 1, 1.0f);
            
            if(Pressed(&input->mouseLeft))
            {
                element->hot = true;
                element->securityTimer = 0;
            }
            
            if(element->hot && IsDown(&input->mouseLeft))
            {
                element->securityTimer += input->timeToAdvance;
                r32 destTimer = 2.0f;
                if(element->securityTimer >= destTimer)
                {
                    SendCraftRequest(recipeSlot->taxonomy, gen);
                    //ActionBeganPrediciton(UI->player, Action_Craft);
                    UI->nextMode = UIMode_None;
                    
                    
                    element->hot = false;
                }
                
            }
            else
            {
                element->hot = false;
            }
        }
        else
        {
            element->hot = false;
        }
    }
    else
    {
        element->hot = false;
    }
    
    //PushBitmap(group, elementTransform, elementID, elementCenterP, elementDim.y, V2(1.0f, 1.0f), elementColor);
    return result;
}

internal b32 UIDrawRecipeCategoryElement(UIState* UI, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* categorySlot, PlatformInput* input)
{
    b32 result = false;
    
    RenderGroup* group = UI->group;
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    
    
    Vec2 categoryTextP = elementCenterP + V2(-0.1f * elementDim.x, 0.25f * elementDim.y);
    PushUITextWithDimension(UI, categorySlot->name, categoryTextP, V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
    BitmapId elementID = GetFirstBitmap(group->assets, Asset_BookElement);
    ObjectTransform elementTransform = UprightTransform();
    elementTransform.additionalZBias = 10.5f;
    
    Vec4 elementColor = V4(1, 1, 1, 0.5f);
    if(activeElement)
    {
        PushUITooltip(UI, "drill down", V4(1, 0, 0, 1));
        elementColor = V4(1, 1, 1, 1.0f);
        if(Pressed(&input->mouseLeft))
        {
            BookMode* activeBookMode = UI->bookModes + UI->currentBookModeIndex;
            activeBookMode->filterTaxonomy = categorySlot->taxonomy;
        }
    }
    
    //PushBitmap(group, elementTransform, elementID, elementCenterP, elementDim.y, V2(1.0f, 1.0f), elementColor);
    return result;
}

internal b32 UIDrawSkillElement(UIState* UI, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* skillSlot, u32 level, PlatformInput* input)
{
    b32 result = false;
    
    RenderGroup* group = UI->group;
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    
    
    Vec2 skillTextP = elementCenterP + V2(-0.1f * elementDim.x, 0.25f * elementDim.y);
    PushUITextWithDimension(UI, skillSlot->name, skillTextP, V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
    char stringLevel[8];
    FormatString(stringLevel, sizeof(stringLevel), "level: %d", level);
    PushUITextWithDimension(UI, stringLevel, skillTextP + V2(0, -0.5f), V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
    
    
    BitmapId elementID = GetFirstBitmap(group->assets, Asset_BookElement);
    ObjectTransform elementTransform = UprightTransform();
    elementTransform.additionalZBias = 10.5f;
    
    BitmapId iconID = GetRecursiveIconId(UI->table, group->assets, skillSlot->taxonomy);
    r32 iconHeight = 0.4f * elementDim.y;
    PushUIBitmap(group, iconID, elementCenterP, iconHeight, 0, 12.5f);
    
    
    Vec4 elementColor = V4(1, 1, 1, 0.5f);
    if(activeElement)
    {
        if(HasEssences(myPlayer->essences, skillSlot->essences))
        {
            PushUITooltip(UI, "level up", V4(1, 0, 0, 1));
            elementColor = V4(1, 1, 1, 1.0f);
            if(Pressed(&input->mouseLeft))
            {
                element->hot = true;
                element->securityTimer = 0;
            }
            
            if(element->hot && IsDown(&input->mouseLeft))
            {
                element->securityTimer += input->timeToAdvance;
                r32 destTimer = 2.0f;
                if(element->securityTimer >= destTimer)
                {
                    SendSkillLevelUpRequest(skillSlot->taxonomy);
                    element->hot = false;
                }
            }
            else
            {
                element->hot = false;
            }
        }
        else
        {
            element->hot = false;
        }
        
        
        for(i32 slotButtonIndex = 0; slotButtonIndex < MAXIMUM_SKILL_SLOTS; ++slotButtonIndex)
        {
            if(Pressed(input->slotButtons + slotButtonIndex))
            {
                for(i32 slotButtonIndex2 = 0; slotButtonIndex2 < MAXIMUM_SKILL_SLOTS; ++slotButtonIndex2)
                {
                    UISkill* skill = UI->skills + slotButtonIndex2;
                    if(skill->taxonomy == skillSlot->taxonomy)
                    {
                        skill->taxonomy = 0;
                        skill->active = false;
                    }
                }
                
                UISkill* skill = UI->skills + slotButtonIndex;
                skill->taxonomy = skillSlot->taxonomy;
                skill->active = true;
                
                if(skillSlot->isPassiveSkill)
                {
                    SendPassiveSkillRequest(skillSlot->taxonomy, true);
                }
                else
                {
                    if(slotButtonIndex == UI->activeSkillSlotIndex)
                    {
                        SendActiveSkillRequest(skillSlot->taxonomy);
                    }
                }
                
                platformAPI.DEBUGWriteFile("skills", UI->skills, sizeof(UI->skills));
            }
        }
    }
    else
    {
        element->hot = false;
    }
    
    //PushBitmap(group, elementTransform, elementID, elementCenterP, elementDim.y, V2(1.0f, 1.0f), elementColor);
    return result;
}


internal b32 UIDrawSkillCategoryElement(UIState* UI, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* categorySlot, PlatformInput* input)
{
    b32 result = false;
    
    RenderGroup* group = UI->group;
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    
    
    Vec2 categoryTextP = elementCenterP + V2(-0.1f * elementDim.x, 0.25f * elementDim.y);
    PushUITextWithDimension(UI, categorySlot->name, categoryTextP, V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
    BitmapId elementID = GetFirstBitmap(group->assets, Asset_BookElement);
    ObjectTransform elementTransform = UprightTransform();
    elementTransform.additionalZBias = 10.5f;
    
    
    BitmapId iconID = GetRecursiveIconId(UI->table, group->assets, categorySlot->taxonomy);
    r32 iconHeight = 0.4f * elementDim.y;
    PushUIBitmap(group, iconID, elementCenterP, iconHeight, 0, 12.5f);
    
    Vec4 elementColor = V4(1, 1, 1, 0.5f);
    if(activeElement)
    {
        elementColor = V4(1, 1, 1, 1.0f);
        if(element->unlocked)
        {
            PushUITooltip(UI, "drill down", V4(1, 0, 0, 1));
            if(Pressed(&input->mouseLeft))
            {
                BookMode* activeBookMode = UI->bookModes + UI->currentBookModeIndex;
                activeBookMode->filterTaxonomy = categorySlot->taxonomy;
            }
        }
        else
        {
            if(HasEssences(myPlayer->essences, categorySlot->essences))
            {
                PushUITooltip(UI, "unlock", V4(1, 0, 0, 1));
                if(Pressed(&input->mouseLeft))
                {
                    element->hot = true;
                    element->securityTimer = 0;
                }
                
                if(element->hot && IsDown(&input->mouseLeft))
                {
                    element->securityTimer += input->timeToAdvance;
                    r32 destTimer = 2.0f;
                    if(element->securityTimer >= destTimer)
                    {
                        SendUnlockSkillCategoryRequest(categorySlot->taxonomy);
                        element->hot = false;
                    }
                }
                else
                {
                    element->hot = false;
                }
                
            }
            else
            {
                element->hot = false;
            }
        }
    }
    else
    {
        element->hot = false;
    }
    
    //PushBitmap(group, elementTransform, elementID, elementCenterP, elementDim.y, V2(1.0f, 1.0f), elementColor);
    return result;
}


inline void UIDrawBookmark(UIState* UI, RenderGroup* group, UIBookmark* bookmark, BitmapId BID, Vec2 P, Vec2 dim, Vec2 activeOffset)
{
    Vec4 bookmarkColor = V4(1, 1, 1, 1);
    
    Vec2 drawingP = P;
    Vec2 boundsP = P + 0.5f * activeOffset;
    Vec2 boundsDim = dim;
    if(boundsDim.x > boundsDim.y)
    {
        boundsDim.x = 0.5f * boundsDim.x;
    }
    else
    {
        boundsDim.y = 0.5f * boundsDim.y;
    }
    
    if(bookmark->active)
    {
        drawingP += activeOffset;
        boundsP = P + activeOffset;
        boundsDim = dim;
    }
    
    Rect2 screenBookmarkRect = ProjectOnScreenCameraAligned(group, V3(0, 0, 0), RectCenterDim(boundsP + group->gameCamera.screenCameraOffset, boundsDim));
    if(PointInRect(screenBookmarkRect, UI->screenMouseP))
    {
        bookmarkColor = V4(0, 1, 1, 1);
        UI->hotBookmark = bookmark;
    }
    
    r32 angle = 0;
    if(bookmark->position == UIBookmark_RightSide || bookmark->position == UIBookmark_LeftSide)
    {
        angle = 90.0f;
    }
    
    PushUIBitmap(group, BID, drawingP, max(dim.x, dim.y), angle, 9.5f, V2(1, 1), bookmarkColor);
}

internal b32 UIDrawPage(UIState* UI, Vec2 pageP, Vec2 pageDim, u32 startingElementIndex, u32 elementCount, PlatformInput* input)
{
	b32 onFocus = false;
    
    GameModeWorld* worldMode = UI->worldMode;
    BookMode* bookMode = UI->bookModes + UI->currentBookModeIndex;
    RenderGroup* group = UI->group;
    
    Vec2 elementDim = V2(pageDim.x, pageDim.y / (r32) elementCount);
    Vec2 elementOffset = V2(0, 0.5f * pageDim.y) - V2(0, 0.5f * elementDim.y);
    
    Vec3 P = GetWorldP(group, pageP);
    Rect2 pageRect = RectCenterDim(V2(0, 0), elementDim);
    
    r32 ignored;
	Rect2 pageScreenRect = ProjectOnScreenCameraAligned(group, P, pageRect, &ignored);
	if(PointInRect(pageScreenRect, UI->screenMouseP))
	{
		onFocus = true;
	}
    
    u32 elementToDrawCount = 0;
    BookElement* elementsToDraw[8] = {};
    
    Assert(elementCount < ArrayCount(elementsToDraw));
    
    u32 runningRecipeCount = 0;
    BookReference* reference = bookMode->filteredElements;
    while(reference && (elementToDrawCount < elementCount))
    {
        if(startingElementIndex < ++runningRecipeCount)
        {
            elementsToDraw[elementToDrawCount++] = reference->element;
        }
        
        reference = reference->next;
    }
    
    for(u32 elementIndex = 0; elementIndex < elementCount; ++elementIndex)
    {
        BookElement* toDraw = elementsToDraw[elementIndex];
        
        if(toDraw)
        {
            Assert(toDraw->taxonomy);
            Vec2 elementCenterP = pageP + elementOffset;
            if(toDraw->hot)
            {
                
#if 0                
                r32 destTimer = 2.0f;
                r32 securityTimerQuarter = destTimer * 0.25f;
                r32 oneOverSecurityTimerQuarter = 1.0f / securityTimerQuarter;
                r32 lowBarPercentage = Clamp01(toDraw->securityTimer * oneOverSecurityTimerQuarter);
                r32 rightBarPercentage = Clamp01((toDraw->securityTimer - 1.0f * securityTimerQuarter)* oneOverSecurityTimerQuarter);
                r32 highBarPercentage = Clamp01((toDraw->securityTimer - 2.0f * securityTimerQuarter)* oneOverSecurityTimerQuarter);
                r32 leftBarPercentage = Clamp01((toDraw->securityTimer - 3.0f * securityTimerQuarter)* oneOverSecurityTimerQuarter);
                
                ObjectTransform barTransform = UprightTransform();
                barTransform.additionalZBias = 12.5f;
                
                r32 barDim = 0.05f;
                
                r32 maxElementWidth = 0.75f;
                r32 maxElementHeight = 0.85f;
                
                // NOTE(Leonardo): low
                r32 barWidth = lowBarPercentage * maxElementWidth * elementDim.x;
                barTransform.offset = V3(-0.5f * maxElementWidth * elementDim.x, -0.5f * maxElementHeight * elementDim.y, 0) + V3(0.5f * barWidth, 0.5f * barDim, 0);
                PushRect(UI->group, barTransform, elementCenterP, V2(barWidth, barDim), V4(1, 0, 0, 1));
                
                
                // NOTE(Leonardo): right
                r32 barHeight = rightBarPercentage * maxElementHeight * elementDim.y;
                barTransform.offset = V3(0.5f * maxElementWidth * elementDim.x, -0.5f * maxElementHeight * elementDim.y, 0) + V3(-0.5f * barDim, 0.5f * barHeight, 0);
                PushRect(UI->group, barTransform, elementCenterP, V2(barDim, barHeight), V4(1, 0, 0, 1));
                
                
                // NOTE(Leonardo): up
                barWidth = highBarPercentage * maxElementWidth * elementDim.x;
                barTransform.offset = V3(0.5f * maxElementWidth * elementDim.x, 0.5f * maxElementHeight * elementDim.y, 0) + V3(-0.5f * barWidth, -0.5f * barDim, 0);
                PushRect(UI->group, barTransform, elementCenterP, V2(barWidth, barDim), V4(1, 0, 0, 1));
                
                
                // NOTE(Leonardo): left
                barHeight = leftBarPercentage * maxElementHeight * elementDim.y;
                barTransform.offset = V3(-0.5f * maxElementWidth * elementDim.x, 0.5f * maxElementHeight * elementDim.y, 0) + V3(0.5f * barDim, -0.5f * barHeight, 0);
                PushRect(UI->group, barTransform, elementCenterP, V2(barDim, barHeight), V4(1, 0, 0, 1));
#endif
                
            }
            
            switch(toDraw->type)
            {
                case Book_Recipe:
                {
                    u32 taxonomy = toDraw->taxonomy;
                    GenerationData gen = toDraw->gen;
                    
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, taxonomy);
                    UIDrawRecipeElement(UI, toDraw, elementCenterP, elementDim, slot, gen, input);
                    
                } break;
                
                case Book_RecipeCategory:
                {
                    u32 taxonomy = toDraw->taxonomy;
                    
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, taxonomy);
                    UIDrawRecipeCategoryElement(UI, toDraw, elementCenterP, elementDim, slot, input);
                } break;
                
                case Book_Skill:
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, toDraw->taxonomy);
                    UIDrawSkillElement(UI, toDraw, elementCenterP, elementDim, slot, toDraw->skillLevel, input);
                    
                } break;
                
                case Book_SkillCategory:
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, toDraw->taxonomy);
                    UIDrawSkillCategoryElement(UI, toDraw, elementCenterP, elementDim, slot, input);
                    
                } break;
                
                InvalidDefaultCase;
            }
            elementOffset.y -= elementDim.y;
        }
    }
    
	return onFocus;
}


internal b32 UpdateAndRenderBook(UIState* UI, PlatformInput* input)
{
	b32 onFocus = false;
    
    GameModeWorld* worldMode = UI->worldMode;
    RenderGroup* group = UI->group;
    BitmapId bookID = GetFirstBitmap(group->assets, Asset_BookPage);
    
    Bitmap* bookPage = GetBitmap(group->assets, bookID);
    Vec2 bookLeft = {};
    Vec2 bookRight = {};
    r32 pageWidth = 0;
    r32 pageHeight = 0;
    
    if(bookPage)
    {
        pageHeight = 8.7f;
        pageWidth = pageHeight * bookPage->widthOverHeight;
        
        bookLeft = V2(-0.5f * pageWidth, 0);
        bookRight = V2(0.5f * pageWidth, 0);
        
        PushUIBitmap(group, bookID, bookLeft, pageHeight, 0, 12.0f, V2(1.0f, 1.0f));
        PushUIBitmap(group, bookID, bookRight, pageHeight, 0, 12.0f, V2(-1.0f, 1.0f));
        
        BitmapId bookmarkID = GetFirstBitmap(group->assets, Asset_Bookmark);
        Bitmap* bitmap = GetBitmap(group->assets, bookmarkID);
        if(bitmap)
        {
            r32 bookmarkHeight = pageHeight / 10.0f;
            r32 bookmarkWidth = bookmarkHeight * bitmap->widthOverHeight;
            
            Vec2 position[UIBookmark_Count];
            Vec2 delta[UIBookmark_Count];
            Vec2 dimension[UIBookmark_Count];
            Vec2 activeOffset[UIBookmark_Count];
            
            position[UIBookmark_RightUp] = V2(1.0f * bookmarkWidth, 0.5f * pageHeight);
            delta[UIBookmark_RightUp] = V2( (pageWidth - 0.5f * bookmarkWidth) / UI->bookmarkCount[UIBookmark_RightUp], 0);
            dimension[UIBookmark_RightUp] = V2(bookmarkWidth, bookmarkHeight);
            activeOffset[UIBookmark_RightUp] = V2(0, 0.5f * bookmarkHeight);
            
            position[UIBookmark_LeftUp] = V2(-pageWidth + 1.0f * bookmarkWidth, 0.5f * pageHeight);
            delta[UIBookmark_LeftUp] =  V2( (pageWidth - 0.5f * bookmarkWidth) / UI->bookmarkCount[UIBookmark_LeftUp], 0);
            dimension[UIBookmark_LeftUp] = V2(bookmarkWidth, bookmarkHeight);
            activeOffset[UIBookmark_LeftUp] = V2(0, 0.5f * bookmarkHeight);
            
            
            r32 sideBookmarkWidth = bookmarkHeight;
            r32 sideBookmarkHeight = bookmarkWidth;
            
            
            position[UIBookmark_RightSide] = V2(pageWidth, 0.5f * pageHeight - 0.5f * sideBookmarkHeight);
            delta[UIBookmark_RightSide] = V2(0, -(pageHeight - sideBookmarkHeight) / UI->bookmarkCount[UIBookmark_RightSide]);
            dimension[UIBookmark_RightSide] = V2(sideBookmarkWidth, sideBookmarkHeight);
            activeOffset[UIBookmark_RightSide] = V2(0.5f * sideBookmarkWidth, 0);
            
            position[UIBookmark_LeftSide] = V2(-pageWidth, 0.5f * pageHeight - 0.5f * sideBookmarkHeight);
            delta[UIBookmark_LeftSide] = V2(0, -(pageHeight - sideBookmarkHeight) / UI->bookmarkCount[UIBookmark_LeftSide]);
            dimension[UIBookmark_LeftSide] = V2(sideBookmarkWidth, sideBookmarkHeight);
            activeOffset[UIBookmark_LeftSide] = V2(-0.5f * sideBookmarkWidth, 0);
            
            
            UI->hotBookmark = 0;
            for(u32 bookmarkIndex = 0; bookmarkIndex < UI->totalBookmarkCount; ++bookmarkIndex)
            {
                
                UIBookmark* bookmark = UI->bookmarks + bookmarkIndex;
                UIDrawBookmark(UI, group, bookmark, bookmarkID, position[bookmark->position], dimension[bookmark->position], activeOffset[bookmark->position]);
                
                position[bookmark->position] += delta[bookmark->position];
            }
            
            if(UI->hotBookmark)
            {
                if(Pressed(&input->mouseLeft))
                {
                    UIDispatchBookmarkCondition(UI, UI->hotBookmark);
                }
                
				onFocus = true;
            }
        }
        else
        {
            LoadBitmap(group->assets, bookmarkID, false);
        }
    }
    else
    {
        LoadBitmap(group->assets, bookID, false);
    }
    
    
    
    BookMode* bookMode = UI->bookModes + UI->currentBookModeIndex;
    
    FREELIST_FREE(bookMode->filteredElements, BookReference, UI->firstFreeReference);
    bookMode->filteredElementCount = 0;
    
    b32 oneAtLeast = false;
    BookElementsBlock* toFilter = bookMode->elements;
    while(toFilter)
    {
        for(u32 elementIndex = 0; elementIndex < toFilter->elementCount; ++elementIndex)
        {
            BookElement* element = toFilter->elements + elementIndex;
            if(UIConditionSatisfied(UI, bookMode, element))
            {
                AddToFilteredElements(UI, bookMode, element);
                oneAtLeast = true;
            }
        }
        
        toFilter = toFilter->next;
    }
    
    if(Pressed(&input->mouseRight))
    {
        BookMode* activeBookMode = UI->bookModes + UI->currentBookModeIndex;
        if(activeBookMode->filterTaxonomy != activeBookMode->rootTaxonomy)
        {
            activeBookMode->filterTaxonomy = GetParentTaxonomy(UI->table, activeBookMode->filterTaxonomy);
        }
        else
        {
            UI->nextMode = UIMode_None;
        }
    }
    
    u32 elementsPerPage = 1;
    u32 viewingElements = elementsPerPage * 2;
    i32 offset = viewingElements * input->mouseWheelOffset;
    
    i64 newIndex = (i32) UI->currentElementBookIndex + offset;
    if(newIndex >= 0 && newIndex < bookMode->filteredElementCount)
    {
        i32 finalIndex = SafeTruncateInt64ToInt32(newIndex);
        Assert(finalIndex >= 0);
        UI->currentElementBookIndex = (u32) finalIndex;
    }
    else if(newIndex < 0)
    {
        UI->currentElementBookIndex = 0;
    }
    else if(newIndex >= bookMode->filteredElementCount)
    {
        u32 moddedIndex = bookMode->filteredElementCount % viewingElements;
        if(!moddedIndex && bookMode->filteredElementCount)
        {
            moddedIndex = viewingElements;
        }
        
        u32 toAdd = viewingElements - moddedIndex;
        u32 fakeModdedIndex = bookMode->filteredElementCount + toAdd;
        Assert(fakeModdedIndex >= viewingElements);
        UI->currentElementBookIndex = fakeModdedIndex - viewingElements;
    }
    
    
    u32 leftPageElementIndex = UI->currentElementBookIndex;
    u32 rightPageElementIndex = leftPageElementIndex + elementsPerPage;
    
    if(bookPage)
    {
        b32 p1OnFocus = UIDrawPage(UI, bookLeft, V2(pageWidth, pageHeight), leftPageElementIndex, elementsPerPage, input);
        b32 p2OnFocus = UIDrawPage(UI, bookRight, V2(pageWidth, pageHeight), rightPageElementIndex, elementsPerPage, input);
        
		onFocus = onFocus || p1OnFocus || p2OnFocus;
    }
    
    
	return onFocus;
}

