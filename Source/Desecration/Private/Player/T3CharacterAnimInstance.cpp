// T3CharcterAnimInstance.cpp


#include "Player/T3CharacterAnimInstance.h"
#include "MotionTrajectoryLibrary.h"
#include "GameFrameWork/Character.h"
#include "VerseVM/VVMRuntimeError.h"


void UT3CharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	if (ACharacter* OwnerChar = Cast<ACharacter>(TryGetPawnOwner()))
	{
		TrajectoryComponent = OwnerChar->FindComponentByClass<UT3CharacterTrajectoryComponent>();
	}
	
	CurrentLocomotionState = ELocomotionState::Idle;
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
	if (CurrentLocomotionState == ELocomotionState::Plants)
	{
		if (CurrentSpeed < 5.0f)
		{
			CurrentLocomotionState = ELocomotionState::Idle;
			return;
		}
		if (FutureSpeed > SpeedThreshold)
		{
			CurrentLocomotionState = ELocomotionState::Starts;
			return;
		}
		return;
	}
	
	if (CurrentSpeed > SpeedThreshold && FutureSpeed < SpeedThreshold)
	{
		CurrentLocomotionState = ELocomotionState::Plants;
	}
	else if (CurrentSpeed < SpeedThreshold && FutureSpeed > SpeedThreshold)
	{
		CurrentLocomotionState = ELocomotionState::Starts;
	}
	else if (CurrentSpeed > SpeedThreshold)
	{
		CurrentLocomotionState = ELocomotionState::Loop;
	}
	else
	{
		CurrentLocomotionState = ELocomotionState::Idle;
	}
}


