// T3CharcterAnimInstance.cpp


#include "Player/T3CharacterAnimInstance.h"
#include "MotionTrajectoryLibrary.h"
#include "GameFrameWork/Character.h"
#include "VerseVM/VVMRuntimeError.h"

const FGameplayTag Tag_Idle = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Idle"));
const FGameplayTag Tag_Starts = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Starts"));
const FGameplayTag Tag_Loop = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Loop"));
const FGameplayTag Tag_Plants = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Plants"));


void UT3CharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	if (ACharacter* OwnerChar = Cast<ACharacter>(TryGetPawnOwner()))
	{
		TrajectoryComponent = OwnerChar->FindComponentByClass<UT3CharacterTrajectoryComponent>();
	}
	
	CurrentLocomotionState = Tag_Idle;
}

void UT3CharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	if (!TrajectoryComponent) return;
	
	FVector CurrentVelocity = GetOwningComponent()->GetComponentVelocity();
	CurrentSpeed = CurrentVelocity.Length();
	
	const FTransformTrajectory& CurrentTrajectory = TrajectoryComponent->GetTrajectoryData();
	float DeltaTime = 0.05f;
	FTransformTrajectorySample FutureSample1 = CurrentTrajectory.GetSampleAtTime(FutureSampleTime);
	FTransformTrajectorySample FutureSample2 = CurrentTrajectory.GetSampleAtTime(FutureSampleTime+DeltaTime);
	
	FutureSpeed = (FutureSample2.Position - FutureSample1.Position).Length()/DeltaTime;
	
	DetermineLocomotionState();

	
}

void UT3CharacterAnimInstance::DetermineLocomotionState()
{
	if (CurrentLocomotionState == Tag_Plants)
	{
		if (CurrentSpeed < 5.0f)
		{
			CurrentLocomotionState = Tag_Idle;
			return;
		}
		if (FutureSpeed > SpeedThreshold)
		{
			CurrentLocomotionState = Tag_Starts;
			return;
		}
		return;
	}
	
	if (CurrentSpeed > SpeedThreshold && FutureSpeed < SpeedThreshold)
	{
		CurrentLocomotionState = Tag_Plants;
	}
	else if (CurrentSpeed < SpeedThreshold && FutureSpeed > SpeedThreshold)
	{
		CurrentLocomotionState = Tag_Starts;
	}
	else if (CurrentSpeed > SpeedThreshold)
	{
		CurrentLocomotionState = Tag_Loop;
	}
	else
	{
		CurrentLocomotionState = Tag_Idle;
	}
}


