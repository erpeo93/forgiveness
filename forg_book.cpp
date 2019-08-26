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

inline void AddToFilteredElements(UIState* UI, BookMode* bookMode, BookElement* element)
{
    BookReference* reference;
    FREELIST_ALLOC(reference, UI->firstFreeReference, PushStruct(UI->pool, BookReference));
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

internal b32 UIDrawRecipeElement(UIState* UI, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* recipeSlot, GenerationData gen, PlatformInput* input)
{
    b32 result = false;
    
    RenderGroup* group = UI->group;
    
    Object object = {};
    object.taxonomy = recipeSlot->taxonomy;
    object.gen = gen;
    r32 additionalZBias = 20.1f;
    Vec3 objectP = GetWorldP(group, elementCenterP + V2(0.3f * elementDim.x, 0));
    RenderObject(group, UI->worldMode, &object, objectP, V2(0.4f * elementDim.x, 0.6f * elementDim.y), additionalZBias);
    
    Vec2 objectTextP = elementCenterP + V2(-0.1f * elementDim.x, 0.25f * elementDim.y);
    PushUITextWithDimension(UI, &UI->gameFont, recipeSlot->name, objectTextP, V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
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
            
            PushUITextWithDimension(UI, &UI->gameFont, toolSlot->name, toolTextP, toolTextDim, color);
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
            owned = 0;
            for(u32 essenceIndex = 0; essenceIndex < MAX_DIFFERENT_ESSENCES; ++essenceIndex)
            {
                EssenceSlot* essence = UI->myPlayer->essences + essenceIndex;
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
        PushUITextWithDimension(UI, &UI->gameFont, quantities, ingredientQuantityP, V2(0.5f * ingredientDim.x, 0.3f * ingredientDim.y), quantitiesColor);
        
        ingredientP.x += ingredientDim.x;
    }
    
    
    
    
    
    
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    if(activeElement && canCraft && renderCraftTooltip)
    {
        PushUITooltip(UI, "craft", V4(1, 0, 0, 1));
        if(element->hot)
        {
            element->securityTimer += input->timeToAdvance;
            r32 destTimer = 2.0f;
            r32 timeToComeback = 2.0f;
            if(element->securityTimer >= destTimer)
            {
                element->securityTimer = destTimer + timeToComeback;
                SendCraftRequest(recipeSlot->taxonomy, gen);
                //ActionBeganPrediciton(UI->player, Action_Craft);
                UI->nextMode = UIMode_None;
                element->hot = false;
            }
            
        }
        else
        {
            element->securityTimer -= input->timeToAdvance;
            element->securityTimer = Max(element->securityTimer, 0.0f);
            
            UIInteraction craftInteraction = {};
            UIAddSetValueAction(UI, &craftInteraction, UI_Trigger, &element->hot, true);
            UIAddSetValueAction(UI, &craftInteraction, UI_Release, &element->hot, false);
            UIAddInteraction(UI, input, mouseLeft, craftInteraction);
        }
    }
    else
    {
        element->securityTimer -= input->timeToAdvance;
        element->securityTimer = Max(element->securityTimer, 0.0f);
        element->hot = false;
    }
    
    return result;
}

internal b32 UIDrawRecipeCategoryElement(UIState* UI, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* categorySlot, PlatformInput* input)
{
    b32 result = false;
    
    RenderGroup* group = UI->group;
    
    
    Vec2 categoryTextP = elementCenterP + V2(-0.1f * elementDim.x, 0.25f * elementDim.y);
    PushUITextWithDimension(UI, &UI->gameFont, categorySlot->name, categoryTextP, V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
    
    
    
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    if(activeElement)
    {
        PushUITooltip(UI, "drill down", V4(1, 0, 0, 1));
        BookMode* activeBookMode = UI->bookModes + UI->currentBookModeIndex;
        UIInteraction recipeCatInteraction = {};
        UIAddSetValueAction(UI, &recipeCatInteraction, UI_Trigger, &activeBookMode->filterTaxonomy, categorySlot->taxonomy);
        UIAddInteraction(UI, input, mouseLeft, recipeCatInteraction);
    }
    
    return result;
}

internal void UIDrawSkillElement(UIState* UI, BookMode* bookMode, Vec3 bookP, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* skillSlot, PlatformInput* input)
{
    RenderGroup* group = UI->group;
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    
    
    
    Rect2 rect = RectCenterDim(elementCenterP + 0.4f *V2(0, elementDim.y), V2(0.8f * elementDim.x, 0.2f * elementDim.y));
    
    FitTextResult fitSkill = FitTextIntoRect(UI, &UI->gameFont, rect, skillSlot->name, 2.0f);
    PushUIText_(UI, &UI->gameFont, skillSlot->name, fitSkill.row.P, bookMode->defaultTextColor, fitSkill.fontScale);
    
    
    
    Rect2 levelRect = RectCenterDim(elementCenterP + 0.0f *V2(0, elementDim.y), V2(0.8f * elementDim.x, 0.2f * elementDim.y));
    char stringLevel[16];
    FormatString(stringLevel, sizeof(stringLevel), "level: %d", element->skillLevel);
    
    FitTextResult fitLevel = FitTextIntoRect(UI, &UI->gameFont, levelRect, stringLevel, 1.5f);
    PushUIText_(UI, &UI->gameFont, stringLevel, fitLevel.row.P, bookMode->defaultTextColor, fitLevel.fontScale);
    
    
    
    Rect2 modelRect = RectCenterDim(elementCenterP - 0.25f *V2(0, elementDim.y), V2(0.8f * elementDim.x, 0.2f * elementDim.y));
    u64 modelTypeID = skillSlot->iconModelTypeID;
    u64 modelNameID = skillSlot->iconModelNameID;
    Vec4 standardColor = skillSlot->iconActiveColor;
    ModelId MID = FindModelByName(group->assets, modelTypeID, modelNameID);
    FitProjectedModelResult fitModel = FitProjectedModelIntoRect(UI, bookP, modelRect, MID);
    
    PushModel(group, MID, Identity(), fitModel.modelP, {}, fitModel.modelScale, standardColor, 0, PAGE_ZBIAS + 0.02f);
    
    
    
    
    
    
    
    
    
    if(activeElement)
    {
        if(HasEssences(UI->myPlayer->essences, skillSlot->firstEssence))
        {
            PushUITooltip(UI, "level up", V4(1, 0, 0, 1));
            
            if(element->hot)
            {
                element->securityTimer += input->timeToAdvance;
                r32 destTimer = 2.0f;
                r32 timeToComeback = 2.0f;
                
                if(element->securityTimer >= destTimer)
                {
                    element->securityTimer = destTimer + timeToComeback;
                    SendSkillLevelUpRequest(skillSlot->taxonomy);
                    element->hot = false;
                }
            }
            else
            {
                element->securityTimer -= input->timeToAdvance;
                element->securityTimer = Max(element->securityTimer, 0.0f);
                
                UIInteraction levelUpInteraction = {};
                UIAddSetValueAction(UI, &levelUpInteraction, UI_Trigger, &element->hot, true);
                UIAddSetValueAction(UI, &levelUpInteraction, UI_Release, &element->hot, false);
                UIAddInteraction(UI, input, mouseLeft, levelUpInteraction);
            }
        }
        else
        {
            element->securityTimer -= input->timeToAdvance;
            element->securityTimer = Max(element->securityTimer, 0.0f);
            element->hot = false;
        }
        
        
        for(i32 slotButtonIndex = 0; slotButtonIndex < MAXIMUM_SKILL_SLOTS; ++slotButtonIndex)
        {
            
            UIInteraction slotInteraction = {};
            
            if(skillSlot->isPassiveSkill)
            {
                UIAddRequestAction(UI, &slotInteraction, UI_Trigger, PassiveSkillRequest(skillSlot->taxonomy));
            }
            else
            {
                b32 sendActiveRequest = (slotButtonIndex == UI->activeSkillSlotIndex);
                UIAddRequestAction(UI, &slotInteraction, UI_Trigger, ActiveSkillRequest(skillSlot->taxonomy, sendActiveRequest, true));
            }
            
            UISkill* skill = UI->skills + slotButtonIndex;
            UIAddSetValueAction(UI, &slotInteraction, UI_Trigger, &skill->taxonomy, skillSlot->taxonomy);
            UIAddWriteFileAction(UI, &slotInteraction, UI_Trigger, "skills", UI->skills, sizeof(UI->skills));
            
            UIAddInteraction(UI, input, slotButtons[slotButtonIndex + 1], slotInteraction);
        }
    }
    else
    {
        element->securityTimer -= input->timeToAdvance;
        element->securityTimer = Max(element->securityTimer, 0.0f);
        element->hot = false;
    }
}


internal void UIDrawSkillCategoryElement(UIState* UI, BookElement* element, Vec2 elementCenterP, Vec2 elementDim, TaxonomySlot* categorySlot, PlatformInput* input)
{
    RenderGroup* group = UI->group;
    b32 activeElement = UIElementActive(UI, group, elementCenterP, elementDim);
    
    
    Vec2 categoryTextP = elementCenterP + V2(-0.1f * elementDim.x, 0.25f * elementDim.y);
    PushUITextWithDimension(UI, &UI->gameFont, categorySlot->name, categoryTextP, V2(0.3f * elementDim.x, 0.1f * elementDim.y), V4(1, 0, 0, 1));
    
    
    
    if(activeElement)
    {
        if(element->unlocked)
        {
            PushUITooltip(UI, "drill down", V4(1, 0, 0, 1));
            
            UIInteraction drillInteraction = {};
            BookMode* activeBookMode = UI->bookModes + UI->currentBookModeIndex;
            UIAddSetValueAction(UI, &drillInteraction, UI_Trigger, &activeBookMode->filterTaxonomy, categorySlot->taxonomy);
            UIAddInteraction(UI, input, mouseLeft, drillInteraction);
            
            element->securityTimer -= input->timeToAdvance;
            element->securityTimer = Max(element->securityTimer, 0.0f);
            element->hot = false;
        }
        else
        {
            if(HasEssences(UI->myPlayer->essences, categorySlot->firstEssence))
            {
                PushUITooltip(UI, "unlock", V4(1, 0, 0, 1));
                if(element->hot)
                {
                    element->securityTimer += input->timeToAdvance;
                    r32 destTimer = 2.0f;
                    r32 timeToComeback = 2.0f;
                    if(element->securityTimer >= destTimer)
                    {
                        SendUnlockSkillCategoryRequest(categorySlot->taxonomy);
                        element->securityTimer = destTimer + timeToComeback;
                        element->hot = false;
                    }
                }
                else
                {
                    element->securityTimer -= input->timeToAdvance;
                    element->securityTimer = Max(element->securityTimer, 0.0f);
                    
                    UIInteraction unlockInteraction = {};
                    UIAddSetValueAction(UI, &unlockInteraction, UI_Trigger, &element->hot, true);
                    UIAddSetValueAction(UI, &unlockInteraction, UI_Release, &element->hot, false);
                    UIAddInteraction(UI, input, mouseLeft, unlockInteraction);
                }
                
            }
            else
            {
                element->securityTimer -= input->timeToAdvance;
                element->securityTimer = Max(element->securityTimer, 0.0f);
                element->hot = false;
            }
        }
    }
    else
    {
        element->securityTimer -= input->timeToAdvance;
        element->securityTimer = Max(element->securityTimer, 0.0f);
        element->hot = false;
    }
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

internal b32 UIDrawPage(UIState* UI, Vec3 bookP, Vec2 pageP, Vec2 pageDim, u32 startingElementIndex, u32 elementCount, PlatformInput* input)
{
	b32 onFocus = false;
    
    GameModeWorld* worldMode = UI->worldMode;
    BookMode* bookMode = UI->bookModes + UI->currentBookModeIndex;
    RenderGroup* group = UI->group;
    
    Vec2 elementDim = V2(pageDim.x, pageDim.y / (r32) elementCount);
    Vec2 elementOffset = V2(0, 0.5f * pageDim.y) - V2(0, 0.5f * elementDim.y);
    elementOffset = {};
    
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
            Vec4 decorationBaseColor = {};
            switch(toDraw->type)
            {
                case Book_Recipe:
                {
                    u32 taxonomy = toDraw->taxonomy;
                    GenerationData gen = toDraw->gen;
                    
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, taxonomy);
                    UIDrawRecipeElement(UI, toDraw, elementCenterP, elementDim, slot, gen, input);
                    decorationBaseColor = V4(0, 0, 1, 0.3f);
                } break;
                
                case Book_RecipeCategory:
                {
                    u32 taxonomy = toDraw->taxonomy;
                    
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, taxonomy);
                    UIDrawRecipeCategoryElement(UI, toDraw, elementCenterP, elementDim, slot, input);
                    decorationBaseColor = V4(0, 1, 1, 0.3f);
                } break;
                
                case Book_Skill:
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, toDraw->taxonomy);
                    UIDrawSkillElement(UI, bookMode, bookP, toDraw, elementCenterP, elementDim, slot, input);
                    decorationBaseColor = V4(1, 0, 0, 0.3f);
                    
                } break;
                
                case Book_SkillCategory:
                {
                    TaxonomySlot* slot = GetSlotForTaxonomy(UI->table, toDraw->taxonomy);
                    UIDrawSkillCategoryElement(UI, toDraw, elementCenterP, elementDim, slot, input);
                    decorationBaseColor = V4(1, 1, 0, 0.3f);
                } break;
                
                InvalidDefaultCase;
            }
            
            r32 destTimer = 2.0f;
            r32 currentTimer = Min(toDraw->securityTimer, destTimer);
            r32 lerp = currentTimer / destTimer;
            
            decorationBaseColor.a = Lerp(decorationBaseColor.a, lerp, 1.0f);
            
            
            BitmapId bookElementID = GetFirstBitmap(group->assets, Asset_BookElement);
            PushUIBitmap(group, bookElementID, elementCenterP, elementDim.y, 0, PAGE_ZBIAS + 0.01f, V2(1.0f, 1.0f), decorationBaseColor);
            
            
            
            elementOffset.y -= elementDim.y;
        }
    }
    
	return onFocus;
}

