// T3HealthComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DamageEvents.h"
#include "Player/T3DamageTypes.h"
#include "T3HealthComponent.generated.h"

// 데미지 처리 후 사망 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeathSignature);
// 데미지 처리 후 히트 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnDamagedSignature,
	float, DamageAmount,
	const class UDamageType*, DamageType,
	class AController*, InstigatedBy,
	AActor*, DamageCauser);

UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DESECRATION_API UT3HealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT3HealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float MaxHP = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	float CurrentHP;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetCurrentHP();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 플레이어의 데미지 처리를 위한 커스텀 함수
	void HandleTakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	// 사망 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeathSignature OnDeath;

	// 히트 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDamagedSignature OnDamaged;
};
