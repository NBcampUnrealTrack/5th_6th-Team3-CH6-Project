// T3SkillSlotWidget.cpp

#include "UI/T3SkillSlotWidget.h"

void UT3SkillSlotWidget::SetupSlot(int32 InSkillID, const FSkillData& InSkillData, bool bUnlocked, bool bEquipped)
{
	SkillID = InSkillID;
	CachedSkillData = InSkillData;
	bIsUnlocked = bUnlocked;
	bIsEquipped = bEquipped;

	BP_OnSlotSetup(CachedSkillData, bIsUnlocked, bIsEquipped);
}

void UT3SkillSlotWidget::SetSelectedState(bool bSelected)
{
	if (bIsSelected == bSelected) return;
	bIsSelected = bSelected;
	BP_OnSelectedChanged(bIsSelected);
}

void UT3SkillSlotWidget::SetEquippedState(bool bEquipped)
{
	if (bIsEquipped == bEquipped) return;
	bIsEquipped = bEquipped;
	BP_OnEquippedChanged(bIsEquipped);
}

FReply UT3SkillSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		OnSkillSlotClicked.Broadcast(SkillID);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}
