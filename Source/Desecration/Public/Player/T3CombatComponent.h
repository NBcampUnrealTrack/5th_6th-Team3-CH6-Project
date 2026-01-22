// T3CombatComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3CombatComponent.generated.h"

class AAICharacter;

// 캐릭터 상태 ENUM
UENUM(BlueprintType)
enum class ECharacterCombatState : uint8
{
	Idle,
	Blocking,
	Parrying,
	Dodge,
	Dead
};

// 노티파이용 ENUM
UENUM(BlueprintType)
enum class ECombatWindowType : uint8
{
	None,
	Parry      UMETA(DisplayName = "Parry Window"),
	Invincible UMETA(DisplayName = "Invincible Window"),
	Attack     UMETA(DisplayName = "Attack Collision")
};

// 피격 방향 ENUM
UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front   UMETA(DisplayName = "Front"),
	Back    UMETA(DisplayName = "Back"),
	Left    UMETA(DisplayName = "Left"),
	Right   UMETA(DisplayName = "Right")
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3CombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT3CombatComponent();

protected:
	virtual void BeginPlay() override;

	// 데미지 처리 함수 (OnTakeAnyDamage에 바인딩용)
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 막기/패링
	void StartBlock();
	void EndBlock();

	UFUNCTION(BlueprintCallable)
	void SetParryingEnabled(bool bEnabled);

	// 록온
	void ToggleLockOn();

	// 캐릭터 상태 Getter
	FORCEINLINE ECharacterCombatState GetCurrentState() const { return CurrentState; }

	// 공격 함수
	void RequestAttackDamage(AActor* TargetActor, float DamageAmount, TSubclassOf<class UDamageType> DamageTypeClass);

private:
	// 상태별 데미지 경감 로직
	float CalculateFinalDamage(float IncomingDamage, const class UDamageType* DamageType);
	
	// 내부 로직용
	AActor* FindBestTarget();
	void ResetLockOn();
	void UpdateTargetUI(AActor* Target, bool bIsVisible);
	bool IsTargetVisible(AActor* Target) const;

	UPROPERTY()
	TObjectPtr<class AT3CharacterBase> OwnerChar;

	UPROPERTY()
	TObjectPtr<class APlayerController> OwnerPC;

	// 상태 변수
	ECharacterCombatState CurrentState = ECharacterCombatState::Idle;

	// 록온 변수
	bool bIsLockOn = false;
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	// 록온 타깃 식별 태그
	UPROPERTY(EditAnywhere, Category = "Combat|LockOn")
	FName TargetTag = FName("Enemy");

	float SearchRadius = 1000.f;
	float InterpSpeed = 10.f;


protected:
	// 방향 계산 함수
	EHitDirection CalculateHitDirection(const FVector& HitLocation);

};
