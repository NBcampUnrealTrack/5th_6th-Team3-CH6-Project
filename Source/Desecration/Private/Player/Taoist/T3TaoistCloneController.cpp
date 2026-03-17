// T3TaoistCloneController.cpp


#include "Player/Taoist/T3TaoistCloneController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Player/Taoist/T3TaoistClone.h"

void AT3TaoistCloneController::UpdateTargetTracking(AActor* Target)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn || !Target) return;

    // 현재 시간: 2026-03-13 15:32
    float DistanceToTarget = ControlledPawn->GetDistanceTo(Target);
    SetFocus(Target, EAIFocusPriority::Gameplay);

    // 1. 인덱스 확보
    int32 Index = 0;
    if (AT3TaoistClone* ClonePawn = Cast<AT3TaoistClone>(ControlledPawn))
    {
        Index = ClonePawn->GetCloneIndex();
    }

    // 2. 사거리 설정 (타겟이 플레이어냐 아니냐에 따라 유연하게)
    float AttackRadius = Target->IsA(AT3CharacterBase::StaticClass()) ? 500.f : 700.f;

    // [핵심] 타겟과 본체(또는 첫 번째 분신) 사이의 벡터를 기준으로 좌우로 벌림
    // 단순 Direction 대신, 타겟 뒤쪽에서 본체를 바라보는 고정 벡터를 활용하는 게 좋음
    FVector ForwardDir = (ControlledPawn->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();

    // 3. 부채꼴 각도 계산
    // Index 0: 0도, Index 1: 45도, Index 2: -45도, Index 3: 90도... 이런 식으로 전개
    float BaseAngle = 45.f;
    float FinalAngle = 0.f;
    if (Index > 0)
    {
        FinalAngle = (Index % 2 == 1) ? (BaseAngle * ((Index + 1) / 2)) : (-BaseAngle * (Index / 2));
    }

    // 벡터 회전: 타겟을 중심으로 분신들이 서 있을 방향 결정
    FVector SpacedDirection = ForwardDir.RotateAngleAxis(FinalAngle, FVector::UpVector);

    // 4. 목표 지점 설정: 타겟에서 AttackRadius만큼 떨어진 위치
    // 사진처럼 겹치지 않으려면 이 Radius가 충분히 확보되어야 함
    FVector GoalLocation = Target->GetActorLocation() + (SpacedDirection * AttackRadius);

    // 5. 이동 및 정지 로직
    // 현재 위치가 목표 지점 근처(AcceptanceRadius)라면 굳이 더 움직이지 않음
    if (DistanceToTarget <= AttackRadius + 50.f && DistanceToTarget >= AttackRadius - 50.f)
    {
        StopMovement();
        return;
    }

    FAIMoveRequest MoveRequest;
    MoveRequest.SetGoalLocation(GoalLocation);
    MoveRequest.SetAcceptanceRadius(30.f); // 정밀하게 이동

    MoveTo(MoveRequest);
}
