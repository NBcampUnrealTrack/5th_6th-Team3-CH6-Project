// T3StrongWind.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3StrongWind.generated.h"

UCLASS()
class DESECRATION_API AT3StrongWind : public AActor
{
	GENERATED_BODY()
	
public:
    AT3StrongWind();
    void SetDamage(float InDamage);
protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<class UBoxComponent> AttackArea; // 길다란 판정 범위

    UPROPERTY(VisibleAnywhere, Category = "Effects")
    TObjectPtr<class UNiagaraComponent> NiagaraComp;

    float Damage;

    UPROPERTY(EditAnywhere, Category = "Components")
    float SpawnTime = 1.5f;

    void ProcessHit(AActor* TargetActor, const FString& HitType);

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
    // 스폰 즉시 데미지를 주는 함수
    void ImmediateDamageCheck();

    UPROPERTY()
    TArray<TObjectPtr<AActor>> AlreadyHitActors;
};
