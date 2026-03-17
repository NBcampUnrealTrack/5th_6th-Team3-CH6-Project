#include "Monster/T3ANS_WeaponAura.h"
#include "Monster/T3BossWeaponComponent.h"
#include "NiagaraSystem.h"
#include "Desecration.h"

FString UT3ANS_WeaponAura::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("WeaponAura: %s"),
		AuraSystem ? *AuraSystem->GetName() : TEXT("Default"));
}

void UT3ANS_WeaponAura::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		if (UT3BossWeaponComponent* WeaponComp = Owner->FindComponentByClass<UT3BossWeaponComponent>())
		{
			WeaponComp->ActivateWeaponAura(AuraSystem);
		}
	}
}

void UT3ANS_WeaponAura::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		if (UT3BossWeaponComponent* WeaponComp = Owner->FindComponentByClass<UT3BossWeaponComponent>())
		{
			WeaponComp->DeactivateWeaponAura();
		}
	}
}
