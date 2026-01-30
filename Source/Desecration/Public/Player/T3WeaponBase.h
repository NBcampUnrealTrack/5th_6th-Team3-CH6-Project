// T3WeaponBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/T3DamageTypes.h"
#include "T3WeaponBase.generated.h"

UCLASS()
class DESECRATION_API AT3WeaponBase : public AActor
{
	GENERATED_BODY()
	
public:
    AT3WeaponBase();

    // 무기 콜리젼 활성화/비활성화 함수
    void SetWeaponCollisionEnabled(bool bEnabled, float InDamageMultiplier = 1.f, TSubclassOf<class UT3DamageType_Base> InType = nullptr, EHitIntensity InIntensity = EHitIntensity::Light, float InStunAmount = 0.f);

protected:
    virtual void BeginPlay() override;

    // 오버랩 이벤트 함수
    UFUNCTION()
    void OnWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
   
    // 충돌체 (칼날 부분에 배치)
    UPROPERTY(VisibleAnywhere, Category = "Combat")
   TObjectPtr<class UBoxComponent> WeaponCollision;

    // 무기 외형
    UPROPERTY(VisibleAnywhere, Category = "Visual")
    TObjectPtr<class UStaticMeshComponent> WeaponMesh;

    float CurrentAttackDamage;
    float StunAmount;
    TSubclassOf<class UT3DamageType_Base> CurrentDamageType;
    EHitIntensity CurrentIntensity;
    TObjectPtr<class AT3CharacterBase> OwnerChar;
    TObjectPtr<class UT3CombatComponent> Combat;



private:
    // 중복 히트 방지 리스트
    TArray<AActor*> AlreadyHitActors;

};

