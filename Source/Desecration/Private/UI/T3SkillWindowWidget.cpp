// T3SkillWindowWidget.cpp

#include "UI/T3SkillWindowWidget.h"
#include "UI/T3SkillSlotWidget.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3SkillComponentBase.h"
#include "Player/T3CommonSkillComponent.h"
#include "Components/WrapBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UT3SkillWindowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!IsDesignTime() || !SkillSlotWidgetClass) return;

	if (EquippedSlotsBox)
	{
		EquippedSlotsBox->ClearChildren();
		for (int32 i = 0; i < UT3SkillComponentBase::MAX_SKILL_SLOTS; ++i)
		{
			if (UUserWidget* NewSlot = CreateWidget<UUserWidget>(this, SkillSlotWidgetClass))
			{
				EquippedSlotsBox->AddChild(NewSlot);
			}
		}
	}

	if (SkillGridBox)
	{
		SkillGridBox->ClearChildren();
		for (int32 i = 0; i < TOTAL_GRID_SLOTS; ++i)
		{
			if (UUserWidget* NewSlot = CreateWidget<UUserWidget>(this, SkillSlotWidgetClass))
			{
				SkillGridBox->AddChild(NewSlot);
			}
		}
	}
}

void UT3SkillWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UT3SkillWindowWidget::NativeDestruct()
{
	UnbindDelegates();
	Super::NativeDestruct();
}

void UT3SkillWindowWidget::InitializeWidget(AT3CharacterBase* InCharacter)
{
	if (!InCharacter) return;

	UnbindDelegates();

	OwnerCharacter = InCharacter;

	if (UT3CombatComponent* Combat = InCharacter->GetCombatComponent())
	{
		ClassSkillComp = Combat->GetSkillComponent();
		CommonSkillComp = Combat->GetCommonSkillComponent();
	}

	if (!ClassSkillComp) return;

	ClassSkillComp->OnSkillEquipStateChanged.AddDynamic(this, &UT3SkillWindowWidget::HandleSkillEquipStateChanged);
	ClassSkillComp->OnSkillUnlockStateChanged.AddDynamic(this, &UT3SkillWindowWidget::HandleSkillUnlockStateChanged);

	if (GridSlots.IsEmpty())
	{
		BuildEquippedRow();
		BuildSkillGrid();
	}
	else
	{
		RefreshEquippedRow();
		RefreshSkillGrid();
	}

	SelectedSkillID = 0;
	UpdateInfoPanel(0);
}

void UT3SkillWindowWidget::RefreshAll()
{
	RefreshEquippedRow();
	RefreshSkillGrid();
	UpdateInfoPanel(SelectedSkillID);
}

// ─────────────────────────────────────────────
// 슬롯 생성 (최초 1회)
// ─────────────────────────────────────────────

void UT3SkillWindowWidget::BuildEquippedRow()
{
	if (!EquippedSlotsBox || !SkillSlotWidgetClass) return;

	EquippedSlotsBox->ClearChildren();
	EquippedDisplaySlots.Empty();

	for (int32 i = 0; i < UT3SkillComponentBase::MAX_SKILL_SLOTS; ++i)
	{
		UT3SkillSlotWidget* SkillSlot = CreateWidget<UT3SkillSlotWidget>(this, SkillSlotWidgetClass);
		if (!SkillSlot) continue;

		SkillSlot->bIsEquippedRowSlot = true;
		SkillSlot->OnSkillSlotClicked.AddDynamic(this, &UT3SkillWindowWidget::OnEquippedSlotClicked);
		EquippedSlotsBox->AddChild(SkillSlot);
		EquippedDisplaySlots.Add(SkillSlot);
	}

	RefreshEquippedRow();
}

void UT3SkillWindowWidget::BuildSkillGrid()
{
	if (!SkillGridBox || !SkillSlotWidgetClass) return;

	SkillGridBox->ClearChildren();
	GridSlots.Empty();

	for (int32 i = 0; i < TOTAL_GRID_SLOTS; ++i)
	{
		UT3SkillSlotWidget* SkillSlot = CreateWidget<UT3SkillSlotWidget>(this, SkillSlotWidgetClass);
		if (!SkillSlot) continue;

		SkillSlot->OnSkillSlotClicked.AddDynamic(this, &UT3SkillWindowWidget::OnGridSlotClicked);
		SkillGridBox->AddChild(SkillSlot);
		GridSlots.Add(SkillSlot);
	}

	RefreshSkillGrid();
}

