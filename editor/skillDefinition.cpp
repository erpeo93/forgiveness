internal void ImportSkillDefinitionTab()
{
    r32 distance = ToR32(GetValue(root, "distance"), 1.0f);
    currentSlot_->skillDistanceAllowed = distance;
    currentSlot_->cooldown = ToR32(GetValue(root, "cooldown"), 0.0f);
    
    currentSlot_->turningPointLevel = ToU32(GetValue(root, "turningPointLevel"), 0);
    currentSlot_->maxLevel = ToU32(GetValue(root, "maxLevel"), 100);
    
    currentSlot_->radixExponent = ToR32(GetValue(root, "radixExponent"), 2.0f);
    currentSlot_->exponentiationExponent = Max(ToR32(GetValue(root, "exponentiationExponent"), 2.0f), 0.0f);
    currentSlot_->radixLerping = ToR32(GetValue(root, "radixLerping"), 0.5f);
    currentSlot_->exponentiationLerping = Max(ToR32(GetValue(root, "exponentiationLerping"), 0.5f), 0.0f);
    
    char* passive = GetValue(root, "passive");
    if(passive)
    {
        InvalidCodePath;
        currentSlot_->isPassiveSkill = true;
        IsPassive();
    }
    
}