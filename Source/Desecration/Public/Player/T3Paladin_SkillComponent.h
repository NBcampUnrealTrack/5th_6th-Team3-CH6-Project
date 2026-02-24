// T3Paladin_SkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "Player/T3SkillComponentBase.h"
#include "T3Paladin_SkillComponent.generated.h"

class AT3SwordWaveProjectile;
class UAnimMontage;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3Paladin_SkillComponent : public UT3SkillComponentBase
{
	GENERATED_BODY()

public:
    
    UT3Paladin_SkillComponent();

    // 스킬 실행 함수
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void ExecuteSwordWave();
    
    UFUNCTION(BlueprintImplementableEvent, Category = "Skill")
    void ShieldStrike();

    // 실제 투사체 스폰 (AnimNotify에서 호출될 용도)
    void SpawnSwordWaveProjectile();

    virtual void ExecuteSkill(int32 SkilSolt) override;
    virtual void ExecuteSkillNotify(int32 Index) override;
    virtual void CancelCurrentSkill() override;

    virtual FSkillData* GetSkillDataByID(int32 SkillID) override;

private:
    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData SwordWaveData;

    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData ShieldStrikeData;

    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData EmptySkillData;

    void OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted);
		




    // ==== 신의 심판 데이터 ====

protected:
    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData JudgmentData;

    UPROPERTY(EditAnywhere, Category = "Skill|Judgement")
    float JudgementExexcuteDistance = 500.f;

    UPROPERTY(EditAnywhere, Category = "Skill|Judgement")
    float JudgementExexcuteRange = 500.f;

    // 장판/레이저 이펙트 클래스
    UPROPERTY(EditAnywhere, Category = "Skill|Judgement")
    TSubclassOf<AActor> JudgmentAreaIndicatorClass;

    UPROPERTY(EditAnywhere, Category = "Skill|Judgement")
    TSubclassOf<AActor> JudgmentLaserClass;

private:

    FTimerHandle JudgmentTimerHandle;
    FVector JudgmentTargetLocation;

    UPROPERTY()
    AActor* CurrentJudgmentLaserActor; // 현재 소환된 레이저 액터 포인터

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void ExecuteJudgment();      // 스킬 시작 (애니메이션 재생)
    void SpawnJudgmentArea();   // 스킬 시작 후 장판 생성 및 위치 확정
    void SpawnJudgmentLaser();  // 2초 후 레이저 발사 및 데미지 로직 시작
    void ApplyJudgmentDamage(int32 RemainingHits); // 다단 히트 처리
    void FinishJudgmentSkill();
    void CancleJudgmentLaser(); // 피격 시 신의 심판 캔슬
};
