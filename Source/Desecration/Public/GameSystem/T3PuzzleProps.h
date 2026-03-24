// T3PuzzleProps.h
// 복도 퍼즐용 배치 액터 — BP로 상속하여 메시/머티리얼 설정

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3PuzzleProps.generated.h"

class UBoxComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

// ============================================================
// AT3PuzzleBarrier — 벽/안개 장벽 (통행 제한용)
// ============================================================

UCLASS()
class DESECRATION_API AT3PuzzleBarrier : public AActor
{
	GENERATED_BODY()

public:
	AT3PuzzleBarrier();

	// 루트 (스케일 독립)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier")
	TObjectPtr<USceneComponent> DefaultRoot;

	// 벽 비주얼 (BP에서 메시/머티리얼 설정 — 루트와 독립적으로 스케일 조절)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier")
	TObjectPtr<UStaticMeshComponent> BarrierMesh;

	// 통행 차단 콜리전 (메시보다 크게 설정 가능)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Barrier")
	TObjectPtr<UBoxComponent> BlockingCollision;

	// 콜리전 크기 (에디터 조절)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
	FVector BlockingExtent = FVector(50.f, 300.f, 200.f);

	// 장벽 활성화/비활성화 (퍼즐 완료 시 호출)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Barrier")
	void SetBarrierActive(bool bActive);
	virtual void SetBarrierActive_Implementation(bool bActive);

protected:
	virtual void BeginPlay() override;
};

// ============================================================
// AT3PuzzleLantern — 석등/진행도 표시 라이트
// ============================================================

UCLASS()
class DESECRATION_API AT3PuzzleLantern : public AActor
{
	GENERATED_BODY()

public:
	AT3PuzzleLantern();

	// 석등 메시 (BP에서 설정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lantern")
	TObjectPtr<UStaticMeshComponent> LanternMesh;

	// 라이트 (색상/강도 BP에서 설정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Lantern")
	TObjectPtr<UPointLightComponent> LanternLight;

	// 라이트 ON 시 강도 (BP 조절 — 컴포넌트 값 대신 이 값이 적용됨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern|Light")
	float LanternLightIntensity = 5000.f;

	// 라이트 ON 시 색상 (BP 조절 — 컴포넌트 값 대신 이 값이 적용됨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lantern|Light")
	FLinearColor LanternLightColor = FLinearColor(1.f, 0.7f, 0.3f);

	// 라이트 ON/OFF (퍼즐 스텝 완료 시 호출)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Lantern")
	void SetLanternLit(bool bLit);
	virtual void SetLanternLit_Implementation(bool bLit);

protected:
	virtual void BeginPlay() override;
};
