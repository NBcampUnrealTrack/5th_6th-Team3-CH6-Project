// T3CharcterAnimInstance.cpp


#include "Player/T3CharacterAnimInstance.h"
#include "MotionTrajectoryLibrary.h"
#include "GameFrameWork/Character.h"

const FGameplayTag Tag_Idle = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Idle"));
const FGameplayTag Tag_Starts = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Starts"));
const FGameplayTag Tag_Loop = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Loop"));
const FGameplayTag Tag_Plants = FGameplayTag::RequestGameplayTag(FName("State.Locomotion.Plants"));


void UT3CharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	if (ACharacter* OwnerChar = Cast<ACharacter>(TryGetPawnOwner()))
	{
		TrajectoryComponent = OwnerChar->FindComponentByClass<UCharacterTrajectoryComponent>();
	}
	
	CurrentLocomotionState = Tag_Idle;
}

void UT3CharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	if (!TrajectoryComponent) return;
	
	FVector CurrentVelocity = GetOwningComponent()->GetComponentVelocity();
	CurrentSpeed = CurrentVelocity.Length();
	
	//FTransformTrajectorySample FutureSample;
	//const FTransformTrajectory& CurrentTrajectory = TrajectoryComponent
	
}