// ─────────────────────────────────────────────
// 데이터 갱신
// ─────────────────────────────────────────────

void UT3SkillWindowWidget::RefreshEquippedRow()
{
	if (!ClassSkillComp) return;

	for (int32 i = 0; i < EquippedDisplaySlots.Num(); ++i)
	{
		UT3SkillSlotWidget* SkillSlot = EquippedDisplaySlots[i];
		if (!SkillSlot) continue;

		int32 ID = ClassSkillComp->GetSkillIDBySlotIndex(i + 1);
		FSkillData* Data = GetSkillDataByID(ID);
		FSkillData SafeData = Data ? *Data : FSkillData();

		bool bUnlocked = (ID != 0) && ClassSkillComp->IsSkillUnlocked(ID);
		SkillSlot->SetupSlot(ID, SafeData, bUnlocked, /*bEquipped=*/ID != 0);
		SkillSlot->SetSelectedState(ID != 0 && ID == SelectedSkillID);
	}
}

void UT3SkillWindowWidget::RefreshSkillGrid()
{
	if (!ClassSkillComp) return;

	for (int32 i = 0; i < GridSlots.Num(); ++i)
	{
		UT3SkillSlotWidget* SkillSlot = GridSlots[i];
		if (!SkillSlot) continue;

		// 0~3  → 직업 스킬 ID 1~4
		// 4~7  → 보스 스킬 ID 5~8
		// 8~19 → 빈 슬롯 (ID 0)
		int32 ID = 0;
		if (i < CLASS_SKILL_COUNT)
		{
			ID = i + 1;
		}
		else if (i < CLASS_SKILL_COUNT + BOSS_SKILL_COUNT)
		{
			ID = i + 1;
		}

		FSkillData* Data = GetSkillDataByID(ID);
		FSkillData SafeData = Data ? *Data : FSkillData();

		bool bUnlocked = (ID != 0) && ClassSkillComp->IsSkillUnlocked(ID);
		bool bEquipped = (ID != 0) && ClassSkillComp->IsSkillEquipped(ID);

		SkillSlot->SetupSlot(ID, SafeData, bUnlocked, bEquipped);
		SkillSlot->SetSelectedState(ID != 0 && ID == SelectedSkillID);
	}
}

// ─────────────────────────────────────────────
// 우측 정보 패널
// ─────────────────────────────────────────────

void UT3SkillWindowWidget::UpdateInfoPanel(int32 SkillID)
{
	if (SkillID == 0)
	{
		if (InfoSkillName) InfoSkillName->SetText(FText::GetEmpty());
		if (InfoSkillDescription) InfoSkillDescription->SetText(FText::GetEmpty());
		if (InfoSkillManaCost) InfoSkillManaCost->SetText(FText::GetEmpty());
		if (InfoSkillCooldown) InfoSkillCooldown->SetText(FText::GetEmpty());
		if (InfoSkillIcon) InfoSkillIcon->SetBrushFromTexture(nullptr);
		return;
	}

	FSkillData* Data = GetSkillDataByID(SkillID);
	if (!Data) return;

	if (InfoSkillName) InfoSkillName->SetText(Data->SkillName);
	if (InfoSkillDescription) InfoSkillDescription->SetText(Data->Description);
	if (InfoSkillManaCost)
	{
		InfoSkillManaCost->SetText(FText::FromString(FString::Printf(TEXT("마나: %.0f"), Data->ManaCost)));
	}
	if (InfoSkillCooldown)
	{
		InfoSkillCooldown->SetText(FText::FromString(FString::Printf(TEXT("쿨다운: %.1fs"), Data->Cooldown)));
	}
	if (InfoSkillIcon && Data->SkillIcon)
	{
		InfoSkillIcon->SetBrushFromTexture(Data->SkillIcon);
		InfoSkillIcon->SetVisibility(ESlateVisibility::Visible);
	}
}

// ─────────────────────────────────────────────
// 내부 유틸
// ─────────────────────────────────────────────

