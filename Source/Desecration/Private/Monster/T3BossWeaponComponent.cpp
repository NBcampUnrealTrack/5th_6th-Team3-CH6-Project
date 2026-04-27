#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "DrawDebugHelpers.h"

UT3BossWeaponComponent::UT3BossWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 무기 메시 (AttachToSocket에서 소켓 부착)
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetCanEverAffectNavigation(false);

	// 무기 판정 캡슐 — AttachToSocket()에서 WeaponMesh에 런타임 부착
	// (생성자 SetupAttachment는 UActorComponent 서브오브젝트에서 템플릿 불일치 유발)
	WeaponHitBox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("WeaponHitBox"));
	WeaponHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponHitBox->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponHitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponHitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponHitBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	WeaponHitBox->SetGenerateOverlapEvents(true);
	WeaponHitBox->SetCapsuleSize(10.f, 40.f);

	// BP에서 튜닝한 크기가 월드 크기 그대로 유지되도록 — 부모 스케일 체인(손소켓×WeaponMesh) 영향 차단
	// (위치/회전은 WeaponMesh 따라감 → 검 휘두름 추적 OK)
	WeaponHitBox->SetUsingAbsoluteScale(true);

	// 에디터에서 히트캡슐 와이어프레임 항상 표시 (선택하지 않아도 보임)
	WeaponHitBox->bDrawOnlyIfSelected = false;
	WeaponHitBox->ShapeColor = FColor::Red;
	WeaponHitBox->SetHiddenInGame(true);

	// 넓은 판정 캡슐 — 대쉬 내려찍기 등 특수 공격용 (런타임 부착)
	WeaponHitBoxWide = CreateDefaultSubobject<UCapsuleComponent>(TEXT("WeaponHitBoxWide"));
	WeaponHitBoxWide->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponHitBoxWide->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponHitBoxWide->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponHitBoxWide->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponHitBoxWide->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	WeaponHitBoxWide->SetGenerateOverlapEvents(true);
	WeaponHitBoxWide->SetCapsuleSize(30.f, 60.f);

	// 동일 — BP 튜닝 크기 보존
	WeaponHitBoxWide->SetUsingAbsoluteScale(true);

	WeaponHitBoxWide->bDrawOnlyIfSelected = false;
	WeaponHitBoxWide->ShapeColor = FColor::Orange;
	WeaponHitBoxWide->SetHiddenInGame(true);

	// 무기 오라 이펙트 (런타임 부착)
	WeaponAuraEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WeaponAuraEffect"));
	WeaponAuraEffect->SetAutoActivate(false);

	// 팔 공격 판정 구체 — AttachToSocket에서 캐릭터 메시 본에 부착
	BodyHitSphere = CreateDefaultSubobject<USphereComponent>(TEXT("BodyHitSphere"));
	BodyHitSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyHitSphere->SetCollisionObjectType(ECC_WorldDynamic);
	BodyHitSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyHitSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BodyHitSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	BodyHitSphere->SetGenerateOverlapEvents(true);
	BodyHitSphere->SetSphereRadius(BodyAttackRadius);

	BodyHitSphere->bDrawOnlyIfSelected = false;
	BodyHitSphere->ShapeColor = FColor::Cyan;
	BodyHitSphere->SetHiddenInGame(true);
}

