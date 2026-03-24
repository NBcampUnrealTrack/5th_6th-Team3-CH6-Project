// T3PuzzleProps.cpp

#include "GameSystem/T3PuzzleProps.h"
#include "Desecration.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

// ============================================================
// AT3PuzzleBarrier
// ============================================================

AT3PuzzleBarrier::AT3PuzzleBarrier()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 (메시와 독립적으로 배치/스케일 조절)
	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	SetRootComponent(DefaultRoot);

	// 메시 (BP에서 에셋 설정 — 루트 아래 자식으로 스케일 독립)
	BarrierMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrierMesh"));
	BarrierMesh->SetupAttachment(DefaultRoot);

	// 통행 차단 콜리전
	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(DefaultRoot);
	BlockingCollision->SetBoxExtent(FVector(50.f, 300.f, 200.f));
	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Block);
}

void AT3PuzzleBarrier::BeginPlay()
{
	Super::BeginPlay();

	// BP에서 변경된 크기 반영
	if (BlockingCollision)
	{
		BlockingCollision->SetBoxExtent(BlockingExtent);
	}
}

void AT3PuzzleBarrier::SetBarrierActive_Implementation(bool bActive)
{
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);

	UE_LOG(LogDesecration, Log, TEXT("T3_PuzzleBarrier: %s — %s"),
		*GetName(), bActive ? TEXT("활성화") : TEXT("비활성화"));
}

// ============================================================
// AT3PuzzleLantern
// ============================================================

AT3PuzzleLantern::AT3PuzzleLantern()
{
	PrimaryActorTick.bCanEverTick = false;

	// 석등 메시 (BP에서 에셋 설정)
	LanternMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LanternMesh"));
	SetRootComponent(LanternMesh);

	// 라이트 (초기 OFF 상태)
	LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(LanternMesh);
	LanternLight->SetVisibility(false);
	LanternLight->SetIntensity(5000.f);
	LanternLight->SetLightColor(FLinearColor(1.f, 0.7f, 0.3f));
	LanternLight->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
}

void AT3PuzzleLantern::BeginPlay()
{
	Super::BeginPlay();

	// BP에서 설정한 값 반영
	if (LanternLight)
	{
		LanternLight->SetIntensity(LanternLightIntensity);
		LanternLight->SetLightColor(LanternLightColor);
		LanternLight->SetVisibility(false);
	}
}

void AT3PuzzleLantern::SetLanternLit_Implementation(bool bLit)
{
	if (LanternLight)
	{
		LanternLight->SetVisibility(bLit);
	}

	UE_LOG(LogDesecration, Verbose, TEXT("T3_PuzzleLantern: %s — %s"),
		*GetName(), bLit ? TEXT("ON") : TEXT("OFF"));
}
