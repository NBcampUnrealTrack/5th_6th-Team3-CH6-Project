// T3FanWeapon.h


#include "Player/Taoist/T3FanWeapon.h"
#include "Animation/AnimInstance.h"

AT3FanWeapon::AT3FanWeapon()
{
    CurrentState = EFanState::Closed;
}

void AT3FanWeapon::SetFanState(EFanState NewState)
{
    if (CurrentState == NewState) return;

    CurrentState = NewState;
    OnFanStateChanged.Broadcast(NewState);

    UE_LOG(LogTemp, Log, TEXT("Fan State Changed to: %d"), (int32)NewState);
}

void AT3FanWeapon::OpenFan()
{
    // if (CurrentState == EFanState::Opened || CurrentState == EFanState::Opening) return;

    // 부모의 Getter를 사용해 스켈레탈 메시 컴포넌트 확보
    USkeletalMeshComponent* MeshComp = GetWeaponSkeletalMesh();

    if (MeshComp && FanOpenMontage)
    {

        MeshComp->PlayAnimation(FanOpenMontage, false);
        // SetFanState(EFanState::Opening);

    }
}

void AT3FanWeapon::CloseFan()
{
    // if (CurrentState == EFanState::Closed || CurrentState == EFanState::Closing) return;

    USkeletalMeshComponent* MeshComp = GetWeaponSkeletalMesh();

    if (MeshComp && FanCloseMontage)
    {

        MeshComp->PlayAnimation(FanCloseMontage, false);
       // SetFanState(EFanState::Opening);

    }


}
