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
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|State")
	float CurrentSpeed;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float SpeedThreshold = 100.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float FutureSampleTime = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float MinStartsTime = 0.5;
	
	UPROPERTY(EditDefaultsOnly, Category = "Locomotion|Tuning")
	float MinPlantsTime = 0.3f;
	
	float StateElapsedTime = 0.0f;
	
	void SetLocomotionState(ELocomotionState NewState);
	

	
	UPROPERTY(Transient)
	TObjectPtr<UT3CharacterTrajectoryComponent> TrajectoryComponent;
	

	float FutureSpeed;
	
	void DetermineLocomotionState();
	
};
