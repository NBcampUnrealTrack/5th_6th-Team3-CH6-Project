// T3MonsterBase.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "T3MonsterBase.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPerformAttackEndDelegate);

USTRUCT(BlueprintType)
struct FMonsterStats : public FTableRowBase {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float MaxHP;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float AttackDamage;
};

UCLASS()
class DESECRATION_API AT3MonsterBase : public ACharacter {
  GENERATED_BODY()

public:
  AT3MonsterBase();

protected:
  virtual void BeginPlay() override;

  virtual float TakeDamage(float DamageAmount,
                           struct FDamageEvent const &DamageEvent,
                           class AController *EventInstigator,
                           AActor *DamageCauser) override;

  void OnDeath();

public:
  virtual void Tick(float DeltaTime) override;

  virtual void SetupPlayerInputComponent(
      class UInputComponent *PlayerInputComponent) override;

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent,
            Category = "AI|Combat")
  void PerformAttack();

  UFUNCTION(BlueprintCallable, BlueprintImplementableEvent,
            Category = "AI|Combat")
  void PerformAttackCheck();

  UFUNCTION(BlueprintCallable, Category = "AI|Combat")
  void SetIsAttacking(bool bValue);

  UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "AI")
  FOnPerformAttackEndDelegate OnPerformAttackEnd;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
  TArray<UAnimMontage *> AttackMontages;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|HitReact")
  UAnimMontage *HitReactMontage_Light;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|HitReact")
  UAnimMontage *HitReactMontage_Heavy;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Death")
  UAnimMontage *DeathMontage_Light;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Death")
  UAnimMontage *DeathMontage_Heavy;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Settings")
  float HeavyDamageThreshold;

protected:
  UFUNCTION()
  void OnMontageEnded(UAnimMontage *Montage, bool bInterrupted);

  void EnableRagdoll();

  // Weapon Handling
  void DropWeapon();

protected:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Weapon")
  TObjectPtr<UPrimitiveComponent> WeaponComponent;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Weapon")
  bool bDropWeaponOnDeath = true;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat|Weapon")
  bool bHideWeaponOnDeath = false;

  bool bIsWeaponDropped = false;
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
  FName MonsterRowName;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Monster|Stats")
  UDataTable *MonsterDataTable;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
  float CurrentHP;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Monster|Stats")
  float CurrentATK;

  bool bIsDead;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
  bool bIsAttacking;
};