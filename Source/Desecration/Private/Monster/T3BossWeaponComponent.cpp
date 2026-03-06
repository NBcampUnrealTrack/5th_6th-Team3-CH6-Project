#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"

UT3BossWeaponComponent::UT3BossWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 무기 메시 (AttachToSocket에서 소켓 부착)
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMeshComponent->SetCanEverAffectNavigation(false);

	// 무기 판정 박스 (WeaponMesh 자식)
	WeaponHitBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponHitBox"));
	WeaponHitBox->SetupAttachment(WeaponMeshComponent);
	WeaponHitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponHitBox->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponHitBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponHitBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponHitBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	WeaponHitBox->SetGenerateOverlapEvents(true);
	WeaponHitBox->SetBoxExtent(FVector(10.f, 5.f, 40.f));

	// 에디터에서 히트박스 와이어프레임 항상 표시 (선택하지 않아도 보임)
	WeaponHitBox->bDrawOnlyIfSelected = false;
	WeaponHitBox->ShapeColor = FColor::Red;
	WeaponHitBox->SetHiddenInGame(true);

	// 넓은 판정 박스 — 대쉬 내려찍기 등 특수 공격용
	WeaponHitBoxWide = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponHitBoxWide"));
	WeaponHitBoxWide->SetupAttachment(WeaponMeshComponent);
	WeaponHitBoxWide->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponHitBoxWide->SetCollisionObjectType(ECC_WorldDynamic);
	WeaponHitBoxWide->SetCollisionResponseToAllChannels(ECR_Ignore);
	WeaponHitBoxWide->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponHitBoxWide->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	WeaponHitBoxWide->SetGenerateOverlapEvents(true);
	WeaponHitBoxWide->SetBoxExtent(FVector(30.f, 30.f, 60.f));

	WeaponHitBoxWide->bDrawOnlyIfSelected = false;
	WeaponHitBoxWide->ShapeColor = FColor::Orange;
	WeaponHitBoxWide->SetHiddenInGame(true);

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

		FTransform BlendedTransform;
		BlendedTransform.Blend(SocketBlendStartRelative, FTransform::Identity, Alpha);
		WeaponMeshComponent->SetRelativeTransform(BlendedTransform);

		if (Alpha >= 1.f)
		{
			WeaponMeshComponent->SetRelativeTransform(FTransform::Identity);
			bIsBlendingSocket = false;
			SetComponentTickEnabled(false);
		}
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

	WeaponMeshComponent->AttachToComponent(
		TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, DefaultSocketName);

	// HitBox를 WeaponMesh에 명시적 재부착
	// (생성자의 SetupAttachment가 UActorComponent 내부 생성 시 런타임에 유지 안 됨)
	if (WeaponHitBox)
	{
		WeaponHitBox->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	if (WeaponHitBoxWide)
	{
		WeaponHitBoxWide->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	// 소켓 스위칭용 캐싱
	CachedTargetMesh = TargetMesh;

	// 팔 공격 판정 구체를 캐릭터 메시 본에 부착
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

	// 새 소켓에 스냅 (상대 트랜스폼 = Identity)
	WeaponMeshComponent->AttachToComponent(
		CachedTargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetSocketName);

	// 히트박스 재부착
	if (WeaponHitBox)
	{
		WeaponHitBox->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	if (WeaponHitBoxWide)
	{
		WeaponHitBoxWide->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	// 블렌드 시작 — 옛 위치에서 새 소켓으로 부드럽게 전환
	if (SocketBlendDuration > 0.f)
	{
		const FTransform NewSocketWorldTransform = WeaponMeshComponent->GetComponentTransform();
		SocketBlendStartRelative = OldWorldTransform.GetRelativeTransform(NewSocketWorldTransform);
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
			TEXT("T3_BossWeapon: 무기 콜리전 %s (위치:%s, Extent:%s, GenerateOverlap:%d)"),
			bEnable ? TEXT("ON") : TEXT("OFF"),
			*WeaponHitBox->GetComponentLocation().ToString(),
			*WeaponHitBox->GetUnscaledBoxExtent().ToString(),
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
			TEXT("T3_BossWeapon: 넓은 콜리전 %s (Extent:%s)"),
			bEnable ? TEXT("ON") : TEXT("OFF"),
			*WeaponHitBoxWide->GetUnscaledBoxExtent().ToString());
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
