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

void UT3CharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (APawn* OwnerChar = TryGetPawnOwner())
	{
		CurrentSpeed = OwnerChar->GetVelocity().Length();
	}
}

void UT3CharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	if (!TrajectoryComponent) return;
	
	StateElapsedTime += DeltaSeconds;
	
	const FTransformTrajectory& CurrentTrajectory = TrajectoryComponent->GetTrajectoryData();
	float DeltaTime = 0.05f;
	FTransformTrajectorySample FutureSample1 = CurrentTrajectory.GetSampleAtTime(FutureSampleTime);
	FTransformTrajectorySample FutureSample2 = CurrentTrajectory.GetSampleAtTime(FutureSampleTime+DeltaTime);
	
	FutureSpeed = (FutureSample2.Position - FutureSample1.Position).Length()/DeltaTime;
	
	DetermineLocomotionState();
}

void UT3CharacterAnimInstance::SetLocomotionState(ELocomotionState NewState)
{
	if (CurrentLocomotionState != NewState)
	{
		CurrentLocomotionState = NewState;
		StateElapsedTime = 0.0f;
	}
}

void UT3CharacterAnimInstance::DetermineLocomotionState()
{
	if (CurrentLocomotionState == ELocomotionState::Plants)
	{
		if (FutureSpeed >= SpeedThreshold)
		{
			SetLocomotionState(ELocomotionState::Starts);
			return;
		}
		
		if (StateElapsedTime < MinPlantsTime) return;
		
		if (CurrentSpeed < SpeedThreshold)
		{
			SetLocomotionState(ELocomotionState::Idle);
			return;
		}
		return;
	}
	
	if (CurrentLocomotionState == ELocomotionState::Starts)
	{
		if (FutureSpeed < SpeedThreshold)
		{
			SetLocomotionState(ELocomotionState::Plants);
			return;
		}
		
		if (StateElapsedTime < MinStartsTime) return;
		
		if (CurrentSpeed >= SpeedThreshold)
		{
			SetLocomotionState(ELocomotionState::Loop);
			return;
		}

		return;
	}
	
	if (CurrentSpeed > SpeedThreshold && FutureSpeed < SpeedThreshold)
	{
		SetLocomotionState(ELocomotionState::Plants);
	}
	else if (CurrentSpeed < SpeedThreshold && FutureSpeed > SpeedThreshold)
	{
		SetLocomotionState(ELocomotionState::Starts);
	}
	else if (CurrentSpeed > SpeedThreshold)
	{
		SetLocomotionState(ELocomotionState::Loop);
	}
	else
	{
		SetLocomotionState(ELocomotionState::Idle);
	}
}