internal b32 UpdateAndRenderBook(UIState* UI, PlatformInput* input, Vec2 bookP)
{
    for(u32 bookModeIndex = 0; bookModeIndex < UIBook_Count; ++bookModeIndex)
    {
        UI->bookModes[bookModeIndex].defaultTextColor = V4(0, 0, 0, 1);
    }
    
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
        
        bookLeft = bookP + V2(-0.5f * pageWidth, 0);
        bookRight = bookP + V2(0.5f * pageWidth, 0);
        
        PushUIBitmap(group, bookID, bookLeft, pageHeight, 0, PAGE_ZBIAS, V2(1.0f, 1.0f));
        PushUIBitmap(group, bookID, bookRight, pageHeight, 0, PAGE_ZBIAS, V2(-1.0f, 1.0f));
        
        BitmapId bookmarkID = GetFirstBitmap(group->assets, Asset_Bookmark);
        Bitmap* bitmap = GetBitmap(group->assets, bookmarkID);
        if(bitmap)
        {
            r32 bookmarkHeight = pageHeight / 6.0f;
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
                UIDrawBookmark(UI, group, bookmark, bookmarkID, bookP + position[bookmark->position], dimension[bookmark->position], activeOffset[bookmark->position]);
                
                position[bookmark->position] += delta[bookmark->position];
            }
            
            if(UI->hotBookmark)
            {
                UIInteraction bookmarkInteraction = {};
                UIAddDispatchBookmarkConditionAction(UI, &bookmarkInteraction, UI_Trigger, UI->hotBookmark);
                UIAddInteraction(UI, input, mouseLeft, bookmarkInteraction);
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
    
    UIInteraction rightMouseInteraction = {};
    
    BookMode* activeBookMode = UI->bookModes + UI->currentBookModeIndex;
    if(activeBookMode->filterTaxonomy != activeBookMode->rootTaxonomy)
    {
        u32 parentTaxonomy = GetParentTaxonomy(UI->table, activeBookMode->filterTaxonomy);
        UIAddSetValueAction(UI, &rightMouseInteraction, UI_Trigger, &activeBookMode->filterTaxonomy, parentTaxonomy);
    }
    else
    {
        UIAddSetValueAction(UI, &rightMouseInteraction, UI_Trigger, &UI->bookOutTime, 0.0f);
        UIAddSetValueAction(UI, &rightMouseInteraction, UI_Trigger, &UI->exitingFromBookMode, true);
        UIAddPlaySoundEventAction(UI, &rightMouseInteraction, UI_Trigger, UISound_BookClose);
    }
    
    UIAddInteraction(UI, input, mouseRight, rightMouseInteraction);
    
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
        Vec3 P3d = V3(bookP, 0) + group->gameCamera.screenCameraOffset.x * group->gameCamera.X + group->gameCamera.screenCameraOffset.y * group->gameCamera.Y;
        b32 p1OnFocus = UIDrawPage(UI, P3d, bookLeft, V2(pageWidth, pageHeight), leftPageElementIndex, elementsPerPage, input);
        b32 p2OnFocus = UIDrawPage(UI, P3d, bookRight, V2(pageWidth, pageHeight), rightPageElementIndex, elementsPerPage, input);
        
		onFocus = onFocus || p1OnFocus || p2OnFocus;
    }
    
    
	return onFocus;
}

