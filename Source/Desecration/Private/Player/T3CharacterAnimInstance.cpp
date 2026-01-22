// T3CharacterAnimInstance.cpp


#include "Player/T3CharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UT3CharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	
	if (OwnerCharacter)
	{
		OwnerMovement = OwnerCharacter->GetCharacterMovement();
	}
}

void UT3CharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeThreadSafeUpdateAnimation(DeltaSeconds);
	
	if (!OwnerCharacter || !OwnerMovement)
	{
		return;
	}
	
	float CurrentAccelerationSq = OwnerMovement->GetCurrentAcceleration().SizeSquared();
	PlayerInputState.bWantsToMove = CurrentAccelerationSq > KINDA_SMALL_NUMBER;
	
	float CurrentGroundSpeedSq = OwnerCharacter->GetVelocity().SizeSquared2D();
	
	const float MoveThreshold = 3.0f;
	PlayerInputState.bIsMoving = CurrentGroundSpeedSq > (MoveThreshold * MoveThreshold);
}
