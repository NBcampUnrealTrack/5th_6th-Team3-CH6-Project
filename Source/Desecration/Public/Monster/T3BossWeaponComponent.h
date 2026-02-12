// T3BossWeaponComponent.h
// 보스 무기 컴포넌트 — 메시, 판정, 소켓 부착, 드롭

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3BossWeaponComponent.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USkeletalMeshComponent;

// 무기 히트 델리게이트 — 히트된 액터를 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponHitActor, AActor*, HitActor);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DESECRATION_API UT3BossWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT3BossWeaponComponent();

	virtual void BeginPlay() override;

	// 무기 외형 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	// 무기 판정 박스 (WeaponMeshComponent의 자식)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> WeaponHitBox;

	// 무기 부착 소켓 이름 (스켈레탈 메시에 정의된 소켓)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponSocketName = FName(TEXT("weapon_r"));

	// 무기 히트 델리게이트 — Monster에서 바인딩하여 데미지 적용
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponHitActor OnWeaponHitActor;

	// 소켓 부착 (BeginPlay에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachToSocket(USkeletalMeshComponent* TargetMesh);

	// 판정 ON/OFF (ON 시 히트 목록 초기화)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetAttackCollisionEnabled(bool bEnable);

	// 무기 드롭 (사망 연출용)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DropWeapon();

private:
	// 무기 드롭 여부
	bool bIsWeaponDropped = false;

	// 스윙당 히트된 액터 (중복 히트 방지)
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	UFUNCTION()
	void OnWeaponOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);
};