void UT3BossWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// 오버랩 이벤트 바인딩
	if (WeaponHitBox)
	{
		WeaponHitBox->OnComponentBeginOverlap.AddDynamic(
			this, &UT3BossWeaponComponent::OnWeaponOverlapBegin);

		UE_LOG(LogDesecration, Log,
			TEXT("T3_BossWeapon: 오버랩 바인딩 완료 (GenerateOverlap:%d, ObjectType:%d)"),
			WeaponHitBox->GetGenerateOverlapEvents(),
			(int32)WeaponHitBox->GetCollisionObjectType());
	}
	else
	{
		UE_LOG(LogDesecration, Error, TEXT("T3_BossWeapon: BeginPlay — WeaponHitBox가 nullptr!"));
	}

	// 넓은 히트박스 오버랩 바인딩 (같은 콜백 공유 — HitActorsThisSwing으로 중복 방지)
	if (WeaponHitBoxWide)
	{
		WeaponHitBoxWide->OnComponentBeginOverlap.AddDynamic(
			this, &UT3BossWeaponComponent::OnWeaponOverlapBegin);
	}

	// 팔 공격 판정 오버랩 바인딩 (같은 콜백 공유)
	if (BodyHitSphere)
	{
		BodyHitSphere->OnComponentBeginOverlap.AddDynamic(
			this, &UT3BossWeaponComponent::OnWeaponOverlapBegin);
	}

	// 무기 메시 부착 상태 확인
	if (WeaponMeshComponent)
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: WeaponMesh 부착 상태 — Parent:%s, Location:%s"),
			WeaponMeshComponent->GetAttachParent() ? *WeaponMeshComponent->GetAttachParent()->GetName() : TEXT("없음"),
			*WeaponMeshComponent->GetComponentLocation().ToString());
	}

	// 오라 이펙트 부착 상태 확인
	if (WeaponAuraEffect)
	{
		USceneComponent* AuraParent = WeaponAuraEffect->GetAttachParent();
		UE_LOG(LogDesecration, Log,
			TEXT("T3_BossWeapon: WeaponAura 부착 상태 — Parent:%s, AutoActivate:%d, Asset:%s"),
			AuraParent ? *AuraParent->GetName() : TEXT("없음"),
			WeaponAuraEffect->bAutoActivate,
			WeaponAuraEffect->GetAsset() ? *WeaponAuraEffect->GetAsset()->GetName() : TEXT("미할당"));
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_BossWeapon: WeaponAuraEffect가 nullptr!"));
	}

	// 디버그 시각화 ON이면 틱 항상 활성화 (소켓 블렌드와 무관하게 매 프레임 DrawDebug)
	if (bShowDebugHitRange)
	{
		SetComponentTickEnabled(true);
	}
}

void UT3BossWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsBlendingSocket && WeaponMeshComponent)
	{
		SocketBlendElapsed += DeltaTime;
		float Alpha = FMath::Clamp(SocketBlendElapsed / SocketBlendDuration, 0.f, 1.f);

		// EaseOut — 시작에 빠르게 이동, 끝에서 미세 안착
		Alpha = FMath::InterpEaseOut(0.f, 1.f, Alpha, SocketBlendExponent);

		// 블렌드 종착점 = Identity (WeaponMesh는 소켓 원점에 Snap 부착)
		FTransform BlendedTransform;
		BlendedTransform.Blend(SocketBlendStartRelative, FTransform::Identity, Alpha);
		WeaponMeshComponent->SetRelativeTransform(BlendedTransform);

		if (Alpha >= 1.f)
		{
			WeaponMeshComponent->SetRelativeTransform(FTransform::Identity);
			bIsBlendingSocket = false;
			// 디버그 시각화 중이면 틱 유지, 아니면 비활성화
			if (!bShowDebugHitRange)
			{
				SetComponentTickEnabled(false);
			}
		}
	}

	if (bShowDebugHitRange)
	{
		DrawDebugHitShapes();
	}
}

