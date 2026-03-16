#include "Interaction/T3LostMoney.h"

#include "Components/SphereComponent.h"
#include "GameSystem/T3GameMode.h"
#include "Item/Component/T3InventoryComponent.h"
#include "Player/T3CharacterBase.h"

AT3LostMoney::AT3LostMoney()
{
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComp);
}

void AT3LostMoney::BeginPlay()
{
	Super::BeginPlay();
	
	T3GameMode = Cast<AT3GameMode>(GetWorld()->GetAuthGameMode());
}

void AT3LostMoney::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnActorOverlap);
}

void AT3LostMoney::SetLostMoneyID(const int32 ID)
{
	LostMoneyID = ID;
}

void AT3LostMoney::SetMoney(const int32 Amount)
{
	Money = Amount;
}

void AT3LostMoney::OnActorOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool FromSweep, const FHitResult& SweepResult)
{
	//플레이어 캐릭터만 허용
	if (!OtherActor->ActorHasTag(PLAYER_TAG))
	{
		return;
	}
	
	//플레이어 캐릭터로 캐스팅
	TObjectPtr<AT3CharacterBase> T3Character = Cast<AT3CharacterBase>(OtherActor);
	if (!T3Character)
	{
		return;
	}
	
	//돈 회수 처리
	if (T3Character->InventoryComponent)
	{
		const int32 SetValue = T3Character->InventoryComponent->GetMoney() + Money;
		T3Character->InventoryComponent->SetMoney(SetValue);
	}
	if (const AT3GameMode* T3Gm = T3GameMode.Get())
	{
		T3Gm->RegainLostMoney(LostMoneyID);
	}
	
	//UE_LOG(LogTemp, Warning, TEXT("돈 회수 : %d"), Money);
	
	//액터 제거
	Destroy();
}
