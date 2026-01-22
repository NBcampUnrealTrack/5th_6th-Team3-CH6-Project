// T3CharacterAnimInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "T3PlayerInputState.h"
#include "T3CharacterAnimInstance.generated.h"

class UCharacterMovementComponent;

UCLASS()
class DESECRATION_API UT3CharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CharacterAnimation")
	FT3PlayerInputState PlayerInputState;
	
protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CharacterAnimation")
	TObjectPtr<ACharacter> OwnerCharacter;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "CharacterAnimation")
	TObjectPtr<UCharacterMovementComponent> OwnerMovement;
	
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeThreadSafeUpdateAnimation(float DeltaSeconds) override;
	
};
