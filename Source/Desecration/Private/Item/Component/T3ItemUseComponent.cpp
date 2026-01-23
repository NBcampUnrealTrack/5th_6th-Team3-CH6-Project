#include "Item/Component/T3ItemUseComponent.h"
#include "Item/Data/T3ConsumableItemData.h"
#include "Player/T3CharacterBase.h"

UT3ItemUseComponent::UT3ItemUseComponent()
: PendingPowerValue(0.f),
PendingDefenseValue(0.f),
PendingSpeedValue(0.f),
PendingBerserkPowerValue(0.f),
PendingBerserkDefenseValue(0.f),
bIsPowerPotionActive(false),
bIsDefensePotionActive(false),
bIsSpeedPotionActive(false),
bIsBerserkPotionActive(false)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UT3ItemUseComponent::BeginPlay()
{
	Super::BeginPlay();
	
	OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());
}

void UT3ItemUseComponent::EndPowerPotionEffect()
{
	OwnerCharacter->SetAttackPower(OwnerCharacter->GetAttackPower() / PendingPowerValue);
	PendingPowerValue = 0.f;
	UE_LOG(LogTemp, Error, TEXT("공격력 포션 종료. 현재 공격력: %f"), OwnerCharacter->GetAttackPower());
}

void UT3ItemUseComponent::EndDefensePotionEffect()
{
	OwnerCharacter->SetDefense(OwnerCharacter->GetDefense() / PendingDefenseValue);
	PendingDefenseValue = 0.f;
	UE_LOG(LogTemp, Error, TEXT("방어력 포션 종료. 현재 방어력: %f"), OwnerCharacter->GetDefense());
}

void UT3ItemUseComponent::EndSpeedPotionEffect()
{
	// Speed 게터, 세터 요청
}

void UT3ItemUseComponent::EndBerserkPotionEffect()
{
	OwnerCharacter->SetAttackPower(OwnerCharacter->GetAttackPower() / PendingBerserkPowerValue);
	OwnerCharacter->SetDefense(OwnerCharacter->GetDefense() * PendingBerserkDefenseValue);
	
	PendingBerserkPowerValue = 0.f;
	PendingBerserkDefenseValue = 0.f;
	UE_LOG(LogTemp, Error, TEXT("광전사 포션 종료. 현재 공격력: %f, 방어력: %f"), OwnerCharacter->GetAttackPower(), OwnerCharacter->GetDefense());
}

void UT3ItemUseComponent::EndPowerPotionCoolTime()
{
	bIsPowerPotionActive = false;
	
	UE_LOG(LogTemp, Error, TEXT("공격력 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndDefensePotionCoolTime()
{
	bIsDefensePotionActive = false;
	
	UE_LOG(LogTemp, Error, TEXT("방어력 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndSpeedPotionCoolTime()
{
	bIsSpeedPotionActive = false;
	
	UE_LOG(LogTemp, Error, TEXT("신속 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndBerserkPotionCoolTime()
{
	bIsBerserkPotionActive = false;
	
	UE_LOG(LogTemp, Error, TEXT("광전사 포션을 사용할 수 있습니다."));
}

bool UT3ItemUseComponent::ApplyConsumableItem(const FT3ConsumableItemData& ItemData)
{
	switch (ItemData.EffectType)
	{
	case EEffectType::HP:
		{
			OwnerCharacter->RestoreHP(ItemData.BuffValue);
			return true;
		}
	case EEffectType::MP:
		{
			OwnerCharacter->RestoreMP(ItemData.BuffValue);
			return true;
		}
	case EEffectType::Power:
		{
			if (bIsPowerPotionActive)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString()); // 쿨타임 계산 로직 필요하면 나중에 추가예정
				
				return false;
			}
			else
			{
				bIsPowerPotionActive = true;
				
				PendingPowerValue = ItemData.BuffValue;
			
				OwnerCharacter->SetAttackPower(OwnerCharacter->GetAttackPower() * PendingPowerValue);
			
				UE_LOG(LogTemp, Warning, TEXT("[%s] 사용. 현재 공격력: %f"), *ItemData.ItemData.Name.ToString(), OwnerCharacter->GetAttackPower());

				// 효과 지속 타이머
				GetWorld()->GetTimerManager().SetTimer(
					PowerPotionActiveTimerHandle,
					this,
					&UT3ItemUseComponent::EndPowerPotionEffect,
					ItemData.ActiveTime,
					false);
				
				// 쿨타임 타이머
				GetWorld()->GetTimerManager().SetTimer(
					PowerPotionCoolTimerHandle,
					this,
					&UT3ItemUseComponent::EndPowerPotionCoolTime,
					ItemData.CoolTime,
					false);
				
				return true;
			}
		}
	case EEffectType::Defense:
		{
			if (bIsDefensePotionActive)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString());
				
				return false;
			}
			else
			{
				bIsDefensePotionActive = true;
				
				PendingDefenseValue = ItemData.BuffValue;
			
				OwnerCharacter->SetDefense(OwnerCharacter->GetDefense() * PendingDefenseValue);

				UE_LOG(LogTemp, Warning, TEXT("[%s] 사용. 현재 방어력: %f"), *ItemData.ItemData.Name.ToString(), OwnerCharacter->GetDefense());

				GetWorld()->GetTimerManager().SetTimer(
					DefensePotionActiveTimerHandle,
					this,
					&UT3ItemUseComponent::EndDefensePotionEffect,
					ItemData.ActiveTime,
					false);
			
				GetWorld()->GetTimerManager().SetTimer(
						DefensePotionCoolTimerHandle,
						this,
						&UT3ItemUseComponent::EndDefensePotionCoolTime,
						ItemData.CoolTime,
						false);
			
				return true;
			}
		}
	case EEffectType::Speed:
		{
			PendingSpeedValue = ItemData.BuffValue;
			
			// Speed 게터, 세터 요청
			
			return true;
		}
	case EEffectType::Berserk:
		{
			if (bIsBerserkPotionActive)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString());
				
				return false;
			}
			else
			{
				bIsBerserkPotionActive = true;
				
				PendingBerserkPowerValue = ItemData.BuffValue;
				PendingBerserkDefenseValue = ItemData.DebuffValue;
			
				OwnerCharacter->SetAttackPower(OwnerCharacter->GetAttackPower() * PendingBerserkPowerValue);
				OwnerCharacter->SetDefense(OwnerCharacter->GetDefense() / PendingBerserkDefenseValue);
			
				UE_LOG(LogTemp, Warning, TEXT("[%s] 사용. 현재 공격력: %f, 방어력: %f"), *ItemData.ItemData.Name.ToString(), OwnerCharacter->GetAttackPower(), OwnerCharacter->GetDefense());

				GetWorld()->GetTimerManager().SetTimer(
					BerserkPotionActiveTimerHandle,
					this,
					&UT3ItemUseComponent::EndBerserkPotionEffect,
					ItemData.ActiveTime,
					false);
			
				GetWorld()->GetTimerManager().SetTimer(
						BerserkPotionCoolTimerHandle,
						this,
						&UT3ItemUseComponent::EndBerserkPotionCoolTime,
						ItemData.CoolTime,
						false);
			
				return true;	
			}
		}
	default:
		return false;
	}
}