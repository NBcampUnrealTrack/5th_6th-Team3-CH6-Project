#include "Equipment/Accessory/T3BalrogAccessoryEffect.h"

#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Monster/Interface/T3Monster.h"
#include "Player/T3CharacterBase.h"

void UT3BalrogAccessoryEffect::OnEquipped_Implementation(AT3CharacterBase* OwnerChar)
{
	CachedOwner = OwnerChar;

	PlayEquipEffect(OwnerChar);

	OwnerChar->GetWorldTimerManager().SetTimer(
		DamageTimerHandle,
		this,
		&UT3BalrogAccessoryEffect::PerformDamage,
		DamageInterval,
		true);
}

void UT3BalrogAccessoryEffect::PerformDamage()
{
	if (!CachedOwner.IsValid())
	{
		return;
	}
	
	AT3CharacterBase* Owner = CachedOwner.Get();

	TArray<AActor*> Overlapped;
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
	
	ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		Owner,
		Owner->GetActorLocation(),
		DamageRadius,
		ObjTypes,
		nullptr,
		{Owner},
		Overlapped);

	if (bShowDebugRadius)
	{
		DrawDebugSphere(Owner->GetWorld(), Owner->GetActorLocation(), DamageRadius, 16, FColor::Red, false, DamageInterval);
	}

	for (AActor* Hit : Overlapped)
	{
		if (IT3Monster* Monster = Cast<IT3Monster>(Hit))
		{
			Monster->ApplyBonusDamage(DamageAmount);
		}
	}
}

void UT3BalrogAccessoryEffect::OnUnequipped_Implementation(AT3CharacterBase* OwnerChar)
{
	OwnerChar->GetWorldTimerManager().ClearTimer(DamageTimerHandle);

	StopEquipEffect();

	CachedOwner = nullptr;
}