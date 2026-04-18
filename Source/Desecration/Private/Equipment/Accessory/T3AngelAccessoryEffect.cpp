#include "Equipment/Accessory/T3AngelAccessoryEffect.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Monster/T3MonsterBase.h"
#include "Player/T3CharacterBase.h"

void UT3AngelAccessoryEffect::OnEquipped_Implementation(AT3CharacterBase* OwnerChar)
{
	CachedOwner = OwnerChar;
	
	OwnerChar->GetWorldTimerManager().SetTimer(
		SlowTimerHandle,
		this,
		&UT3AngelAccessoryEffect::ApplySlowAura,
		SlowInterval,
		true);
}

void UT3AngelAccessoryEffect::ApplySlowAura()
{
	if (!CachedOwner.IsValid())
	{
		return;
	}

	for (int32 i = 0; i < SlowedMonsters.Num(); ++i)
	{
		if (SlowedMonsters[i].IsValid())
		{
			SlowedMonsters[i]->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeeds[i];
		}
	}
	
	SlowedMonsters.Empty();
	
	OriginalSpeeds.Empty();

	AT3CharacterBase* Owner = CachedOwner.Get();
	
	TArray<AActor*> Overlapped;

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjTypes;
	
	ObjTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		Owner,
		Owner->GetActorLocation(),
		SlowRadius,
		ObjTypes,
		nullptr,
		{Owner},
		Overlapped);
	
	for (AActor* Actor : Overlapped)
	{
		if (AT3MonsterBase* Monster = Cast<AT3MonsterBase>(Actor))
		{
			float OriginSpeed = Monster->GetCharacterMovement()->MaxWalkSpeed;
			
			Monster->GetCharacterMovement()->MaxWalkSpeed *= SlowAmount;
			
			SlowedMonsters.Add(Monster);
			
			OriginalSpeeds.Add(OriginSpeed);
		}
	}
}

void UT3AngelAccessoryEffect::OnUnequipped_Implementation(AT3CharacterBase* OwnerChar)
{
	OwnerChar->GetWorldTimerManager().ClearTimer(SlowTimerHandle);
	// 슬로우 중인 몬스터 전체 속도 복구
	for (int32 i = 0; i < SlowedMonsters.Num(); ++i)
	{
		if (SlowedMonsters[i].IsValid())
		{
			SlowedMonsters[i]->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeeds[i];
		}
	}
	
	SlowedMonsters.Empty();
	
	OriginalSpeeds.Empty();
}