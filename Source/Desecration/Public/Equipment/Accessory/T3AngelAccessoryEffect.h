#pragma once

#include "CoreMinimal.h"
#include "T3AccessoryEffectBase.h"
#include "Components/SphereComponent.h"
#include "T3AngelAccessoryEffect.generated.h"

UCLASS()
class DESECRATION_API UT3AngelAccessoryEffect : public UT3AccessoryEffectBase
{
	GENERATED_BODY()

public:
	virtual void OnEquipped_Implementation(AT3CharacterBase* OwnerChar) override;

	virtual void OnUnequipped_Implementation(AT3CharacterBase* OwnerChar) override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void RestoreMonster(int32 Index);

	UPROPERTY(EditDefaultsOnly)
	float SlowRadius = 400.f;

	UPROPERTY(EditDefaultsOnly)
	float MoveSpeedSlowAmount = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	float MoveAnimSlowAmount = 0.5f;

	UPROPERTY(EditDefaultsOnly)
	float AttackAnimSlowAmount = 0.7f;

	UPROPERTY(EditDefaultsOnly)
	bool bShowDebugRadius = false;

	UPROPERTY()
	USphereComponent* AuraSphere = nullptr;

	TArray<TWeakObjectPtr<AActor>> SlowedMonsters;

	TArray<float> OriginalMaxWalkSpeeds;
};
