// T3HUDSlotWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3HUDSlotWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UImage;

UCLASS()
class DESECRATION_API UT3HUDSlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* CurrentSkillCooldownBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CurrentSkillCooldownTime;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* NextSkillCooldownBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* NextSkillCooldownTime;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* FinishAnim;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;

public:
	UFUNCTION()
	void HandleCooldownStarted(int32 SkillID, float CooldownTime);

	// 스왑이나 장착 시 호출하여 틱을 강제로 켬
	UFUNCTION()
	void HandleSkillSlotUpdated(int32 SlotIndex, int32 SkillID, const struct FSkillData& SkillData);

	virtual void InitializeWidget(class UT3SkillComponentBase* InSkillComp);
private:

	UPROPERTY()
	class UT3SkillComponentBase* SkillComp;

	bool bIsTickActive = false;

	bool bCurrentWasCoolingDown = false;
	bool bNextWasCoolingDown = false;


	// 각 슬롯별 쿨타임 업데이트 로직
	bool ProcessCooldown(int32 SlotIndex, class UProgressBar* Bar, class UTextBlock* Text, bool& bOutWasCoolingDown);

	void PlayFinishAnimation();
};
