// T3SkillSlotWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/T3SkillComponentBase.h"
#include "T3SkillSlotWidget.generated.h"

class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkillSlotClicked, int32, SkillID);

// 스킬 그리드/장착 행에서 개별 슬롯 하나를 표현하는 위젯.
// 장착 슬롯(상단)과 전체 스킬 그리드(하단) 양쪽에 동일하게 사용한다.
// 시각 표현(아이콘, 잠금 오버레이, 선택 하이라이트 등)은 BP 서브클래스에서 구현한다.
UCLASS()
class DESECRATION_API UT3SkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 이 슬롯이 나타내는 스킬 ID. 0이면 빈 슬롯(잠금 표시).
	UPROPERTY(BlueprintReadOnly, Category = "Skill Slot")
	int32 SkillID = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Slot")
	bool bIsUnlocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Slot")
	bool bIsEquipped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Skill Slot")
	bool bIsSelected = false;

	// true = 상단 장착 행 슬롯 / false = 하단 그리드 슬롯
	// BuildEquippedRow()에서 true로 세팅됨
	UPROPERTY(BlueprintReadWrite, Category = "Skill Slot")
	bool bIsEquippedRowSlot = false;

	// 캐시된 스킬 데이터 (아이콘·이름·설명 등 BP에서 참조)
	UPROPERTY(BlueprintReadOnly, Category = "Skill Slot")
	FSkillData CachedSkillData;

	// SkillWindowWidget에서 호출하여 슬롯 데이터를 세팅한다.
	UFUNCTION(BlueprintCallable, Category = "Skill Slot")
	void SetupSlot(int32 InSkillID, const FSkillData& InSkillData, bool bUnlocked, bool bEquipped);

	UFUNCTION(BlueprintCallable, Category = "Skill Slot")
	void SetSelectedState(bool bSelected);

	UFUNCTION(BlueprintCallable, Category = "Skill Slot")
	void SetEquippedState(bool bEquipped);

	// 클릭 시 SkillWindowWidget으로 전달
	UPROPERTY(BlueprintAssignable, Category = "Skill Slot")
	FOnSkillSlotClicked OnSkillSlotClicked;

protected:

	// BP에서 구현 — 슬롯 데이터가 바뀔 때 아이콘·잠금 오버레이 등을 갱신
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Slot")
	void BP_OnSlotSetup(const FSkillData& SkillData, bool bUnlocked, bool bEquipped);

	// BP에서 구현 — 선택 상태 시각화 (하이라이트 on/off)
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Slot")
	void BP_OnSelectedChanged(bool bSelected);

	// BP에서 구현 — 장착 상태 시각화 (테두리 등)
	UFUNCTION(BlueprintImplementableEvent, Category = "Skill Slot")
	void BP_OnEquippedChanged(bool bEquipped);

	// 마우스 클릭 → OnSkillSlotClicked 브로드캐스트
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
