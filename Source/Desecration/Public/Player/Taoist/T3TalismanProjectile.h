// T3TalismanProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3TalismanProjectile.generated.h"

UCLASS()
class DESECRATION_API AT3TalismanProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AT3TalismanProjectile();
    void SetDamage(float InDamage) { Damage = InDamage; }
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    // 부적의 외형
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* TalismanMesh;

    // 투사체 움직임 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    class UProjectileMovementComponent* ProjectileMovement;

    // 충돌 처리
    UPROPERTY(VisibleAnywhere, Category = "Collision")
    class UBoxComponent* CollisionBox;

    // --- 부적 흔들림 설정 ---
    UPROPERTY(EditAnywhere, Category = "Movement|SinWave")
    float Amplitude = 50.f;  // 흔들림 폭

    UPROPERTY(EditAnywhere, Category = "Movement|SinWave")
    float Frequency = 10.f;  // 흔들림 속도

    float RunningTime = 0.f;
    FVector InitialRelativeLocation;


    // 충돌 이벤트 함수
    UFUNCTION()
    void OnTalismanOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(EditAnywhere, Category = "Combat")
    float DamageMultiflier = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float Damage = 20.f;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ExplosionEffect;

    // 중복 히트 방지 리스트
    TArray<AActor*> HitActors;
};