void UT3BossWeaponComponent::AttachToSocket(USkeletalMeshComponent* TargetMesh)
{
	if (!WeaponMeshComponent || !TargetMesh)
	{
		UE_LOG(LogDesecration, Error, TEXT("T3_BossWeapon: AttachToSocket 실패 — Mesh:%s, TargetMesh:%s"),
			WeaponMeshComponent ? TEXT("유효") : TEXT("nullptr"),
			TargetMesh ? TEXT("유효") : TEXT("nullptr"));
		return;
	}

	// 소켓 존재 확인
	if (!TargetMesh->DoesSocketExist(DefaultSocketName))
	{
		UE_LOG(LogDesecration, Error, TEXT("T3_BossWeapon: 소켓 '%s'이 스켈레탈 메시에 없음! 사용 가능한 소켓:"),
			*DefaultSocketName.ToString());

		TArray<FName> AllSockets = TargetMesh->GetAllSocketNames();
		for (const FName& SocketName : AllSockets)
		{
			UE_LOG(LogDesecration, Log, TEXT("  → %s"), *SocketName.ToString());
		}
		return;
	}

	// WeaponMesh는 소켓 원점이 곧 그립 → Snap (BP 프리뷰용 오프셋은 의미 없음, 소켓 기준 정렬)
	WeaponMeshComponent->AttachToComponent(
		TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, DefaultSocketName);

	// HitBox는 WeaponMesh 기준 오프셋(블레이드 끝 등)이 의미 있음 → KeepRelative로 BP 오프셋 보존
	// (생성자의 SetupAttachment가 UActorComponent 내부 생성 시 런타임에 유지 안 됨)
	// AbsoluteScale 강제 — 손 소켓 스케일(2x) 체인 무시, BP 튜닝한 RelScale을 그대로 월드 스케일로
	// (생성자에서만 켜면 BP 직렬화값(false)이 덮어쓰므로 런타임에 재설정 필수)

	// BP 튜닝 RelLoc 최초 1회 캐시 — 후속 부착에서 누적 ÷ 방지
	if (!bHitBoxBPLocCached)
	{
		if (WeaponHitBox)     HitBoxBPRelLoc     = WeaponHitBox->GetRelativeLocation();
		if (WeaponHitBoxWide) HitBoxWideBPRelLoc = WeaponHitBoxWide->GetRelativeLocation();
		bHitBoxBPLocCached = true;
	}

	if (WeaponHitBox)
	{
		WeaponHitBox->SetUsingAbsoluteScale(true);
		WeaponHitBox->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}

	if (WeaponHitBoxWide)
	{
		WeaponHitBoxWide->SetUsingAbsoluteScale(true);
		WeaponHitBoxWide->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}

	// 위치 보정 — bAbsoluteScale은 크기만 격리하고 RelLoc은 여전히 부모.WorldScale이 곱해짐
	// 따라서 BP 튜닝 위치를 부모스케일로 나눠 적용 → 월드 오프셋 = BP RelLoc 그대로
	{
		const FVector ParentWorldScale = WeaponMeshComponent->GetComponentScale();
		auto CompensateRelLoc = [&ParentWorldScale](USceneComponent* Comp, const FVector& BPLoc)
		{
			if (!Comp) return;
			const FVector Safe(
				FMath::IsNearlyZero(ParentWorldScale.X) ? 1.f : ParentWorldScale.X,
				FMath::IsNearlyZero(ParentWorldScale.Y) ? 1.f : ParentWorldScale.Y,
				FMath::IsNearlyZero(ParentWorldScale.Z) ? 1.f : ParentWorldScale.Z);
			Comp->SetRelativeLocation(FVector(BPLoc.X / Safe.X, BPLoc.Y / Safe.Y, BPLoc.Z / Safe.Z));
		};
		CompensateRelLoc(WeaponHitBox, HitBoxBPRelLoc);
		CompensateRelLoc(WeaponHitBoxWide, HitBoxWideBPRelLoc);
	}

	// 오라 이펙트 재부착 + 에셋 할당 (KeepRelative — BP 오프셋 유지)
	if (WeaponAuraEffect)
	{
		WeaponAuraEffect->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);

		if (WeaponAuraSystem)
		{
			WeaponAuraEffect->SetAsset(WeaponAuraSystem);
		}
	}

	// 소켓 스위칭용 캐싱
	CachedTargetMesh = TargetMesh;

	// 팔 공격 판정 구체 — 본 자체가 손 원점 → Snap
	if (BodyHitSphere && TargetMesh->DoesSocketExist(BodyAttackBoneName))
	{
		BodyHitSphere->AttachToComponent(
			TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, BodyAttackBoneName);

		// 런타임에 반경 반영
		BodyHitSphere->SetSphereRadius(BodyAttackRadius);

		UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: BodyHitSphere → 본 '%s'에 부착 (반경:%.0f)"),
			*BodyAttackBoneName.ToString(), BodyAttackRadius);
	}
	else if (BodyHitSphere)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_BossWeapon: 본 '%s'이 스켈레탈 메시에 없음 — BodyHitSphere 미부착"),
			*BodyAttackBoneName.ToString());
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 소켓 '%s'에 부착 성공 (Mesh위치:%s, HitBox위치:%s)"),
		*DefaultSocketName.ToString(),
		*WeaponMeshComponent->GetComponentLocation().ToString(),
		WeaponHitBox ? *WeaponHitBox->GetComponentLocation().ToString() : TEXT("nullptr"));

	// 스케일 체인 진단 — BP 튜닝 vs 인게임 어디서 커지는지 추적
	{
		const FVector HandSocketWorldScale = TargetMesh->GetSocketTransform(DefaultSocketName, RTS_World).GetScale3D();
		const FVector MeshRelScale = WeaponMeshComponent->GetRelativeScale3D();
		const FVector MeshWorldScale = WeaponMeshComponent->GetComponentScale();
		UE_LOG(LogDesecration, Warning, TEXT("[Scale진단] HandSocket월드:%s | WeaponMesh Rel:%s World:%s"),
			*HandSocketWorldScale.ToString(), *MeshRelScale.ToString(), *MeshWorldScale.ToString());

		if (WeaponHitBox)
		{
			UE_LOG(LogDesecration, Warning,
				TEXT("[Scale진단] HitBox bAbsScale:%d Rel:%s World:%s | CapsuleSize H:%.2f R:%.2f (Scaled H:%.2f R:%.2f)"),
				WeaponHitBox->IsUsingAbsoluteScale() ? 1 : 0,
				*WeaponHitBox->GetRelativeScale3D().ToString(),
				*WeaponHitBox->GetComponentScale().ToString(),
				WeaponHitBox->GetUnscaledCapsuleHalfHeight(), WeaponHitBox->GetUnscaledCapsuleRadius(),
				WeaponHitBox->GetScaledCapsuleHalfHeight(), WeaponHitBox->GetScaledCapsuleRadius());
		}
		if (WeaponHitBoxWide)
		{
			UE_LOG(LogDesecration, Warning,
				TEXT("[Scale진단] HitBoxWide bAbsScale:%d Rel:%s World:%s | CapsuleSize H:%.2f R:%.2f (Scaled H:%.2f R:%.2f)"),
				WeaponHitBoxWide->IsUsingAbsoluteScale() ? 1 : 0,
				*WeaponHitBoxWide->GetRelativeScale3D().ToString(),
				*WeaponHitBoxWide->GetComponentScale().ToString(),
				WeaponHitBoxWide->GetUnscaledCapsuleHalfHeight(), WeaponHitBoxWide->GetUnscaledCapsuleRadius(),
				WeaponHitBoxWide->GetScaledCapsuleHalfHeight(), WeaponHitBoxWide->GetScaledCapsuleRadius());
		}
	}
}

