// T3MidBossHPBarWidget.h
// 중간보스 HP바 위젯 — BindWidget 패턴, 이벤트 기반 갱신

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3MidBossHPBarWidget.generated.h"

class AT3MidBossMonster;
class UProgressBar;
class UTextBlock;

// 위젯 종료 델리게이트 (사망 연출 완료 후 외부 통보)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBossHPBarWidgetEnd);

/**
 * 중간보스 HP바 위젯 베이스 클래스
 * WBP_T3MidBossHPBar의 Parent Class로 지정하여 사용
 * BindWidget으로 Blueprint 위젯과 C++ 로직을 연결
 *
 * 사용법:
 * 1. 이 클래스를 부모로 Widget Blueprint 생성
 * 2. Designer에서 HPBar_Red, HPBar_Yellow, Txt_BossName 배치
 * 3. BP_MidBoss의 BossHPBarWidgetClass에 할당
 */
UCLASS()
class DESECRATION_API UT3MidBossHPBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 보스 레퍼런스 (CreateWidget 직후 외부에서 설정)
	UPROPERTY(BlueprintReadWrite, Category = "BossHPBar")
	TObjectPtr<AT3MidBossMonster> TargetBoss;

	// Yellow 바 보간 속도 (높을수록 빠르게 쫓아감)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossHPBar")
	float YellowInterpSpeed = 3.0f;

	// Red 바 갱신 후 Yellow 캐치업 시작까지 지연 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossHPBar")
	float YellowDelaySeconds = 0.5f;

	// 사망 후 위젯 제거까지 대기 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BossHPBar")
	float DeathRemoveDelay = 3.0f;

	// 위젯 종료 델리게이트 (RemoveFromParent 직전 호출)
	UPROPERTY(BlueprintAssignable, Category = "BossHPBar|Events")
	FOnBossHPBarWidgetEnd OnWidgetEnd;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ==========================================================
	// BindWidget — Designer에서 동일 이름 위젯과 자동 연결
	// ==========================================================

	// 빨간 바 (즉시 갱신 — 현재 HP 비율)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HPBar_Red;

	// 노란 바 (지연 캐치업 — FInterpTo로 서서히 줄어듦)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HPBar_Yellow;

	// 보스 이름 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_BossName;

private:
	// HP 갱신 (OnMidBossDamaged 델리게이트에서 호출)
	UFUNCTION()
	void UpdateHPBar();

	// 사망 처리 (OnMidBossDeath 델리게이트에서 호출)
	UFUNCTION()
	void OnBossDeath();

	// 사망 지연 후 위젯 제거
	UFUNCTION()
	void RemoveWidget();

	// 현재 Red 바 퍼센트 (0~1)
	float RedPercent = 1.0f;

	// 현재 Yellow 바 퍼센트 (0~1)
	float YellowPercent = 1.0f;

	// Yellow 캐치업 목표 (Red와 동일)
	float TargetPercent = 1.0f;

	// Yellow 캐치업 활성 여부
	bool bShouldCatchUp = false;

	// Yellow 지연 타이머
	FTimerHandle YellowDelayTimerHandle;

	// 사망 제거 타이머
	FTimerHandle DeathRemoveTimerHandle;
};
