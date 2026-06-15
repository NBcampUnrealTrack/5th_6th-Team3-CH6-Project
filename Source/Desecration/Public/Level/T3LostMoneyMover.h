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

protected:
	virtual void BeginPlay() override;

private:
	//재화 옮기기
	void MoveLostMoneyActors();
	
	//루트 컴포넌트
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<USceneComponent> RootComp;
	
	//잃어버린 재화를 감지할 콜리전
	UPROPERTY(VisibleInstanceOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<UBoxComponent> CollisionBox;
	
	//잃어버린 재화를 이 컴포넌트로 옮김
	UPROPERTY(VisibleInstanceOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<USceneComponent> DestinationComponent;
	
	//여러 개를 옮길 경우 각 재화 액터의 거리 (0이면 재화 액터의 콜리전으로 계산)
	UPROPERTY(EditDefaultsOnly, Category = "Lost Money", meta = (AllowPrivateAccess = true))
	float DistanceBetweenActors;
	
	//잃어버린 재화 옮기기 작업 예약용 핸들러
	FTimerHandle MoveLostMoneyTimerHandle;
	
	//여러 개를 옮길 경우 1줄당 존재 가능한 액터 개수
	int32 ActorCountPerLine;
	
	//옮긴 재화 액터 개수
	int32 MovedActorCount;
	
	//읽어버린 재화 태그
	const FName LOST_MONEY_TAG = TEXT("LostMoney");
};