FName UT3BossWeaponComponent::GetSocketNameByType(EWeaponSocketType SocketType) const
{
	switch (SocketType)
	{
	case EWeaponSocketType::Alternative:
		return AlternativeSocketName;
	default:
		return DefaultSocketName;
	}
}

void UT3BossWeaponComponent::SwitchToSocket(EWeaponSocketType SocketType)
{
	if (!CachedTargetMesh || !WeaponMeshComponent || bIsWeaponDropped)
	{
		return;
	}

	const FName TargetSocketName = GetSocketNameByType(SocketType);

	if (!CachedTargetMesh->DoesSocketExist(TargetSocketName))
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_BossWeapon: 소켓 스위칭 실패 — '%s' 소켓 없음"),
			*TargetSocketName.ToString());
		return;
	}

	// 블렌드용: 전환 전 월드 트랜스폼 저장
	const FTransform OldWorldTransform = WeaponMeshComponent->GetComponentTransform();

	// 새 소켓 부착 — Snap (WeaponMesh는 소켓 원점이 곧 그립)
	WeaponMeshComponent->AttachToComponent(
		CachedTargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocketName);

	// 히트박스 재부착 (KeepRelative — BP에서 튜닝한 WeaponMesh 기준 오프셋 유지)
	// AbsoluteScale 보장 (소켓 전환 시에도 부모 스케일 체인 차단 유지)
	if (WeaponHitBox)
	{
		WeaponHitBox->SetUsingAbsoluteScale(true);
		WeaponHitBox->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (WeaponHitBoxWide)
	{
		WeaponHitBoxWide->SetUsingAbsoluteScale(true);
		WeaponHitBoxWide->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}

	// RelLoc 부모 스케일 보정 (AttachToSocket과 동일 — BP RelLoc ÷ 부모스케일)
	{
		const FVector ParentWorldScale = WeaponMeshComponent->GetComponentScale();
		auto CompensateRelLoc = [&ParentWorldScale](USceneComponent* Comp, const FVector& BPLoc)
		{
			if (!Comp) return;
			const FVector Safe(
				FMath::IsNearlyZero(ParentWorldScale.X) ? 1.f : ParentWorldScale.X,
				FMath::IsNearlyZero(ParentWorldScale.Y) ? 1.f : ParentWorldScale.Y,
				FMath::IsNearlyZero(ParentWorldScale.Z) ? 1.f : ParentWorldScale.Z);
			Comp->SetRelativeLocation(FVector(BPLoc.X / Safe.X, BPLoc.Y / Safe.Y, BPLoc.Z / Safe.Z));
		};
		CompensateRelLoc(WeaponHitBox, HitBoxBPRelLoc);
		CompensateRelLoc(WeaponHitBoxWide, HitBoxWideBPRelLoc);
	}
	if (WeaponAuraEffect)
	{
		WeaponAuraEffect->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::KeepRelativeTransform);
	}

	// 블렌드 시작 — 옛 위치에서 새 소켓 원점으로 부드럽게 전환
	if (SocketBlendDuration > 0.f)
	{
		// Snap 부착 직후 GetComponentTransform()은 새 소켓 월드와 동일
		const FTransform NewSocketWorld = WeaponMeshComponent->GetComponentTransform();
		// SetRelativeTransform(X) 시 mesh 월드 = NewSocketWorld × X — OldWorld 위치 유지하려면 X = SocketWorld⁻¹ × OldWorld
		SocketBlendStartRelative = OldWorldTransform.GetRelativeTransform(NewSocketWorld);
		WeaponMeshComponent->SetRelativeTransform(SocketBlendStartRelative);
		SocketBlendElapsed = 0.f;
		bIsBlendingSocket = true;
		SetComponentTickEnabled(true);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 소켓 스위칭 → '%s' (블렌드:%.2f초)"),
		*TargetSocketName.ToString(), SocketBlendDuration);
}

