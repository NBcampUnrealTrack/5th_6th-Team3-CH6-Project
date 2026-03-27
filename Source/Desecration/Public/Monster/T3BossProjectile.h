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
class UAudioComponent;

UCLASS()
class DESECRATION_API AT3BossProjectile : public AActor
{
	GENERATED_BODY()

public:
	AT3BossProjectile();

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

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

	// Niagara 이펙트 (에디터에서 설정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Effect")
	TObjectPtr<UNiagaraComponent> ProjectileEffect;

	// --- 콜리전 크기 (에디터 조절) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	FVector CollisionExtent = FVector(30.f, 80.f, 40.f);

	// --- 사운드 ---
	// 비행 루프 사운드 (투사체에 붙어서 이동)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Sound")
	TObjectPtr<USoundBase> LoopSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Sound")
	float LoopVolumeMultiplier = 1.0f;

	// 충돌 시 재생되는 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Sound")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Sound")
	float ImpactVolumeMultiplier = 1.5f;

	// 3D 거리 감쇠 설정 (미설정 시 기본 감쇠 자동 생성)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Sound")
	TObjectPtr<USoundAttenuation> SoundAttenuation;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopAudioComponent;
	float Damage = 0.f;
	EHitIntensity HitIntensity = EHitIntensity::Light;
	TSubclassOf<UT3DamageType_Base> DamageTypeClass;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	UFUNCTION()
	void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
