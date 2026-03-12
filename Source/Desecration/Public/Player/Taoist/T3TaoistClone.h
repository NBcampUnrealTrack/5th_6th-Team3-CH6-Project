// T3TaoistClone.h

#pragma once

#include "CoreMinimal.h"
#include "Player/T3CharacterBase.h"
#include "Player/Taoist/T3Taoist_SkillComponent.h"
#include "T3TaoistClone.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnCloneDestroyed, class AT3TaoistClone*);

UCLASS()
class DESECRATION_API AT3TaoistClone : public AT3CharacterBase
{
	GENERATED_BODY()

public:
    AT3TaoistClone();

    FOnCloneDestroyed OnCloneDestroyed;

    // 본체로부터 호출될 초기화 함수
    void InitializeClone(AT3CharacterBase* InOwner);

    // 본체의 공격/스킬 명령을 실행
    void ExecuteMirrorAction(EActionType ActionType);
    void SetCloneIndex(int32 InIndex) { CloneIndex = InIndex; }
    int32 GetCloneIndex() const { return CloneIndex; }

    // 본체(Owner) 접근용 Getter (컨트롤러에서 사용)
    FORCEINLINE class AT3CharacterBase* GetOwnerCharacter() const { return OwnerCharacter; }

    // 공격력 보정 (본체의 30%)
    virtual float GetAttackPower() const override;
protected:
    virtual void BeginPlay() override;
    virtual void Destroyed() override;
    void ApplyGlowToEverything();

    UPROPERTY()
    TObjectPtr<class AT3WeaponBase> MyCloneWeapon;

    // 무작위 이동을 위한 타이머 핸들
    FTimerHandle IdleWanderTimer;

    // 무작위 이동 실행 함수
    void StartIdleWander();
    void ResetIdleWanderTimer();





    virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* InstigatedBy, AActor* DamageCauser) override;

    UPROPERTY(EditAnywhere, Category = "Montage")
    UAnimMontage* AttackMontage = nullptr;

    UPROPERTY(EditAnywhere, Category = "Montage")
    UAnimMontage* StrongWindMontage = nullptr;

    UPROPERTY(EditAnywhere, Category = "Montage")
    UAnimMontage* SummonTigerMontage = nullptr;

    // 부적 투사체 클래스
    UPROPERTY(EditAnywhere, Category = "Combat")
    TSubclassOf<class AT3TalismanProjectile> TalismanClass;

    // 장풍 클래스
    UPROPERTY(EditAnywhere, Category = "Combat")
    TSubclassOf<class AT3StrongWind> StrongWindClass;
    float StrongWindSpawnDistance;
    float StrongWindDamageMultiflier;

    // 호랑이 클래스
    UPROPERTY(EditAnywhere, Category = "Combat")
    TSubclassOf<class AT3TigerAttack> TigerClass;

    void SpawnTalisman();

    void SpawnStrongWind();


    // 머터리얼

    // 모든 슬롯의 동적 머터리얼을 담을 배열
    UPROPERTY()
    TArray<class UMaterialInstanceDynamic*> DynamicMaterials;

    UPROPERTY(EditAnywhere, Category = "Appearance")
    FLinearColor CloneGlowColor = FLinearColor(0.0f, 0.1f, 0.2f, 1.0f);


    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* DestroyEffect;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* DestroySound;

private:
    UPROPERTY()
    TObjectPtr<AT3CharacterBase> OwnerCharacter;

    // AI 이동 로직을 위한 타겟 탐색 및 거리 유지
    void UpdateAIBehavior();

    FTimerHandle AIUpdateTimer;

    UPROPERTY()
    int32 CloneIndex = 0; // 소환 시 0 또는 1 부여
};