void UT3BossWeaponComponent::ResetToDefaultSocket()
{
	SwitchToSocket(EWeaponSocketType::Default);
}

void UT3BossWeaponComponent::SetAttackCollisionEnabled(bool bEnable)
{
	if (WeaponHitBox)
	{
		if (bEnable)
		{
			HitActorsThisSwing.Reset();
		}

		WeaponHitBox->SetCollisionEnabled(
			bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

		UE_LOG(LogDesecration, Log,
			TEXT("T3_BossWeapon: 무기 콜리전 %s (위치:%s, 반경:%.0f, 반높이:%.0f, GenerateOverlap:%d)"),
			bEnable ? TEXT("ON") : TEXT("OFF"),
			*WeaponHitBox->GetComponentLocation().ToString(),
			WeaponHitBox->GetUnscaledCapsuleRadius(),
			WeaponHitBox->GetUnscaledCapsuleHalfHeight(),
			WeaponHitBox->GetGenerateOverlapEvents());
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_BossWeapon: WeaponHitBox가 nullptr!"));
	}
}

void UT3BossWeaponComponent::SetWideCollisionEnabled(bool bEnable)
{
	if (WeaponHitBoxWide)
	{
		if (bEnable)
		{
			HitActorsThisSwing.Reset();
		}

		WeaponHitBoxWide->SetCollisionEnabled(
			bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

		UE_LOG(LogDesecration, Log,
			TEXT("T3_BossWeapon: 넓은 콜리전 %s (반경:%.0f, 반높이:%.0f)"),
			bEnable ? TEXT("ON") : TEXT("OFF"),
			WeaponHitBoxWide->GetUnscaledCapsuleRadius(),
			WeaponHitBoxWide->GetUnscaledCapsuleHalfHeight());
	}
}

void UT3BossWeaponComponent::SetBodyAttackCollisionEnabled(bool bEnable)
{
	if (BodyHitSphere)
	{
		if (bEnable)
		{
			HitActorsThisSwing.Reset();
		}

		BodyHitSphere->SetCollisionEnabled(
			bEnable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);

		UE_LOG(LogDesecration, Log,
			TEXT("T3_BossWeapon: 팔 공격 콜리전 %s (본:%s, 반경:%.0f, 위치:%s)"),
			bEnable ? TEXT("ON") : TEXT("OFF"),
			*BodyAttackBoneName.ToString(),
			BodyHitSphere->GetUnscaledSphereRadius(),
			*BodyHitSphere->GetComponentLocation().ToString());
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_BossWeapon: BodyHitSphere가 nullptr!"));
	}
}

void UT3BossWeaponComponent::OnWeaponOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// 어떤 히트박스에서 오버랩 발생했는지 구분
	const bool bIsBodyAttack = (OverlappedComp == BodyHitSphere);
	const bool bIsWideHitBox = (OverlappedComp == WeaponHitBoxWide);
	const FString HitBoxName = bIsBodyAttack ? TEXT("Body") : (bIsWideHitBox ? TEXT("Wide") : TEXT("Normal"));

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: [오버랩] HitBox:%s, Other:%s, Comp:%s"),
		*HitBoxName,
		OtherActor ? *OtherActor->GetName() : TEXT("nullptr"),
		OtherComp ? *OtherComp->GetName() : TEXT("nullptr"));

	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	// 스윙당 중복 히트 방지 (Normal과 Wide가 같은 목록 공유)
	if (HitActorsThisSwing.Contains(OtherActor))
	{
		UE_LOG(LogDesecration, Verbose, TEXT("T3_BossWeapon: [중복 무시] HitBox:%s, Actor:%s"),
			*HitBoxName, *OtherActor->GetName());
		return;
	}

	HitActorsThisSwing.Add(OtherActor);
	OnWeaponHitActor.Broadcast(OtherActor);

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 히트 확정 — HitBox:%s → %s"),
		*HitBoxName, *OtherActor->GetName());
}

