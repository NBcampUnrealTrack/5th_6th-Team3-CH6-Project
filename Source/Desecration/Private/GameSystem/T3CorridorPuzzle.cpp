// T3CorridorPuzzle.cpp

#include "GameSystem/T3CorridorPuzzle.h"
#include "GameSystem/T3PuzzleMonsterSpawner.h"
#include "GameSystem/T3PuzzleProps.h"
#include "Desecration.h"
#include "Components/BoxComponent.h"
#include "Components/LightComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

AT3CorridorPuzzle::AT3CorridorPuzzle()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트
	DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
	SetRootComponent(DefaultRoot);

	// Forward 트리거 (복도 전진 쪽 끝)
	ForwardTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("ForwardTrigger"));
	ForwardTrigger->SetupAttachment(DefaultRoot);
	ForwardTrigger->SetBoxExtent(FVector(100.f, 300.f, 200.f));
	ForwardTrigger->SetRelativeLocation(FVector(500.f, 0.f, 0.f));
	ForwardTrigger->SetGenerateOverlapEvents(true);
	ForwardTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ForwardTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ForwardTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ForwardTrigger->ShapeColor = FColor::Green;

	// Backward 트리거 (복도 후진 쪽 끝)
	BackwardTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("BackwardTrigger"));
	BackwardTrigger->SetupAttachment(DefaultRoot);
	BackwardTrigger->SetBoxExtent(FVector(100.f, 300.f, 200.f));
	BackwardTrigger->SetRelativeLocation(FVector(-500.f, 0.f, 0.f));
	BackwardTrigger->SetGenerateOverlapEvents(true);
	BackwardTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BackwardTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	BackwardTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BackwardTrigger->ShapeColor = FColor::Red;

	// 텔레포트 지점 (에디터에서 드래그 배치)
	EntrySpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EntrySpawnPoint"));
	EntrySpawnPoint->SetupAttachment(DefaultRoot);
	EntrySpawnPoint->SetRelativeLocation(FVector(200.f, 0.f, 0.f));

	ExitSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ExitSpawnPoint"));
	ExitSpawnPoint->SetupAttachment(DefaultRoot);
	ExitSpawnPoint->SetRelativeLocation(FVector(-200.f, 0.f, 0.f));
}

void AT3CorridorPuzzle::BeginPlay()
{
	Super::BeginPlay();

	// 트리거 바인딩
	if (ForwardTrigger)
	{
		ForwardTrigger->OnComponentBeginOverlap.AddDynamic(this, &AT3CorridorPuzzle::OnForwardTriggerOverlap);
	}
	if (BackwardTrigger)
	{
		BackwardTrigger->OnComponentBeginOverlap.AddDynamic(this, &AT3CorridorPuzzle::OnBackwardTriggerOverlap);
	}

	// 초기 상태: 모든 변경 액터 숨기기 + 라이트 OFF
	ClearAllStepChanges();
	UpdateLights();

	// 첫 단계 변경사항 적용 (Backward 단계라면 차이점 표시)
	if (PuzzleSteps.Num() > 0)
	{
		ApplyStepChanges(0, true);
	}

	// 초기 몬스터 스폰 (숨겨지지 않은 스포너만)
	for (AT3PuzzleMonsterSpawner* Spawner : MonsterSpawners)
	{
		if (IsValid(Spawner) && !Spawner->IsHidden())
		{
			Spawner->RespawnMonster();
		}
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: 시작 (총 %d단계)"), PuzzleSteps.Num());
}

// ============================================================
// 플레이어 이동 판정
// ============================================================

void AT3CorridorPuzzle::OnPlayerMoved(ECorridorDirection Direction)
{
	if (!bPuzzleActive || bPuzzleCompleted)
	{
		return;
	}

	if (!PuzzleSteps.IsValidIndex(CurrentStep))
	{
		UE_LOG(LogDesecration, Warning, TEXT("T3_CorridorPuzzle: 유효하지 않은 스텝 (%d)"), CurrentStep);
		return;
	}

	const FCorridorPuzzleStep& Step = PuzzleSteps[CurrentStep];

	// 판정 즉시 비활성화 (BP에서 시퀀스 완료 후 ActivatePuzzle로 재활성)
	bPuzzleActive = false;

	if (Direction == Step.CorrectDirection)
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: Step %d 정답 (%s)"),
			CurrentStep, Direction == ECorridorDirection::Forward ? TEXT("Forward") : TEXT("Backward"));

		OnPuzzleStepCompleted.Broadcast(CurrentStep);
		CurrentStep++;

		if (CurrentStep >= PuzzleSteps.Num())
		{
			// 모든 단계 완료
			CompletePuzzle();
		}
		// BP에서 FadeIn → RefreshStepVisuals → TeleportPlayerToEntry → FadeOut
	}
	else
	{
		UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: Step %d 오답 (%s) — 리셋"),
			CurrentStep, Direction == ECorridorDirection::Forward ? TEXT("Forward") : TEXT("Backward"));

		CurrentStep = 0;
		OnPuzzleReset.Broadcast();
		// BP에서 FadeIn → RefreshStepVisuals → TeleportPlayerToEntry → FadeOut
	}
}

