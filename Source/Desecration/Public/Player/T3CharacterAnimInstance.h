// T3CharacterAnimInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "CharacterTrajectoryComponent.h"
#include "T3CharacterAnimInstance.generated.h"


UCLASS()
class DESECRATION_API UT3CharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float SpeedThreshold = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float FutureSampleTime = 0.3f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|State")
	FGameplayTag CurrentLocomotionState;
	
	UPROPERTY(Transient)
	TObjectPtr<UCharacterTrajectoryComponent> TrajectoryComponent;
	
	float CurrentSpeed;
	float FutureSpeed;
	
	void DetermineLocomotionState();
	
};
