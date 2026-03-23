// T3MonsterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interface/T3Monster.h"
#include "Monster/T3HealthComponent.h"
#include "Player/T3LockOnTarget.h"
#include "T3MonsterBase.generated.h"

UCLASS()
class DESECRATION_API AT3MonsterBase : public ACharacter, public IT3LockOnTarget, public IT3Monster
{
	GENERATED_BODY()

public:
	AT3MonsterBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	bool bIsDead;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UT3HealthComponent* HealthComponent;

protected:
	UFUNCTION()
	void OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ReportTouchStimulus(AActor* OtherActor, const FVector& TouchLocation);

	// 록온 위젯을 담을 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	class UWidgetComponent* LockOnWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float BaseDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float LevelMultiplier = 1.0f;

public:
	virtual void SetLockOnWidgetVisible(bool bVisible) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Monster")
	void HalfHpSuperArmor();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual float GetCurrentAttackDamage() const;

	// 몬스터 스포너에게 인자를 전달받아 몬스터의 패트롤 루트를 설정하는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner")
	void ReceivePatrolRoute(AActor* InPatrolRoute);

	// 몬스터 스포너에게 인자를 전달받아 석상 몬스터의 트리거 박스를 설정하는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner")
	void ReceiveStatueTrigger(AActor* InStatueTrigger);
	
#pragma region ExecuteRune
public:
	virtual float GetHPPercent() const override;
	
	virtual ET3MonsterType GetMonsterType() const override;
	
	virtual void ApplyBonusDamage(float BonusDamage) override;
	
private:
	ET3MonsterType MonsterType = ET3MonsterType::Normal;
	
#pragma endregion
};
