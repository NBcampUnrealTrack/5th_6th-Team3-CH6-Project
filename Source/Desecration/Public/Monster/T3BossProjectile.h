// T3BossProjectile.h
// 보스 전용 투사체 (검기 등) — TakeDamage 직접 호출

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/T3DamageTypes.h"
#include "T3BossProjectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UStaticMeshComponent;

UCLASS()
class DESECRATION_API AT3BossProjectile : public AActor
{
	GENERATED_BODY()

public:
	AT3BossProjectile();

protected:
	virtual void BeginPlay() override;

public:
	// 데미지/속도/강도 초기화
	void InitializeProjectile(float InDamage, float InSpeed,
		EHitIntensity InIntensity = EHitIntensity::Light,
		TSubclassOf<UT3DamageType_Base> InDamageType = nullptr);

	// --- 컴포넌트 ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> MovementComp;

	// Niagara 이펙트 (에디터에서 설정, 없으면 임시 메시 표시)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Effect")
	TObjectPtr<UNiagaraComponent> ProjectileEffect;

	// 임시 시각 메시 (Niagara 없을 때 디버그용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Debug")
	TObjectPtr<UStaticMeshComponent> DebugMesh;

	// --- 콜리전 크기 (에디터 조절) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FVector CollisionExtent = FVector(30.f, 80.f, 40.f);

private:
	float Damage = 0.f;
	EHitIntensity HitIntensity = EHitIntensity::Light;
	TSubclassOf<UT3DamageType_Base> DamageTypeClass;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
