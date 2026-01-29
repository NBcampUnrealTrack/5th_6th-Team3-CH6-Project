// T3CombatComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3CharacterDataAsset.h"
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
	Attacking,
	Dead
};

// 노티파이용 ENUM
UENUM(BlueprintType)
enum class ECombatWindowType : uint8
{
	None,
	Parry      UMETA(DisplayName = "Parry Window"),
	Dodge UMETA(DisplayName = "Dodge Window"),
	Attack     UMETA(DisplayName = "Attack Collision"),
	PrevenRegen UMETA(DisplayName = "PrevenRegen")
	
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

	void InitializeWeapons(const TMap<EEquipSlot, FWeaponEquipInfo>& WeaponMap);
	
	// 델리게이트용 데미지 처리 함수 (OnTakeAnyDamage에 바인딩용)
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	// 2. 실제 로직용 (우리가 원하는 Intensity 포함)
	void ExecuteHitLogic(AActor* DamageCauser, float Damage, const UDamageType* DamageType, AController* InstigatedBy, EHitIntensity Intensity);

public:
	void SetOwnerChar(ACharacter* InChar) { AIChar = InChar; };

protected:
	virtual void BeginPlay() override;



public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 막기/패링
	void StartBlock();
	void EndBlock();
	
	void Attack();

	UFUNCTION(BlueprintCallable)
	void SetParryingEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable)
	void SetDodgingEnabled(bool bEnabled);


	// 록온
	void ToggleLockOn();

	// 캐릭터 상태 Getter
	FORCEINLINE ECharacterCombatState GetCurrentState() const { return CurrentState; }

	// 공격 함수
	UFUNCTION(BlueprintCallable)
	void RequestAttackDamage(AActor* TargetActor, float DamageAmount, EHitIntensity Intensity, TSubclassOf<class UT3DamageType_Base> DamageTypeClass);

	// 스태미너 소모 함수
	void ConsumeStamina(float Amount);




	// 무기 가져오기
	UFUNCTION(BlueprintCallable, Category = "Combat")
	class AT3WeaponBase* GetWeaponBySlot(EEquipSlot Slot) const;


	// 무기 제거 로직
	void ClearWeapons();


private:
	// 상태별 데미지 경감 로직
	float CalculateFinalDamage(float IncomingDamage, const class UDamageType* DamageType);
	
	// 내부 로직용
	AActor* FindBestTarget();
	void ResetLockOn();
	void UpdateTargetUI(AActor* Target, bool bIsVisible);
	bool IsTargetVisible(AActor* Target) const;

	// 패링
	FTimerHandle ParryingToBlockingTimerHandle;
	UFUNCTION()
	void SwitchToBlockingState();

	UPROPERTY()
	TObjectPtr<class AT3CharacterBase> OwnerChar;

	UPROPERTY()
	TObjectPtr<class APlayerController> OwnerPC;

	UPROPERTY()
	TObjectPtr<class ACharacter> AIChar;

	UPROPERTY()
	TObjectPtr<class AController> AIPC;

	// 상태 변수
	ECharacterCombatState CurrentState;

	// 록온 변수
	bool bIsLockOn = false;
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	// 록온 타깃 식별 태그
	UPROPERTY(EditAnywhere, Category = "Combat|LockOn")
	FName TargetTag = FName("Enemy");

	float SearchRadius = 2000.f;
	float InterpSpeed = 20.f;


	// 무기
	UPROPERTY()
	TMap<EEquipSlot, TObjectPtr<class AT3WeaponBase>> EquippedWeapons;

	// 방향 계산 함수
	EHitDirection CalculateHitDirection(const FVector& HitLocation);


};
