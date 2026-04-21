#include "Equipment/Accessory/T3OniAccessoryEffect.h"

#include "Equipment/T3PlayerEquipmentComponent.h"
#include "Player/T3CharacterBase.h"

void UT3OniAccessoryEffect::OnEquipped_Implementation(AT3CharacterBase* OwnerChar)
{
	OwnerChar->EquipComp->SetOniAccessoryEquipped(true);
}

void UT3OniAccessoryEffect::OnUnequipped_Implementation(AT3CharacterBase* OwnerChar)
{
	OwnerChar->EquipComp->SetOniAccessoryEquipped(false);
}