// ============================================================
// 퍼즐 초기화
// ============================================================

void AT3CorridorPuzzle::ResetPuzzle()
{
	ClearAllStepChanges();
	CurrentStep = 0;

	// 첫 단계 변경사항 다시 적용
	if (PuzzleSteps.Num() > 0)
	{
		ApplyStepChanges(0, true);
	}

	UpdateLights();
	OnPuzzleReset.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: 리셋 완료"));
}

// ============================================================
// 플레이어 텔레포트 (BP에서 페이드 중 호출)
// ============================================================

void AT3CorridorPuzzle::TeleportPlayerToEntry()
{
	if (!EntrySpawnPoint)
	{
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Player)
	{
		return;
	}

	const FVector Loc = EntrySpawnPoint->GetComponentLocation();
	const FRotator Rot = EntrySpawnPoint->GetComponentRotation();
	Player->TeleportTo(Loc, Rot);

	// 카메라도 EntrySpawnPoint 방향으로 강제 회전
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->SetControlRotation(Rot);
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: 플레이어 텔레포트 → EntrySpawnPoint"));
}

void AT3CorridorPuzzle::ActivatePuzzle()
{
	bPuzzleActive = true;

	// 몬스터 리스폰은 BP에서 RefreshStepVisuals → ActivatePuzzle 순서로 처리
	UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: 퍼즐 활성화 — 트리거 판정 시작"));
}

// ============================================================
// 스텝 시각 변경 적용 (BP에서 페이드 중 호출)
// ============================================================

