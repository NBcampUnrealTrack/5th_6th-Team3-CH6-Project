// T3CharacterAnimInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "T3CharacterTrajectoryComponent.h"
#include "T3CharacterAnimInstance.generated.h"

UENUM(BlueprintType)
enum class ELocomotionState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Starts UMETA(DisplayName = "Starts"),
	Loop UMETA(DisplayName = "Loop"),
	Plants UMETA(DisplayName = "Plants")
};

UCLASS()
class DESECRATION_API UT3CharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|State")
	ELocomotionState CurrentLocomotionState = ELocomotionState::Idle;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float SpeedThreshold = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float FutureSampleTime = 0.3f;
	

	
	UPROPERTY(Transient)
	TObjectPtr<UT3CharacterTrajectoryComponent> TrajectoryComponent;
	
	float CurrentSpeed;
	float FutureSpeed;
	
	void DetermineLocomotionState();
	
};
