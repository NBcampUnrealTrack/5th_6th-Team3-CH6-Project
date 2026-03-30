// T3LunarSlash.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "T3LunarSlash.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

UCLASS()
class DESECRATION_API AT3LunarSlash : public AActor
{
	GENERATED_BODY()
	
public:	
	AT3LunarSlash();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LunarSlash")
	float ExplosionDamage = 300.0f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	class UNiagaraComponent* ColdAuraComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadwrite, Category = "LunarSlash")
	bool bIsAlreadyExploded;
	
	UFUNCTION()
	void OnLunarOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherOverlappedComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void ExplodeLunarSlash(int32 ChargeLevel);

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> SphereComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MoonMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarSlash")
	float DamageTickRate = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarSlash")
	float DamageRate = 0.4f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LunarSlash")
	TObjectPtr<AT3CharacterBase> OwnerChar;
	
	FTimerHandle DamageTickTimer;
	
	UFUNCTION()
	void ApplyDamage();
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnDoTAttack();
	UFUNCTION(BlueprintImplementableEvent)
	void OnLunarExplosion();
};
