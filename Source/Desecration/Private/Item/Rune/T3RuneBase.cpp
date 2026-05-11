#include "Item/Rune/T3RuneBase.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

void UT3RuneBase::OnSocketed_Implementation(AT3CharacterBase* OwnerChar)
{
}

void UT3RuneBase::OnUnsocketed_Implementation(AT3CharacterBase* OwnerChar)
{
}

void UT3RuneBase::SetGrade(ET3RuneGrade InGrade)
{
}

bool UT3RuneBase::CanUnsocket() const
{
	return true;
}

float UT3RuneBase::GetCooldownRemaining() const
{
	return 0.0f;
}

void UT3RuneBase::RestoreCooldown(float RemainingTime)
{
}

UNiagaraComponent* UT3RuneBase::PlayTriggerEffect(AActor* Target, FName SocketName, bool bAutoDestroy)
{
    if (!TriggerEffect || !IsValid(Target))
    {
        return nullptr;
    }

    USkeletalMeshComponent* Mesh = Target->FindComponentByClass<USkeletalMeshComponent>();

    if (!IsValid(Mesh))
    {
        return nullptr;
    }

    return UNiagaraFunctionLibrary::SpawnSystemAttached(
        TriggerEffect,
        Mesh,
        SocketName,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget,
        bAutoDestroy
    );
}
