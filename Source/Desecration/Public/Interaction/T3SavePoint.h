#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3SavePoint.generated.h"

enum class ELevelName : uint8;
class UBoxComponent;
class AT3GameMode;

UCLASS()
class DESECRATION_API AT3SavePoint : public AActor
{
	GENERATED_BODY()
	
public:	
	AT3SavePoint();

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	
private:
	//액텨와 겹치면 실행
	UFUNCTION()
	void OnActorOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool FromSweep, const FHitResult& SweepResult);
	
	//루트 컴포넌트
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<USceneComponent> RootComp;
	
	//메시
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<UStaticMeshComponent> SavePointMesh;
	
	//콜리전 역할을 하는 박스
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
	TObjectPtr<UBoxComponent> CollisionBox;
	
	//여기가 어디오?
	UPROPERTY(EditInstanceOnly, Category = "Save Game Data", meta = (AllowPrivateAccess = true))
	ELevelName LevelName;
	
	//게임 모드
	UPROPERTY()
	TObjectPtr<AT3GameMode> T3GameMode;
	
	//플레이어 태그
	const FName PLAYER_TAG = TEXT("Player");
};
