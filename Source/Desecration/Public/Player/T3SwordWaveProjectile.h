// T3SwordWaveProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "T3SwordWaveProjectile.generated.h"

class UNiagaraComponent;

UCLASS()
class DESECRATION_API AT3SwordWaveProjectile : public AActor
{
	GENERATED_BODY()

public:

    AT3SwordWaveProjectile();

    // 데미지와 속도를 설정하는 함수
    void InitializeProjectile(float InDamage, float InSpeed);

    void PostInitializeComponents() override;

private:
    UPROPERTY(VisibleAnywhere)
    class UProjectileMovementComponent* MovementComp;

    UPROPERTY(VisibleAnywhere)
    class UBoxComponent* BoxCollision;

    UPROPERTY(VisibleAnywhere, Category = "Effects")
    UNiagaraComponent* SkillEffect;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    // 전달받은 데미지를 담을 변수
    float Damage;

};
