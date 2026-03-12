// T3TaoistCloneController.cpp


#include "Player/Taoist/T3TaoistCloneController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/Taoist/T3TaoistClone.h"

void AT3TaoistCloneController::UpdateTargetTracking(AActor* Target)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !Target) return;

    float DistanceToTarget = ControlledPawn->GetDistanceTo(Target);
    SetFocus(Target, EAIFocusPriority::Gameplay);

    // 2. 사거리 내 도착 시 로직
   
    if (Target->IsA(AT3CharacterBase::StaticClass()))
    {
        if (DistanceToTarget <= 500.f)
        {
            StopMovement();
            return;
        }
    }
    
    else if (DistanceToTarget <= 1200.f)
    {
        StopMovement();
        return;
    }

    // 3. 이동 로직: 분신들이 서로 겹치지 않게 '부채꼴'로 벌어지는 로직
    FVector Direction = (ControlledPawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();

    int32 Index = 0;
    if (AT3TaoistClone* ClonePawn = Cast<AT3TaoistClone>(ControlledPawn))
    {
        Index = ClonePawn->GetCloneIndex();
    }

    // 
    float OffsetAngle = (Index == 0) ? 80.f : -80.f;
    FVector SpacedDirection = Direction.RotateAngleAxis(OffsetAngle, FVector::UpVector);

    // Target 주변 180 유닛 거리에 배치
    float SafeDistance = 300.f;
    FVector GoalLocation = Target->GetActorLocation() + (SpacedDirection * SafeDistance);

    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(GoalLocation);
    MoveRequest.SetAcceptanceRadius(50.f); 

    MoveTo(MoveRequest);
}
