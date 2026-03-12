#include "Monster/T3ANS_WeaponSocketSwitch.h"
#include "Monster/T3BossWeaponComponent.h"

FString UT3ANS_WeaponSocketSwitch::GetNotifyName_Implementation() const
{
	return TEXT("WeaponSocket: Alternative");
}

void UT3ANS_WeaponSocketSwitch::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) { return; }
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) { return; }

	if (UT3BossWeaponComponent* WeaponComp = Owner->FindComponentByClass<UT3BossWeaponComponent>())
	{
		WeaponComp->SwitchToSocket(EWeaponSocketType::Alternative);
	}
}

void UT3ANS_WeaponSocketSwitch::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) { return; }
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) { return; }

	if (UT3BossWeaponComponent* WeaponComp = Owner->FindComponentByClass<UT3BossWeaponComponent>())
	{
		WeaponComp->ResetToDefaultSocket();
	}
}
