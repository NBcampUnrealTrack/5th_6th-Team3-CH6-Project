// T3PoisonStackWidget.h
// 독 축적치/독 상태를 표시하는 HUD 위젯.
// 이 클래스를 상속받아 WBP_PoisonStackBar를 만들고 UMG에서 레이아웃을 구성한다.
//
// 바인딩 흐름:
//   NativeConstruct → 소유 캐릭터 자동 탐색 → OnStatChanged / OnPoisonActivated 구독
//   스택 변화 시 BP_OnStackChanged 호출 → BP에서 바 수치 갱신 + 표시/숨김 처리
//   독 활성화 시 BP_OnPoisonStateChanged(true) → BP에서 색상/애니메이션 전환

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/T3CharacterBase.h"
#include "T3PoisonStackWidget.generated.h"

UCLASS()
class DESECRATION_API UT3PoisonStackWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:
	/**
	 * 위젯이 추적할 캐릭터를 수동으로 지정한다.
	 * NativeConstruct에서 자동 바인딩에 실패했을 때 외부에서 호출한다.
	 * (예: HUD BP의 Event Begin Play에서 GetOwningPlayerPawn으로 설정)
	 */
	UFUNCTION(BlueprintCallable, Category = "Poison")
	void InitializeWidget(AT3CharacterBase* InCharacter);

	// ──────────────────────────────────────────────
	// BP 구현 이벤트 — WBP_PoisonStackBar에서 오버라이드
	// ──────────────────────────────────────────────

	/**
	 * 독 축적치가 변경될 때마다 호출된다.
	 * BP 구현 가이드:
	 *   CurrentStack > 0  → 위젯 Visible, ProgressBar 비율 = CurrentStack / MaxStack
	 *   CurrentStack == 0 → 위젯 Collapsed (또는 FadeOut 애니메이션)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Poison")
	void BP_OnStackChanged(int32 CurrentStack, int32 MaxStack);

	/**
	 * 독 DoT가 활성화/해제될 때 호출된다.
	 * BP 구현 가이드:
	 *   bIsActive == true  → 바 색상을 "독 활성화" 색으로 변경 (예: 보라/진녹)
	 *   bIsActive == false → 원래 색으로 복귀
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Poison")
	void BP_OnPoisonStateChanged(bool bIsActive);

private:
	UPROPERTY()
	TObjectPtr<AT3CharacterBase> OwnerCharacter;

	UFUNCTION()
	void HandleStatChanged(ET3StatType StatType, float CurrentValue, float MaxValue);

	UFUNCTION()
	void HandlePoisonActivated(bool bIsActive);

	void BindToCharacter(AT3CharacterBase* Character);
	void UnbindFromCharacter();
};
