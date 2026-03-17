// T3BossWeaponComponent.h
// 보스 무기 컴포넌트 — 메시, 판정, 소켓 부착, 드롭

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T3BossWeaponComponent.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class USphereComponent;
class USkeletalMeshComponent;
class UTimelineComponent;
class UCurveFloat;
class UNiagaraComponent;
class UNiagaraSystem;

// 무기 소켓 타입 — 애님팩별 그립 보정용
UENUM(BlueprintType)
enum class EWeaponSocketType : uint8
{
	Default		UMETA(DisplayName = "Default"),
	Alternative	UMETA(DisplayName = "Alternative"),
};

// 무기 히트 델리게이트 — 히트된 액터를 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponHitActor, AActor*, HitActor);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DESECRATION_API UT3BossWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT3BossWeaponComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 무기 외형 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> WeaponMeshComponent;

	// 무기 판정 박스 (WeaponMeshComponent의 자식)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> WeaponHitBox;

	// 넓은 판정 박스 — 대쉬 내려찍기 등 특수 공격용 (WeaponMeshComponent의 자식)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UBoxComponent> WeaponHitBoxWide;

	// 팔 공격 판정 구체 — 캐릭터 메시 본에 부착 (무기와 독립)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|BodyAttack")
	TObjectPtr<USphereComponent> BodyHitSphere;

	// 기본 소켓 이름 (팩 1 기준)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Socket")
	FName DefaultSocketName = FName(TEXT("weapon_r"));

	// 대체 소켓 이름 (팩 2 기준 — 에디터에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Socket")
	FName AlternativeSocketName = FName(TEXT("weapon_r_alt"));

	// 소켓 전환 블렌드 시간 (0이면 즉시 전환)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Socket", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SocketBlendDuration = 0.15f;

	// 블렌드 EaseOut 지수 (높을수록 시작에 빠르게 이동, 끝에서 미세 안착)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Socket", meta = (ClampMin = "1.0", ClampMax = "6.0"))
	float SocketBlendExponent = 3.5f;

	// 팔 공격 판정이 부착될 본 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|BodyAttack")
	FName BodyAttackBoneName = FName(TEXT("hand_r"));

	// 팔 공격 판정 구체 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|BodyAttack", meta = (ClampMin = "5.0", ClampMax = "100.0"))
	float BodyAttackRadius = 30.f;

	// 무기 오라 이펙트 — WeaponMeshComponent 자식으로 부착 (소켓 전환 무관하게 추적)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|FX")
	TObjectPtr<UNiagaraComponent> WeaponAuraEffect;

	// BP에서 할당할 Niagara 에셋
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|FX")
	TObjectPtr<UNiagaraSystem> WeaponAuraSystem;

	// 무기 히트 델리게이트 — Monster에서 바인딩하여 데미지 적용
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponHitActor OnWeaponHitActor;

	// 소켓 부착 (BeginPlay에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void AttachToSocket(USkeletalMeshComponent* TargetMesh);

	// 소켓 스위칭 — 드롭다운으로 선택
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchToSocket(EWeaponSocketType SocketType);

	// 기본 소켓으로 복귀
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void ResetToDefaultSocket();

	// 소켓 타입 → 이름 조회
	FName GetSocketNameByType(EWeaponSocketType SocketType) const;

	// 기본 판정 ON/OFF (ON 시 히트 목록 초기화)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetAttackCollisionEnabled(bool bEnable);

	// 넓은 판정 ON/OFF — 대쉬 내려찍기 등 (ON 시 히트 목록 초기화, 기본 히트박스와 공유)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetWideCollisionEnabled(bool bEnable);

	// 팔 공격 판정 ON/OFF (ON 시 히트 목록 초기화)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SetBodyAttackCollisionEnabled(bool bEnable);

	// 무기 오라 이펙트 활성화 (OverrideSystem 지정 시 해당 에셋 사용, 없으면 WeaponAuraSystem 사용)
	UFUNCTION(BlueprintCallable, Category = "Weapon|FX")
	void ActivateWeaponAura(UNiagaraSystem* OverrideSystem = nullptr);

	// 무기 오라 이펙트 비활성화
	UFUNCTION(BlueprintCallable, Category = "Weapon|FX")
	void DeactivateWeaponAura();

	// 무기 드롭 (사망 연출용)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void DropWeapon();

	// 무기 디졸브 (드롭 후 사라지는 연출)
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartWeaponDissolve(float Duration = 2.f, FName ParameterName = TEXT("Dissolve"));

	// 디졸브 파라미터 이름 (머티리얼에 정의된 Scalar Parameter)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dissolve")
	FName WeaponDissolveParameterName = TEXT("Dissolve");

	// 무기 디졸브 지속 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Dissolve")
	float WeaponDissolveDuration = 2.f;

private:
	// 무기 드롭 여부
	bool bIsWeaponDropped = false;

	// 부착 대상 스켈레탈 메시 캐싱 (소켓 스위칭용)
	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> CachedTargetMesh;

	// 소켓 블렌드 상태
	bool bIsBlendingSocket = false;
	float SocketBlendElapsed = 0.f;
	FTransform SocketBlendStartRelative = FTransform::Identity;

	// 스윙당 히트된 액터 (중복 히트 방지)
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActorsThisSwing;

	UFUNCTION()
	void OnWeaponOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 무기 디졸브용
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> WeaponDynamicMaterials;

	UPROPERTY()
	TObjectPtr<UTimelineComponent> WeaponDissolveTimeline;

	UPROPERTY()
	TObjectPtr<UCurveFloat> WeaponDissolveCurve;

	UFUNCTION()
	void OnWeaponDissolveUpdate(float Value);

	UFUNCTION()
	void OnWeaponDissolveFinished();
};
