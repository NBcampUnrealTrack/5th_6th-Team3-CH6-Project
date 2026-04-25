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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	int CurrentStage;

protected:
	// 이동 상태(Walking, Falling 등)가 변경될 때
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	// 지면에 착지했을 때
	virtual void Landed(const FHitResult& Hit) override;

	// 낙하 시작 시점의 Z 높이
	float StartFallHeight = 0.f;

	// 즉사 낙하 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Movement")
	float DeathFallDistance = 1500.f;


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
	
	// 스포너에게 삭제 요청을 받아 몬스터를 강제로 제거하는 함수
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawner")
	void CleanupBeforeDestroy();

	// 스포너에서 현재 스테이지 정보를 받아 몬스터의 배율을 조정하는 함수
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void UpdateByStage();

#pragma region ExecuteRune
public:
	virtual float GetHPPercent() const override;
	
	virtual ET3MonsterType GetMonsterType() const override;
	
	virtual void ApplyBonusDamage(float BonusDamage) override;
	
private:
	ET3MonsterType MonsterType = ET3MonsterType::Normal;
	
#pragma endregion
	
#pragma region 장신구
public:
	// IT3Monster 인터페이스 구현
	virtual void SetAnimationSpeedMultiplier(float MoveAnimMultiplier, float AttackAnimMultiplier) override;

	// 엔진 기본 몽타주 재생 함수를 오버라이드하여 새 몽타주 재생 시 배율 강제 적용
	virtual float PlayAnimMontage(class UAnimMontage* AnimMontage, float InPlayRate = 1.f, FName StartSectionName = NAME_None) override;

	// 현재 저장된 공격 애니메이션 배율 반환
	UFUNCTION(BlueprintCallable, Category = "Combat|Accessory")
	float GetCurrentAttackRate() const { return CurrentAttackRate; }

private:
	// 현재 적용된 배율 (기본값 1.0)
	float CurrentMoveRate = 1.f;
	float CurrentAttackRate = 1.f;

	// 현재 재생 중인 몽타주의 속도를 즉시 업데이트하는 내부 헬퍼
	void UpdateActiveMontagePlayRate();
#pragma endregion
};