void UT3BossWeaponComponent::ActivateWeaponAura(UNiagaraSystem* OverrideSystem)
{
	if (WeaponAuraEffect)
	{
		// 노티파이에서 지정한 에셋 우선, 없으면 컴포넌트 기본값
		UNiagaraSystem* SystemToUse = OverrideSystem ? OverrideSystem : WeaponAuraSystem.Get();

		if (SystemToUse && WeaponAuraEffect->GetAsset() != SystemToUse)
		{
			WeaponAuraEffect->SetAsset(SystemToUse);
		}

		WeaponAuraEffect->Activate(true);

		USceneComponent* AuraParent = WeaponAuraEffect->GetAttachParent();
		const FString ParentName = AuraParent ? *AuraParent->GetName() : TEXT("미부착");
		const FString ParentOwnerName = (AuraParent && AuraParent->GetOwner()) ? *AuraParent->GetOwner()->GetName() : TEXT("없음");
		UE_LOG(LogDesecration, Warning,
			TEXT("T3_BossWeapon: 오라 활성화 — 에셋:%s, Parent:%s (Owner:%s), Active:%d, 오라위치:%s, 무기위치:%s"),
			SystemToUse ? *SystemToUse->GetName() : TEXT("없음"),
			*ParentName,
			*ParentOwnerName,
			WeaponAuraEffect->IsActive(),
			*WeaponAuraEffect->GetComponentLocation().ToString(),
			WeaponMeshComponent ? *WeaponMeshComponent->GetComponentLocation().ToString() : TEXT("nullptr"));
	}
	else
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_BossWeapon: WeaponAuraEffect nullptr — 오라 활성화 실패"));
	}
}

void UT3BossWeaponComponent::DeactivateWeaponAura()
{
	if (WeaponAuraEffect)
	{
		WeaponAuraEffect->Deactivate();
		UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 무기 오라 비활성화"));
	}
}

void UT3BossWeaponComponent::DropWeapon()
{
	if (bIsWeaponDropped || !WeaponMeshComponent)
	{
		return;
	}

	bIsWeaponDropped = true;

	// 소켓 블렌드 중단 — Detach 후 Tick이 상대 트랜스폼을 덮어쓰는 것 방지
	bIsBlendingSocket = false;
	SetComponentTickEnabled(false);

	// 오라 이펙트 비활성화
	DeactivateWeaponAura();

	// 판정 비활성화
	SetAttackCollisionEnabled(false);

	// 메시를 소켓에서 분리 → 물리 시뮬레이션
	WeaponMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WeaponMeshComponent->SetSimulatePhysics(true);
	WeaponMeshComponent->SetLinearDamping(0.5f);
	WeaponMeshComponent->SetAngularDamping(1.0f);

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 무기 드롭"));
}

