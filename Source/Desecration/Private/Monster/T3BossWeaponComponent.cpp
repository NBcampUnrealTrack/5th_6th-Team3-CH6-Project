#include "Monster/T3BossWeaponComponent.h"
#include "Desecration.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"

UT3BossWeaponComponent::UT3BossWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

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

	// 무기 메시 부착 상태 확인
	if (WeaponMeshComponent)
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: WeaponMesh 부착 상태 — Parent:%s, Location:%s"),
			WeaponMeshComponent->GetAttachParent() ? *WeaponMeshComponent->GetAttachParent()->GetName() : TEXT("없음"),
			*WeaponMeshComponent->GetComponentLocation().ToString());
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
	if (!TargetMesh->DoesSocketExist(WeaponSocketName))
	{
		UE_LOG(LogDesecration, Error, TEXT("T3_BossWeapon: 소켓 '%s'이 스켈레탈 메시에 없음! 사용 가능한 소켓:"),
			*WeaponSocketName.ToString());

		TArray<FName> AllSockets = TargetMesh->GetAllSocketNames();
		for (const FName& SocketName : AllSockets)
		{
			UE_LOG(LogDesecration, Log, TEXT("  → %s"), *SocketName.ToString());
		}
		return;
	}

	WeaponMeshComponent->AttachToComponent(
		TargetMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName);

	// HitBox를 WeaponMesh에 명시적 재부착
	// (생성자의 SetupAttachment가 UActorComponent 내부 생성 시 런타임에 유지 안 됨)
	if (WeaponHitBox)
	{
		WeaponHitBox->AttachToComponent(
			WeaponMeshComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 소켓 '%s'에 부착 성공 (Mesh위치:%s, HitBox위치:%s)"),
		*WeaponSocketName.ToString(),
		*WeaponMeshComponent->GetComponentLocation().ToString(),
		WeaponHitBox ? *WeaponHitBox->GetComponentLocation().ToString() : TEXT("nullptr"));
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

void UT3BossWeaponComponent::OnWeaponOverlapBegin(UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	// 디버그: 오버랩 발생 자체를 확인 (필터링 전)
	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: [오버랩 발생] Other:%s, Comp:%s"),
		OtherActor ? *OtherActor->GetName() : TEXT("nullptr"),
		OtherComp ? *OtherComp->GetName() : TEXT("nullptr"));

	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	// 스윙당 중복 히트 방지
	if (HitActorsThisSwing.Contains(OtherActor))
	{
		return;
	}

	HitActorsThisSwing.Add(OtherActor);
	OnWeaponHitActor.Broadcast(OtherActor);

	UE_LOG(LogDesecration, Log, TEXT("T3_BossWeapon: 히트 감지 → %s (델리게이트 브로드캐스트)"), *OtherActor->GetName());
}

void UT3BossWeaponComponent::DropWeapon()
{
	if (bIsWeaponDropped || !WeaponMeshComponent)
	{
		return;
	}

	bIsWeaponDropped = true;

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
