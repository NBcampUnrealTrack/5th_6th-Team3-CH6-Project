// T3MonsterBase.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Engine/DataTable.h"
#include "T3MonsterBase.generated.h"

USTRUCT(BlueprintType)
struct FMonsterStats : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackDamage;
};

UCLASS()
class DESECRATION_API AT3MonsterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AT3MonsterBase();

protected:
	virtual void BeginPlay() override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void OnDeath();

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "AI")
	void PerformAttack();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "AI")
	void PerformAttackCheck();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<UAnimMontage*> AttackMontages;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	FName MonsterRowName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Stats")
	UDataTable* MonsterDataTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
	float CurrentHP;

	bool bIsDead;
};