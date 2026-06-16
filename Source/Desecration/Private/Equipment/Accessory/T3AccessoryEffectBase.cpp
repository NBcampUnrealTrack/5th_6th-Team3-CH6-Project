#include "Equipment/Accessory/T3AccessoryEffectBase.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

void UT3AccessoryEffectBase::OnEquipped_Implementation(AT3CharacterBase* OwnerChar)
{
}

void UT3AccessoryEffectBase::OnUnequipped_Implementation(AT3CharacterBase* OwnerChar)
{
}

UNiagaraComponent* UT3AccessoryEffectBase::PlayEquipEffect(AActor* Target, FName SocketName, bool bAutoDestroy)
{
	if (!EquipEffect || !IsValid(Target))
	{
		return nullptr;
	}

	USkeletalMeshComponent* Mesh = Target->FindComponentByClass<USkeletalMeshComponent>();

	if (!IsValid(Mesh))
	{
		return nullptr;
	}

	ActiveEquipEffect = UNiagaraFunctionLibrary::SpawnSystemAttached(
		EquipEffect,
		Mesh,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		bAutoDestroy
	);

	return ActiveEquipEffect;
}

void UT3AccessoryEffectBase::StopEquipEffect()
{
	if (IsValid(ActiveEquipEffect))
	{
		ActiveEquipEffect->DeactivateImmediate();

		ActiveEquipEffect = nullptr;
	}
}
