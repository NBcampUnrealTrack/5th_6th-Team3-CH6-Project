// T3DamageTestActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DamageEvents.h"
#include "T3DamageTestActor.generated.h"

class UBoxComponent;

UCLASS()
class DESECRATION_API AT3DamageTestActor : public AActor
{
    GENERATED_BODY()

public:
    AT3DamageTestActor();

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

    void SetLockOnWidgetVisible(bool bVisible);

    // 공격 함수
    UFUNCTION(BlueprintCallable, Category = "Test Combat")
    void ExecuteTestAttack();

protected:
    virtual void BeginPlay() override;

    // 무기 메시
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    TObjectPtr<USkeletalMeshComponent> WeaponMesh;

    // 공격 사거리 및 판정 범위
    UPROPERTY(EditAnywhere, Category = "Damage Setup")
    float AttackRadius = 100.f;

    // 테스트하고 싶은 데미지 수치
    UPROPERTY(EditAnywhere, Category = "Damage Setup")
    float TestDamageAmount = 5.f;

    // 3번 테스트 목적: 데미지 타입 (TSubclassOf를 사용해 에디터에서 선택)
    // 예: UDamageType_Light, UDamageType_Heavy 등을 선택 가능
    UPROPERTY(EditAnywhere, Category = "Damage Setup")
    TSubclassOf<UDamageType> DamageTypeClass;


    UPROPERTY(EditAnywhere, Category = "Stats")
    float MaxHP = 500.f;

    UPROPERTY(VisibleAnywhere, Category = "Stats")
    float CurrentHP;


protected:
    UPROPERTY(VisibleAnywhere, Category = "UI")
    TObjectPtr<class UWidgetComponent> LockOnWidgetComponent;

};
