#include "Interaction/T3SavePoint.h"

#include "Components/BoxComponent.h"
#include "GameSystem/GlobalEnums.h"
#include "GameSystem/T3GameMode.h"
#include "Player/T3CharacterBase.h"

AT3SavePoint::AT3SavePoint()
{
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);
	SavePointMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SavePointMesh"));
	SavePointMesh->SetupAttachment(RootComp);
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComp);
}

void AT3SavePoint::BeginPlay()
{
	Super::BeginPlay();
	
	T3GameMode = Cast<AT3GameMode>(GetWorld()->GetAuthGameMode());
	if (!T3GameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameMode is NULL"), *GetNameSafe(this));
		return;
	}
}

void AT3SavePoint::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnActorOverlap);
}

void AT3SavePoint::OnActorOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool FromSweep, const FHitResult& SweepResult)
{
	//플레이어 캐릭터만 허용
	if (!OtherActor->ActorHasTag(PLAYER_TAG))
	{
		return;
	}
	
}
