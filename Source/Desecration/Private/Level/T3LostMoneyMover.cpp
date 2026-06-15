#include "Level/T3LostMoneyMover.h"

#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "GameSystem/T3GameMode.h"
#include "Interaction/T3LostMoney.h"

AT3LostMoneyMover::AT3LostMoneyMover()
{
	PrimaryActorTick.bCanEverTick = false;
	
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComp);
	
	DestinationComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DestinationComponent"));
	DestinationComponent->SetupAttachment(RootComp);
	
	ActorCountPerLine = 5;
}

void AT3LostMoneyMover::BeginPlay()
{
	Super::BeginPlay();
	
	MovedActorCount = 0;
	
	//현재 이 액터와 겹친 상태인 잃어버린 재화 액터를 옮긴다.
	if (const TObjectPtr<AT3GameMode> T3GameMode = Cast<AT3GameMode>(GetWorld()->GetAuthGameMode()))
	{
		//게임 모드에서 잃어버린 재화 생성이 완료된 다음에 실행하기, 아니면 생성 완료까지 계속 확인
		if (T3GameMode->GetSpawnLostMoneyEnd())
		{
			MoveLostMoneyActors();
		}
		else
		{
			GetWorldTimerManager().SetTimer(MoveLostMoneyTimerHandle, FTimerDelegate::CreateLambda([&]()
			{
				if (T3GameMode->GetSpawnLostMoneyEnd())
				{
					MoveLostMoneyActors();
					GetWorldTimerManager().ClearTimer(MoveLostMoneyTimerHandle);
				}
			}), 0.5f, true);
		}
	}
}

void AT3LostMoneyMover::MoveLostMoneyActors()
{
	//일단 겹친 상태인 액터를 모두 가져온다.
	TArray<AActor*> OverlappingActors;
	CollisionBox->GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		//잃어버린 재화가 아니면 무시
		if (!OverlappingActor->ActorHasTag(LOST_MONEY_TAG))
		{
			continue;
		}
		
		//여려개 옮길 경우 간격 벌리기
		FVector NewDestination = DestinationComponent->GetComponentLocation();
		if (MovedActorCount > 0)
		{
			if (DistanceBetweenActors == 0)
			{
				if (const TObjectPtr<AT3LostMoney> LostMoney = Cast<AT3LostMoney>(OverlappingActor))
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
		OverlappingActor->SetActorLocation(NewDestination);
	}
}
