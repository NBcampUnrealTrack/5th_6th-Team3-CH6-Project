// T3SkillWindowWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/T3SkillComponentBase.h"
#include "T3SkillWindowWidget.generated.h"

class AT3CharacterBase;
class UT3SkillComponentBase;
class UT3CommonSkillComponent;
class UT3SkillSlotWidget;
class UWrapBox;
class UImage;
class UTextBlock;

// 스킬 장착/탈착 전용 창.
//
// 레이아웃:
//   [상단] EquippedSlotsBox  — 현재 장착된 스킬 4슬롯
//   [하단] SkillGridBox      — 전체 스킬 20슬롯 (8개 유효 + 12개 빈 자리)
//   [우측] 선택된 스킬 정보 패널
//
// 그리드 슬롯 클릭: 언락 상태면 장착/탈착 토글. 장착 행 슬롯 클릭: 탈착.
// 사용법: PlayerController에서 열 때 InitializeWidget(CharacterBase) 호출
UCLASS()
class DESECRATION_API UT3SkillWindowWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// 창을 열 때 호출. 스킬 컴포넌트 참조를 캐시하고 전체를 빌드한다.
	UFUNCTION(BlueprintCallable, Category = "Skill Window")
	void InitializeWidget(AT3CharacterBase* InCharacter);

	// 열린 상태에서 외부 변경(언락 등)이 생겼을 때 수동 갱신
	UFUNCTION(BlueprintCallable, Category = "Skill Window")
	void RefreshAll();

protected:

	// ===== BindWidget (BP에서 반드시 같은 이름으로 배치) =====

	// 상단: 장착된 스킬 4슬롯 컨테이너
	UPROPERTY(meta = (BindWidget))
	UWrapBox* EquippedSlotsBox;

	// 하단: 전체 스킬 그리드 컨테이너
	UPROPERTY(meta = (BindWidget))
	UWrapBox* SkillGridBox;

	// 우측 정보 패널
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* InfoSkillIcon;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InfoSkillName;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* InfoSkillDescription;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* InfoSkillManaCost;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* InfoSkillCooldown;

	// ===== 슬롯 위젯 클래스 =====
	// BP 서브클래스에서 BP_SkillSlotWidget을 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Skill Window | Class")
	TSubclassOf<UT3SkillSlotWidget> SkillSlotWidgetClass;

	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:

	UPROPERTY()
	AT3CharacterBase* OwnerCharacter = nullptr;

	UPROPERTY()
	UT3SkillComponentBase* ClassSkillComp = nullptr;

	UPROPERTY()
	UT3CommonSkillComponent* CommonSkillComp = nullptr;

	// 현재 선택된 스킬 ID (0=미선택)
	int32 SelectedSkillID = 0;

	// 그리드에 생성된 20개 슬롯
	UPROPERTY()
	TArray<UT3SkillSlotWidget*> GridSlots;

	// 상단에 생성된 4개 장착 슬롯
	UPROPERTY()
	TArray<UT3SkillSlotWidget*> EquippedDisplaySlots;

	// 그리드 전체 슬롯 수 (8개 유효 + 12개 빈 자리)
	static constexpr int32 TOTAL_GRID_SLOTS = 20;
	// 직업 스킬 ID 수 (1~4)
	static constexpr int32 CLASS_SKILL_COUNT = 4;
	// 보스 스킬 ID 수 (5~8)
	static constexpr int32 BOSS_SKILL_COUNT = 4;

	// 최초 1회 슬롯 위젯을 생성하여 컨테이너에 추가
	void BuildEquippedRow();
	void BuildSkillGrid();

	// 데이터를 읽어 기존 슬롯 위젯을 갱신
	void RefreshEquippedRow();
	void RefreshSkillGrid();

	// 우측 정보 패널 갱신
	void UpdateInfoPanel(int32 SkillID);

	// ClassSkillComp + CommonSkillComp 모두 탐색하는 스킬 데이터 조회
	FSkillData* GetSkillDataByID(int32 SkillID) const;

	// 스킬 ID → SkillGridBox 내 인덱스 위치 (없으면 -1)
	int32 FindGridSlotIndexBySkillID(int32 SkillID) const;

	// 델리게이트 바인딩 해제
	void UnbindDelegates();

	// ===== 이벤트 핸들러 =====

	UFUNCTION()
	void OnGridSlotClicked(int32 SkillID);

	UFUNCTION()
	void OnEquippedSlotClicked(int32 SkillID);

	// 스킬 컴포넌트 델리게이트 수신
	UFUNCTION()
	void HandleSkillEquipStateChanged(int32 SkillID, bool bIsEquipped);

	UFUNCTION()
	void HandleSkillUnlockStateChanged(int32 SkillID, bool bIsUnlocked);
};
