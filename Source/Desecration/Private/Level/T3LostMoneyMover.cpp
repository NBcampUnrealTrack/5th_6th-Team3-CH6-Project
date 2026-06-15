#include "Level/T3LostMoneyMover.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Interaction/T3LostMoney.h"

AT3LostMoneyMover::AT3LostMoneyMover()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComp);
	
	ActorCountPerLine = 5;
}

void AT3LostMoneyMover::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnActorOverlap);
}

void AT3LostMoneyMover::BeginPlay()
{
	Super::BeginPlay();
	
	MovedActorCount = 0;
}

void AT3LostMoneyMover::OnActorOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool FromSweep, const FHitResult& SweepResult)
{
	//잃어버린 재화가 아니면 무시
	if (!OtherActor->ActorHasTag(LOST_MONEY_TAG))
	{
		return;
	}

	//여려개 옮길 경우 간격 벌리기
	FVector NewDestination = MoveDestination;
	if (MovedActorCount > 0)
	{
		if (DistanceBetweenActors == 0)
		{
			if (const TObjectPtr<AT3LostMoney> LostMoney = Cast<AT3LostMoney>(OtherActor))
			{
				DistanceBetweenActors = LostMoney->GetCollisionSphere()->GetScaledSphereRadius();
			}
		}

		//지정된 거리 (또는 잃어버린 재화 액터의 콜리전에 따라) Y 방향으로 배치하며 줄당 개수가 초과될 때마다 X 방향으로 늘어난다.
		const FVector AddValue = FVector((MovedActorCount / ActorCountPerLine) * DistanceBetweenActors, (MovedActorCount % ActorCountPerLine) * DistanceBetweenActors, 0);
		NewDestination += AddValue;
	}
	++MovedActorCount;
	
	//지정한 위치로 옮기기
	OtherActor->SetActorLocation(NewDestination);
}
