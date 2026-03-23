// T3CorridorPuzzle.h
// 8번출구 스타일 복도 퍼즐 — 정해진 시퀀스대로 전진/후진, 틀리면 초기화

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3CorridorPuzzle.generated.h"

class UBoxComponent;
class USceneComponent;
class AT3PuzzleMonsterSpawner;

// ============================================================
// Enum: 이동 방향
// ============================================================

UENUM(BlueprintType)
enum class ECorridorDirection : uint8
{
	Forward		UMETA(DisplayName = "Forward"),
	Backward	UMETA(DisplayName = "Backward")
};

// ============================================================
// Struct: 퍼즐 단계 데이터
// ============================================================

USTRUCT(BlueprintType)
struct FCorridorPuzzleStep
{
	GENERATED_BODY()

	// 이 단계의 정답 방향
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
	ECorridorDirection CorrectDirection = ECorridorDirection::Forward;

	// 이 단계 진입 시 보여줄 액터 (차이점 추가 — 없던 물건 등장, 교체 시 변형 버전)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
	TArray<TObjectPtr<AActor>> ActorsToShow;

	// 이 단계 진입 시 숨길 액터 (차이점 제거 — 있던 물건 사라짐, 교체 시 원본)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
	TArray<TObjectPtr<AActor>> ActorsToHide;
};

// ============================================================
// 델리게이트
// ============================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCorridorPuzzleCompleted, FVector, ExitLocation, FRotator, ExitRotation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCorridorPuzzleReset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCorridorPuzzleStepCompleted, int32, Step);

// ============================================================
// AT3CorridorPuzzle
// ============================================================

UCLASS()
class DESECRATION_API AT3CorridorPuzzle : public AActor
{
	GENERATED_BODY()

public:
	AT3CorridorPuzzle();

protected:
	virtual void BeginPlay() override;

public:

	// ============================================================
	// 트리거 (에디터에서 복도 양 끝에 드래그 배치)
	// ============================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Trigger")
	TObjectPtr<USceneComponent> DefaultRoot;

	// 전진 방향 트리거 (복도 Forward 쪽 끝)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Trigger")
	TObjectPtr<UBoxComponent> ForwardTrigger;

	// 후진 방향 트리거 (복도 Backward 쪽 끝)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Trigger")
	TObjectPtr<UBoxComponent> BackwardTrigger;

	// 트리거 크기 (에디터 조절)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Trigger")
	FVector TriggerExtent = FVector(100.f, 300.f, 200.f);

	// ============================================================
	// 퍼즐 설정
	// ============================================================

	// 퍼즐 단계 시퀀스 (에디터에서 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle")
	TArray<FCorridorPuzzleStep> PuzzleSteps;

	// 진행도 표시 라이트 액터 (순서대로 연결 — 스텝 완료 시 불 켜짐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Light")
	TArray<TObjectPtr<AActor>> ProgressLightActors;

	// 통행 제한 액터 (완료 시 파괴 — 레벨에서 스포이드로 연결)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Barrier")
	TArray<TObjectPtr<AActor>> BarrierActors;

	// 몬스터 스포너 (스텝 전환 시 자동 리스폰 — 에디터에서 스포이드로 연결)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Puzzle|Monster")
	TArray<TObjectPtr<AT3PuzzleMonsterSpawner>> MonsterSpawners;

	// ============================================================
	// 텔레포트 지점 (BP FogGate에서 참조 — 에디터에서 드래그 배치)
	// ============================================================

	// 퍼즐 진입 후 스폰될 위치 (입구 장막 너머 복도 안쪽)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Teleport")
	TObjectPtr<USceneComponent> EntrySpawnPoint;

	// 퍼즐 완료 후 스폰될 위치 (출구 장막 너머)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle|Teleport")
	TObjectPtr<USceneComponent> ExitSpawnPoint;

	// ============================================================
	// 상태
	// ============================================================

	// 현재 진행 단계
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
	int32 CurrentStep = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
	bool bPuzzleCompleted = false;

	// 퍼즐 활성 여부 (false면 트리거 무시 — RoomLock 입장 완료 후 활성화)
	UPROPERTY(BlueprintReadOnly, Category = "Puzzle")
	bool bPuzzleActive = false;


	// ============================================================
	// 인터페이스 (BP/트리거에서 호출)
	// ============================================================

	// 플레이어 이동 판정 — 복도 끝 트리거에서 호출
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void OnPlayerMoved(ECorridorDirection Direction);

	// 퍼즐 수동 초기화
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void ResetPuzzle();

	// 플레이어를 EntrySpawnPoint로 텔레포트 (BP에서 페이드 중 호출)
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void TeleportPlayerToEntry();

	// 퍼즐 활성화 (RoomLock 입장 완료 후 호출 — 이후 트리거 판정 시작)
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void ActivatePuzzle();

	// 스텝 시각 변경 적용 (BP에서 페이드 중 호출 — 모든 변경 해제 후 현재 스텝만 적용)
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void RefreshStepVisuals();

	// 배리어 파괴 (BP에서 페이드 완료 후 호출)
	UFUNCTION(BlueprintCallable, Category = "Puzzle")
	void DestroyBarriers();

	// 라이트 상태 변경 (BP 오버라이드 가능 — 머티리얼 변경 등 커스텀)
	UFUNCTION(BlueprintNativeEvent, Category = "Puzzle")
	void SetLightState(AActor* LightActor, bool bOn);
	virtual void SetLightState_Implementation(AActor* LightActor, bool bOn);

	// 스텝 변경 적용 (BP 오버라이드 가능 — 커스텀 연출)
	UFUNCTION(BlueprintNativeEvent, Category = "Puzzle")
	void OnStepActivated(int32 StepIndex);
	virtual void OnStepActivated_Implementation(int32 StepIndex);

	// ============================================================
	// 델리게이트
	// ============================================================

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnCorridorPuzzleCompleted OnPuzzleCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnCorridorPuzzleReset OnPuzzleReset;

	UPROPERTY(BlueprintAssignable, Category = "Puzzle|Events")
	FOnCorridorPuzzleStepCompleted OnPuzzleStepCompleted;

private:
	void CompletePuzzle();
	void UpdateLights();

	// 특정 스텝의 Show/Hide 액터 적용/해제
	void ApplyStepChanges(int32 StepIndex, bool bApply);

	// 모든 스텝의 변경사항 해제 (초기 상태 복원)
	void ClearAllStepChanges();


	// 트리거 콜백
	UFUNCTION()
	void OnForwardTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBackwardTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