FSkillData* UT3SkillWindowWidget::GetSkillDataByID(int32 SkillID) const
{
	if (SkillID <= 0) return nullptr;

	if (ClassSkillComp)
	{
		FSkillData* Data = ClassSkillComp->GetSkillDataByID(SkillID);
		if (Data) return Data;
	}
	if (CommonSkillComp)
	{
		return CommonSkillComp->GetSkillDataByID(SkillID);
	}
	return nullptr;
}

int32 UT3SkillWindowWidget::FindGridSlotIndexBySkillID(int32 SkillID) const
{
	for (int32 i = 0; i < GridSlots.Num(); ++i)
	{
		if (GridSlots[i] && GridSlots[i]->SkillID == SkillID)
		{
			return i;
		}
	}
	return -1;
}

void UT3SkillWindowWidget::UnbindDelegates()
{
	if (ClassSkillComp)
	{
		ClassSkillComp->OnSkillEquipStateChanged.RemoveDynamic(this, &UT3SkillWindowWidget::HandleSkillEquipStateChanged);
		ClassSkillComp->OnSkillUnlockStateChanged.RemoveDynamic(this, &UT3SkillWindowWidget::HandleSkillUnlockStateChanged);
	}
}

// ─────────────────────────────────────────────
// 이벤트 핸들러
// ─────────────────────────────────────────────

void UT3SkillWindowWidget::OnGridSlotClicked(int32 SkillID)
{
	if (SkillID == 0) return;

	// 이전 선택 하이라이트 해제
	if (SelectedSkillID != 0)
	{
		int32 PrevIdx = FindGridSlotIndexBySkillID(SelectedSkillID);
		if (GridSlots.IsValidIndex(PrevIdx))
		{
			GridSlots[PrevIdx]->SetSelectedState(false);
		}
	}

	// 잠긴 스킬은 아무것도 하지 않음
	if (!OwnerCharacter || !ClassSkillComp || !ClassSkillComp->IsSkillUnlocked(SkillID)) return;

	SelectedSkillID = SkillID;

	int32 NewIdx = FindGridSlotIndexBySkillID(SelectedSkillID);
	if (GridSlots.IsValidIndex(NewIdx))
	{
		GridSlots[NewIdx]->SetSelectedState(true);
	}

	UpdateInfoPanel(SelectedSkillID);

	bool bEquipped = ClassSkillComp->IsSkillEquipped(SkillID);
	if (UT3CombatComponent* Combat = OwnerCharacter->GetCombatComponent())
	{
		Combat->RequestUpdateSkill(SkillID, !bEquipped);
	}
}

void UT3SkillWindowWidget::OnEquippedSlotClicked(int32 SkillID)
{
	if (SkillID == 0) return;

	// 그리드 하이라이트 동기화
	int32 GridIdx = FindGridSlotIndexBySkillID(SkillID);
	for (int32 i = 0; i < GridSlots.Num(); ++i)
	{
		if (GridSlots[i]) GridSlots[i]->SetSelectedState(i == GridIdx);
	}

	SelectedSkillID = SkillID;
	UpdateInfoPanel(SelectedSkillID);

	// 장착 행 클릭은 항상 탈착
	if (OwnerCharacter)
	{
		if (UT3CombatComponent* Combat = OwnerCharacter->GetCombatComponent())
		{
			Combat->RequestUpdateSkill(SkillID, false);
		}
	}
}

void UT3SkillWindowWidget::HandleSkillEquipStateChanged(int32 SkillID, bool bIsEquipped)
{
	int32 GridIdx = FindGridSlotIndexBySkillID(SkillID);
	if (GridSlots.IsValidIndex(GridIdx))
	{
		GridSlots[GridIdx]->SetEquippedState(bIsEquipped);
	}

	RefreshEquippedRow();
}

void UT3SkillWindowWidget::HandleSkillUnlockStateChanged(int32 SkillID, bool bIsUnlocked)
{
	int32 GridIdx = FindGridSlotIndexBySkillID(SkillID);
	if (GridSlots.IsValidIndex(GridIdx))
	{
		FSkillData* Data = GetSkillDataByID(SkillID);
		FSkillData SafeData = Data ? *Data : FSkillData();
		bool bEquipped = ClassSkillComp ? ClassSkillComp->IsSkillEquipped(SkillID) : false;
		GridSlots[GridIdx]->SetupSlot(SkillID, SafeData, bIsUnlocked, bEquipped);
	}
}
