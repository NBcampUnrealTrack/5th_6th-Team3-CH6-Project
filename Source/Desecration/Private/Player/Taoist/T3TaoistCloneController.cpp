// T3TaoistCloneController.cpp


#include "Player/Taoist/T3TaoistCloneController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/Taoist/T3TaoistClone.h"

void AT3TaoistCloneController::UpdateTargetTracking(AActor* Target)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !Target) return;

    float DistanceToTarget = ControlledPawn->GetDistanceTo(Target);

    // 1. 시선 고정 (매 업데이트마다 최우선 실행)
    // EAIFocusPriority::Gameplay를 사용하여 다른 회전 명령보다 우선권을 갖게 합니다.
    SetFocus(Target, EAIFocusPriority::Gameplay);

    // 2. 사거리 내 도착 시 로직
    if (DistanceToTarget <= 500.f)
    {
        StopMovement();
        return; // SetFocus가 이미 위에서 설정됨
    }

    // 3. 이동 로직 (기존과 동일)
    FVector Direction = (ControlledPawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
    int32 Index = 0;
    if (AT3TaoistClone* ClonePawn = Cast<AT3TaoistClone>(ControlledPawn))
    {
        Index = ClonePawn->GetCloneIndex();
    }

    float OffsetAngle = (Index == 0) ? 30.f : -30.f;
    FVector SpacedDirection = Direction.RotateAngleAxis(OffsetAngle, FVector::UpVector);
    FVector GoalLocation = Target->GetActorLocation() + (SpacedDirection * 180.f);

    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(GoalLocation);
    MoveRequest.SetAcceptanceRadius(30.f);

    MoveTo(MoveRequest);
}