void AT3CorridorPuzzle::RefreshStepVisuals()
{
	// 모든 스포너 몬스터 먼저 제거 (Show/Hide 변경 전)
	for (AT3PuzzleMonsterSpawner* Spawner : MonsterSpawners)
	{
		if (IsValid(Spawner))
		{
			Spawner->DestroySpawnedMonster();
		}
	}

	// 모든 변경 해제 후 현재 스텝만 적용
	ClearAllStepChanges();

	if (!bPuzzleCompleted && PuzzleSteps.IsValidIndex(CurrentStep))
	{
		ApplyStepChanges(CurrentStep, true);
		OnStepActivated(CurrentStep);
	}

	UpdateLights();

	// Show/Hide 적용 후 — 숨겨지지 않은 스포너만 리스폰
	for (AT3PuzzleMonsterSpawner* Spawner : MonsterSpawners)
	{
		if (IsValid(Spawner) && !Spawner->IsHidden())
		{
			Spawner->RespawnMonster();
		}
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: 스텝 %d 시각 갱신 (몬스터 리스폰)"), CurrentStep);
}

// ============================================================
// 배리어 파괴 (BP에서 페이드 완료 후 호출)
// ============================================================

void AT3CorridorPuzzle::DestroyBarriers()
{
	for (AActor* Barrier : BarrierActors)
	{
		if (IsValid(Barrier))
		{
			// Wind/Foliage 렌더 크래시 방지: Visibility 먼저 해제 → 다음 틱에 파괴
			Barrier->SetActorHiddenInGame(true);
			Barrier->SetActorEnableCollision(false);
			if (Barrier->GetRootComponent())
			{
				Barrier->GetRootComponent()->SetVisibility(false, true);
			}
			TWeakObjectPtr<AActor> WeakBarrier = Barrier;
			GetWorld()->GetTimerManager().SetTimerForNextTick([WeakBarrier]()
			{
				if (AActor* B = WeakBarrier.Get())
				{
					B->Destroy();
				}
			});
		}
	}

	UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: 장막(배리어) 제거"));
}

// ============================================================
// 퍼즐 완료
// ============================================================

void AT3CorridorPuzzle::CompletePuzzle()
{
	bPuzzleCompleted = true;

	// 모든 라이트 ON
	UpdateLights();

	// 모든 스포너 몬스터 제거 (이상현상 정리 전)
	for (AT3PuzzleMonsterSpawner* Spawner : MonsterSpawners)
	{
		if (IsValid(Spawner))
		{
			Spawner->DestroySpawnedMonster();
		}
	}

	// 이상현상 액터 정리 (ActorsToShow 스포너 Destroy 포함)
	for (const FCorridorPuzzleStep& Step : PuzzleSteps)
	{
		// 이상현상으로 추가된 액터 → 렌더 스레드 안전 파괴
		for (AActor* Actor : Step.ActorsToShow)
		{
			if (IsValid(Actor))
			{
				// Wind/Foliage 렌더 크래시 방지: Visibility 먼저 해제 → 다음 틱에 파괴
				Actor->SetActorHiddenInGame(true);
				Actor->SetActorEnableCollision(false);
				if (Actor->GetRootComponent())
				{
					Actor->GetRootComponent()->SetVisibility(false, true);
				}
				TWeakObjectPtr<AActor> WeakActor = Actor;
				GetWorld()->GetTimerManager().SetTimerForNextTick([WeakActor]()
				{
					if (AActor* A = WeakActor.Get())
					{
						A->Destroy();
					}
				});
			}
		}
		// 이상현상으로 숨겨진 원본 → 복원
		for (AActor* Actor : Step.ActorsToHide)
		{
			if (IsValid(Actor))
			{
				Actor->SetActorHiddenInGame(false);
				Actor->SetActorEnableCollision(true);
			}
		}
	}

	// 이상현상 정리 후 — 살아남은(기본) 스포너만 리스폰
	for (AT3PuzzleMonsterSpawner* Spawner : MonsterSpawners)
	{
		if (IsValid(Spawner) && !Spawner->IsHidden())
		{
			Spawner->RespawnMonster();
		}
	}

	// 출구 위치 캐시 (BP에서 페이드 후 텔레포트용)
	const FVector ExitLoc = ExitSpawnPoint ? ExitSpawnPoint->GetComponentLocation() : FVector::ZeroVector;
	const FRotator ExitRot = ExitSpawnPoint ? ExitSpawnPoint->GetComponentRotation() : FRotator::ZeroRotator;

	// 델리게이트 발동 (BP에서 페이드 → 텔레포트 → DestroyBarriers 호출)
	OnPuzzleCompleted.Broadcast(ExitLoc, ExitRot);

	UE_LOG(LogDesecration, Log, TEXT("T3_CorridorPuzzle: 퍼즐 완료 — 이상현상 파괴"));
}

// ============================================================
// 라이트 업데이트
// ============================================================

void AT3CorridorPuzzle::UpdateLights()
{
	for (int32 i = 0; i < ProgressLightActors.Num(); ++i)
	{
		AActor* LightActor = ProgressLightActors[i];
		if (IsValid(LightActor))
		{
			SetLightState(LightActor, i < CurrentStep);
		}
	}
}

void AT3CorridorPuzzle::SetLightState_Implementation(AActor* LightActor, bool bOn)
{
	if (!IsValid(LightActor))
	{
		return;
	}

	// AT3PuzzleLantern이면 전용 함수 호출
	if (AT3PuzzleLantern* Lantern = Cast<AT3PuzzleLantern>(LightActor))
	{
		Lantern->SetLanternLit(bOn);
		return;
	}

	// ULightComponent 찾아서 토글
	if (ULightComponent* Light = LightActor->FindComponentByClass<ULightComponent>())
	{
		Light->SetVisibility(bOn);
		return;
	}

	// LightComponent 없으면 액터 전체 숨김/표시
	LightActor->SetActorHiddenInGame(!bOn);
}

// ============================================================
// 스텝 변경 적용 (BP 오버라이드 가능)
// ============================================================

void AT3CorridorPuzzle::OnStepActivated_Implementation(int32 StepIndex)
{
	// 기본 구현: 로그만 출력 (BP에서 커스텀 연출 추가 가능)
	UE_LOG(LogDesecration, Verbose, TEXT("T3_CorridorPuzzle: Step %d 활성화"), StepIndex);
}

// ============================================================
// Show/Hide 액터 적용/해제
// ============================================================

void AT3CorridorPuzzle::ApplyStepChanges(int32 StepIndex, bool bApply)
{
	if (!PuzzleSteps.IsValidIndex(StepIndex))
	{
		return;
	}

	const FCorridorPuzzleStep& Step = PuzzleSteps[StepIndex];

	// ActorsToShow: bApply=true → 보여줌, bApply=false → 숨김
	for (AActor* Actor : Step.ActorsToShow)
	{
		if (IsValid(Actor))
		{
			Actor->SetActorHiddenInGame(!bApply);
			Actor->SetActorEnableCollision(bApply);
			// Wind/Foliage 렌더 크래시 방지: 렌더 스레드에서 완전히 제거
			Actor->GetRootComponent()->SetVisibility(bApply, true);
		}
	}

	// ActorsToHide: bApply=true → 숨김, bApply=false → 보여줌 (원복)
	for (AActor* Actor : Step.ActorsToHide)
	{
		if (IsValid(Actor))
		{
			Actor->SetActorHiddenInGame(bApply);
			Actor->SetActorEnableCollision(!bApply);
			// Wind/Foliage 렌더 크래시 방지: 렌더 스레드에서 완전히 제거
			Actor->GetRootComponent()->SetVisibility(!bApply, true);
		}
	}
}

void AT3CorridorPuzzle::ClearAllStepChanges()
{
	for (int32 i = 0; i < PuzzleSteps.Num(); ++i)
	{
		ApplyStepChanges(i, false);
	}
}

// ============================================================
// 트리거 콜백
// ============================================================

void AT3CorridorPuzzle::OnForwardTriggerOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (const ACharacter* PlayerChar = Cast<ACharacter>(OtherActor))
	{
		if (PlayerChar->IsPlayerControlled())
		{
			OnPlayerMoved(ECorridorDirection::Forward);
		}
	}
}

void AT3CorridorPuzzle::OnBackwardTriggerOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (const ACharacter* PlayerChar = Cast<ACharacter>(OtherActor))
	{
		if (PlayerChar->IsPlayerControlled())
		{
			OnPlayerMoved(ECorridorDirection::Backward);
		}
	}
}
