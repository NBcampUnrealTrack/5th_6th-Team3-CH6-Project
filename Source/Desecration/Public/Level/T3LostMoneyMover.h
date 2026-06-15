#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3LostMoneyMover.generated.h"

class UBoxComponent;

UCLASS()
class DESECRATION_API AT3LostMoneyMover : public AActor
{
	GENERATED_BODY()
	
public:	
	AT3LostMoneyMover();
	
	virtual void PostInitializeComponents() override;

protected:
	virtual void BeginPlay() override;

private:
	//액텨와 겹치면 실행
	UFUNCTION()
	void OnActorOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool FromSweep, const FHitResult& SweepResult);
	
	//루트 컴포넌트
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<USceneComponent> RootComp;
	
	//잃어버린 재화를 감지할 콜리전
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<UBoxComponent> CollisionBox;
	
	//잃어버린 재화를 옮길 위치
	UPROPERTY(EditInstanceOnly, Category = "Lost Money", meta = (AllowPrivateAccess = true))
	FVector MoveDestination;
	
	//여러 개를 옮길 경우 각 재화 액터의 거리 (0이면 재화 액터의 콜리전으로 계산)
	UPROPERTY(EditDefaultsOnly, Category = "Lost Money", meta = (AllowPrivateAccess = true))
	float DistanceBetweenActors;
	
	//여러 개를 옮길 경우 1줄당 존재 가능한 액터 개수
	int32 ActorCountPerLine;
	
	//옮긴 재화 액터 개수
	int32 MovedActorCount;
	
	//읽어버린 재화 태그
	const FName LOST_MONEY_TAG = TEXT("LostMoney");
};