void UT3BossWeaponComponent::StartWeaponDissolve(float Duration, FName ParameterName)
{
	if (!WeaponMeshComponent)
	{
		return;
	}

	// 물리 시뮬 정지 — 디졸브 중 굴러다니지 않도록
	WeaponMeshComponent->SetSimulatePhysics(false);
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// DynamicMaterial 생성
	const int32 NumMaterials = WeaponMeshComponent->GetNumMaterials();
	WeaponDynamicMaterials.Reserve(NumMaterials);

	for (int32 i = 0; i < NumMaterials; ++i)
	{
		UMaterialInstanceDynamic* DynMat = WeaponMeshComponent->CreateAndSetMaterialInstanceDynamic(i);
		if (DynMat)
		{
			WeaponDynamicMaterials.Add(DynMat);
		}
	}

	if (WeaponDynamicMaterials.Num() == 0)
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_BossWeapon: 무기 디졸브 — DynamicMaterial 0개, 스킵"));
		return;
	}

	// 파라미터 이름 저장
	WeaponDissolveParameterName = ParameterName;

	// 선형 커브 자동 생성
	WeaponDissolveCurve = NewObject<UCurveFloat>(this);
	WeaponDissolveCurve->FloatCurve.AddKey(0.f, 0.f);
	WeaponDissolveCurve->FloatCurve.AddKey(Duration, 1.f);

	// Timeline 생성 + 바인딩
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	WeaponDissolveTimeline = NewObject<UTimelineComponent>(Owner, TEXT("WeaponDissolveTimeline"));
	WeaponDissolveTimeline->RegisterComponent();

	FOnTimelineFloat UpdateDelegate;
	UpdateDelegate.BindUFunction(this, FName("OnWeaponDissolveUpdate"));

	FOnTimelineEvent FinishedDelegate;
	FinishedDelegate.BindUFunction(this, FName("OnWeaponDissolveFinished"));

	WeaponDissolveTimeline->AddInterpFloat(WeaponDissolveCurve, UpdateDelegate, FName("WeaponDissolveTrack"));
	WeaponDissolveTimeline->SetTimelineFinishedFunc(FinishedDelegate);
	WeaponDissolveTimeline->SetTimelineLength(Duration);
	WeaponDissolveTimeline->SetLooping(false);
	WeaponDissolveTimeline->PlayFromStart();

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 무기 디졸브 시작 (%.1f초, 머티리얼 %d개)"),
		Duration, WeaponDynamicMaterials.Num());
}

void UT3BossWeaponComponent::OnWeaponDissolveUpdate(float Value)
{
	for (UMaterialInstanceDynamic* DynMat : WeaponDynamicMaterials)
	{
		if (DynMat)
		{
			DynMat->SetScalarParameterValue(WeaponDissolveParameterName, Value);
		}
	}
}

void UT3BossWeaponComponent::OnWeaponDissolveFinished()
{
	// 무기 메시 숨김
	if (WeaponMeshComponent)
	{
		WeaponMeshComponent->SetVisibility(false);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 무기 디졸브 완료"));
}

void UT3BossWeaponComponent::DrawDebugHitShapes() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 콜리전 활성 여부에 따라 색상 강조 — 활성: 풀컬러, 비활성: 회색조 디밍
	auto DrawCapsuleVisualizer = [World](const UCapsuleComponent* Cap, const FColor& ActiveColor)
	{
		if (!Cap)
		{
			return;
		}
		const bool bActive = (Cap->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
		const FColor DrawColor = bActive ? ActiveColor : FColor(80, 80, 80);
		const float Thickness = bActive ? 1.5f : 0.5f;
		DrawDebugCapsule(
			World,
			Cap->GetComponentLocation(),
			Cap->GetScaledCapsuleHalfHeight(),
			Cap->GetScaledCapsuleRadius(),
			Cap->GetComponentQuat(),
			DrawColor,
			false,   // bPersistentLines
			-1.f,    // LifeTime (한 프레임)
			0,       // DepthPriority
			Thickness
		);
	};

	DrawCapsuleVisualizer(WeaponHitBox, FColor::Red);
	DrawCapsuleVisualizer(WeaponHitBoxWide, FColor::Orange);

	// 팔 공격 구체
	if (BodyHitSphere)
	{
		const bool bActive = (BodyHitSphere->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
		const FColor DrawColor = bActive ? FColor::Yellow : FColor(80, 80, 80);
		const float Thickness = bActive ? 1.5f : 0.5f;
		DrawDebugSphere(
			World,
			BodyHitSphere->GetComponentLocation(),
			BodyHitSphere->GetScaledSphereRadius(),
			16,
			DrawColor,
			false,
			-1.f,
			0,
			Thickness
		);
	}
}
