#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3LostMoney.generated.h"

class USphereComponent;
class AT3GameMode;

UCLASS()
class DESECRATION_API AT3LostMoney : public AActor
{
	GENERATED_BODY()
	
public:	
	AT3LostMoney();

	virtual void PostInitializeComponents() override;
	
protected:
	virtual void BeginPlay() override;
	
public:
	void SetLostMoneyID(const int32 ID);
	void SetMoney(const int32 Amount);
	
	//콜리전 역할을 하는 구체
	FORCEINLINE TObjectPtr<USphereComponent> GetCollisionSphere() const { return CollisionSphere; }

private:
	//액텨와 겹치면 실행
	UFUNCTION()
	void OnActorOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool FromSweep, const FHitResult& SweepResult);
	
	//루트 컴포넌트
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<USceneComponent> RootComp;
	
	//콜리전 역할을 하는 구체
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<USphereComponent> CollisionSphere;
	
	//게임 모드
	UPROPERTY()
	TSoftObjectPtr<AT3GameMode> T3GameMode;
	
	//잃어버린 돈에 대한 ID
	UPROPERTY(VisibleInstanceOnly, Category = "Lost Money")
	int32 LostMoneyID;
	
	//잃어버린 재화량
	UPROPERTY(VisibleInstanceOnly, Category = "Lost Money")
	int32 Money;

	//플레이어 태그
	const FName PLAYER_TAG = TEXT("PlayerCharacter");
};
