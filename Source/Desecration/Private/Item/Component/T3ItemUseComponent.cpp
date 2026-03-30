#include "Item/Component/T3ItemUseComponent.h"
#include "Item/Data/T3ConsumableItemData.h"
#include "Player/T3CharacterBase.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

UT3ItemUseComponent::UT3ItemUseComponent()
: OriginalPowerValue(0.f),
OriginalDefenseValue(0.f),
OriginalSpeedValue(0.f),
PendingPowerValue(0.f),
PendingDefenseValue(0.f),
PendingSpeedValue(0.f),
PendingBerserkPowerValue(0.f),
PendingBerserkDefenseValue(0.f),
bIsHPPotionActive(false),
bIsMPPotionActive(false),
bIsPowerPotionActive(false),
bIsDefensePotionActive(false),
bIsSpeedPotionActive(false),
bIsBerserkPotionActive(false),
bIsHPPotionCooldown(false),
bIsMPPotionCooldown(false),
bIsPowerPotionCooldown(false),
bIsDefensePotionCooldown(false),
bIsSpeedPotionCooldown(false),
bIsBerserkPotionCooldown(false),
RecoverHPInterval(0.05f),
RecoverHPTickCount(0.f),
RecoverHPPerTick(0.f),
RecoverHPAmount(0.f),
AccumulatedRecoverHP(0.f),
RecoverMPInterval(0.05f),
RecoverMPTickCount(0.f),
RecoverMPPerTick(0.f),
RecoverMPAmount(0.f),
AccumulatedRecoverMP(0.f)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UT3ItemUseComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<AT3CharacterBase>(GetOwner());

	if (!IsValid(OwnerCharacter))
	{
		return;
	}

	OriginalPowerValue = OwnerCharacter->GetAttackPower();
	OriginalDefenseValue = OwnerCharacter->GetDefense();
	OriginalSpeedValue = OwnerCharacter->GetMoveSpeed();
}

void UT3ItemUseComponent::EndPowerPotionEffect()
{
	bIsPowerPotionActive = false;
	
	if (bIsBerserkPotionActive)
	{
		OwnerCharacter->SetAttackPower(OriginalPowerValue * PendingBerserkPowerValue);
	}
	else
	{
		OwnerCharacter->SetAttackPower(OriginalPowerValue);
	}
	
	UE_LOG(LogTemp, Error, TEXT("공격력 포션 종료. 현재 공격력: %f"), OwnerCharacter->GetAttackPower());
}

void UT3ItemUseComponent::EndDefensePotionEffect()
{
	bIsDefensePotionActive = false;
	
	if (bIsBerserkPotionActive)
	{
		OwnerCharacter->SetDefense(OriginalDefenseValue / PendingBerserkDefenseValue);
	}
	else
	{
		OwnerCharacter->SetDefense(OriginalDefenseValue);
	}
	
	UE_LOG(LogTemp, Error, TEXT("방어력 포션 종료. 현재 방어력: %f"), OwnerCharacter->GetDefense());
}

void UT3ItemUseComponent::EndSpeedPotionEffect()
{
	bIsSpeedPotionActive = false;
	
	OwnerCharacter->SetMoveSpeed(OriginalSpeedValue);
	
	UE_LOG(LogTemp, Error, TEXT("신속 포션 종료. 현재 이동속도: %f"), OwnerCharacter->GetMoveSpeed());
}

void UT3ItemUseComponent::EndBerserkPotionEffect()
{
	bIsBerserkPotionActive = false;
	
	if (bIsPowerPotionActive)
	{
		OwnerCharacter->SetAttackPower(OriginalPowerValue * PendingPowerValue);
	}
	else
	{
		OwnerCharacter->SetAttackPower(OriginalPowerValue);
	}
	
	if (bIsDefensePotionActive)
	{
		OwnerCharacter->SetDefense(OriginalDefenseValue * PendingDefenseValue);
	}
	else
	{
		OwnerCharacter->SetDefense(OriginalDefenseValue);
	}
	
	UE_LOG(LogTemp, Error, TEXT("광전사 포션 종료. 현재 공격력: %f, 방어력: %f"), OwnerCharacter->GetAttackPower(), OwnerCharacter->GetDefense());
}

void UT3ItemUseComponent::EndHPPotionCoolTime()
{
	// 타이머 매니저 컨테이너 변경 방지를 위해 매우 작은 딜레이로 플래그 해제
	// 0.001초 딜레이는 거의 즉시 실행되지만 타이머 매니저 순회가 끝난 후 실행됨
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UT3ItemUseComponent::ClearHPPotionCoolTime, 0.001f, false);
}

void UT3ItemUseComponent::ClearHPPotionCoolTime()
{
	bIsHPPotionCooldown = false;
	UE_LOG(LogTemp, Error, TEXT("체력 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndMPPotionCoolTime()
{
	// 타이머 매니저 컨테이너 변경 방지를 위해 매우 작은 딜레이로 플래그 해제
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UT3ItemUseComponent::ClearMPPotionCoolTime, 0.001f, false);
}

void UT3ItemUseComponent::ClearMPPotionCoolTime()
{
	bIsMPPotionCooldown = false;
	UE_LOG(LogTemp, Error, TEXT("마나 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndPowerPotionCoolTime()
{
	// 타이머 매니저 컨테이너 변경 방지를 위해 매우 작은 딜레이로 플래그 해제
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UT3ItemUseComponent::ClearPowerPotionCoolTime, 0.001f, false);
}

void UT3ItemUseComponent::ClearPowerPotionCoolTime()
{
	bIsPowerPotionCooldown = false;
	UE_LOG(LogTemp, Error, TEXT("공격력 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndDefensePotionCoolTime()
{
	// 타이머 매니저 컨테이너 변경 방지를 위해 매우 작은 딜레이로 플래그 해제
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UT3ItemUseComponent::ClearDefensePotionCoolTime, 0.001f, false);
}

void UT3ItemUseComponent::ClearDefensePotionCoolTime()
{
	bIsDefensePotionCooldown = false;
	UE_LOG(LogTemp, Error, TEXT("방어력 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndSpeedPotionCoolTime()
{
	// 타이머 매니저 컨테이너 변경 방지를 위해 매우 작은 딜레이로 플래그 해제
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UT3ItemUseComponent::ClearSpeedPotionCoolTime, 0.001f, false);
}

void UT3ItemUseComponent::ClearSpeedPotionCoolTime()
{
	bIsSpeedPotionCooldown = false;
	UE_LOG(LogTemp, Error, TEXT("신속 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::EndBerserkPotionCoolTime()
{
	// 타이머 매니저 컨테이너 변경 방지를 위해 매우 작은 딜레이로 플래그 해제
	FTimerHandle TempHandle;
	GetWorld()->GetTimerManager().SetTimer(TempHandle, this, &UT3ItemUseComponent::ClearBerserkPotionCoolTime, 0.001f, false);
}

void UT3ItemUseComponent::ClearBerserkPotionCoolTime()
{
	bIsBerserkPotionCooldown = false;
	UE_LOG(LogTemp, Error, TEXT("광전사 포션을 사용할 수 있습니다."));
}

void UT3ItemUseComponent::PlayItemUseEffect(const FT3ConsumableItemData& ItemData)
{
	if (IsValid(ItemData.UseEffect) && IsValid(OwnerCharacter))
	{
		USkeletalMeshComponent* MeshComp = OwnerCharacter->GetMesh();
		if (IsValid(MeshComp))
		{
			UNiagaraFunctionLibrary::SpawnSystemAttached(
				ItemData.UseEffect,
				MeshComp,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::SnapToTarget,
				true
			);
		}
	}
}

void UT3ItemUseComponent::PlayItemUseSound()
{
	UGameplayStatics::PlaySoundAtLocation(
				this,
				HealSound,
				OwnerCharacter->GetActorLocation()
				);
}

void UT3ItemUseComponent::RecoverHPTick()
{
	float RemainAmount = RecoverHPAmount - AccumulatedRecoverHP;
	float ApplyAmount = FMath::Min(RecoverHPPerTick, RemainAmount);
	
	OwnerCharacter->RestoreHP(ApplyAmount);
	AccumulatedRecoverHP += ApplyAmount;
	
	UE_LOG(LogTemp, Log, TEXT("체력 [%.1f] 회복"), ApplyAmount);

	if (AccumulatedRecoverHP >= RecoverHPAmount)
	{
		GetWorld()->GetTimerManager().ClearTimer(RecoverHPTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("현재 캐릭터의 체력 : %.1f"), OwnerCharacter->GetCurrentHP());
	}
}

void UT3ItemUseComponent::RecoverMPTick()
{
	float RemainAmount = RecoverMPAmount - AccumulatedRecoverMP;
	float ApplyAmount = FMath::Min(RecoverMPPerTick, RemainAmount);
	
	OwnerCharacter->RestoreMP(ApplyAmount);
	AccumulatedRecoverMP += ApplyAmount;
	
	UE_LOG(LogTemp, Log, TEXT("마나 [%.1f] 회복"), ApplyAmount);

	if (AccumulatedRecoverMP >= RecoverMPAmount)
	{
		GetWorld()->GetTimerManager().ClearTimer(RecoverMPTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("현재 캐릭터의 마나 : %.1f"), OwnerCharacter->GetCurrentMana());
	}
}

bool UT3ItemUseComponent::ApplyConsumableItem(const FT3ConsumableItemData& ItemData, int32 RecoveryBonus)
{
	switch (ItemData.EffectType)
	{
	case EEffectType::HP:
		{
			if (bIsHPPotionCooldown)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString()); // 쿨타임 계산 로직 필요하면 나중에 추가예정
	
				return false;
			}
			else
			{
				bIsHPPotionCooldown = true;
				
				RecoverHPAmount = OwnerCharacter->GetMaxHP() * (ItemData.BuffValue + RecoveryBonus) / 100.0f;
			
				RecoverHPTickCount = ItemData.ActiveTime / RecoverHPInterval;
				RecoverHPPerTick = RecoverHPAmount / RecoverHPTickCount;
			
				AccumulatedRecoverHP = 0.f;
			
				GetWorld()->GetTimerManager().SetTimer(
					RecoverHPTimerHandle,
					this,
					&UT3ItemUseComponent::RecoverHPTick,
					RecoverHPInterval,
					true);
			
				GetWorld()->GetTimerManager().SetTimer(
					HPPotionCoolTimerHandle,
					this,
					&UT3ItemUseComponent::EndHPPotionCoolTime,
					ItemData.CoolTime,
					false);
			
				PlayItemUseEffect(ItemData);
				
				PlayItemUseSound();
				
				return true;
			}
		}
	case EEffectType::MP:
		{
			if (bIsMPPotionCooldown)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString()); // 쿨타임 계산 로직 필요하면 나중에 추가예정

				return false;
			}
			else
			{
				bIsMPPotionCooldown = true;
				
				RecoverMPAmount = OwnerCharacter->GetMaxMana() * (ItemData.BuffValue + RecoveryBonus) / 100.0f;
			
				RecoverMPTickCount = ItemData.ActiveTime / RecoverMPInterval;
				RecoverMPPerTick = RecoverMPAmount / RecoverMPTickCount;
			
				AccumulatedRecoverMP = 0.f;
			
				GetWorld()->GetTimerManager().SetTimer(
					RecoverMPTimerHandle,
					this,
					&UT3ItemUseComponent::RecoverMPTick,
					RecoverMPInterval,
					true);
			
				GetWorld()->GetTimerManager().SetTimer(
					MPPotionCoolTimerHandle,
					this,
					&UT3ItemUseComponent::EndMPPotionCoolTime,
					ItemData.CoolTime,
					false);
			
				PlayItemUseEffect(ItemData);
				
				PlayItemUseSound();
				
				return true;
			}
		}
	case EEffectType::Power:
		{
			if (bIsPowerPotionCooldown)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString()); // 쿨타임 계산 로직 필요하면 나중에 추가예정
				
				return false;
			}
			else
			{
				bIsPowerPotionCooldown = true;
				
				PendingPowerValue = ItemData.BuffValue;
				
				if (bIsBerserkPotionActive)
				{
					float Result = FMath::RoundToFloat(OriginalPowerValue * PendingPowerValue * PendingBerserkPowerValue * 10.f) / 10.f;
					OwnerCharacter->SetAttackPower(Result);
				}
				else
				{
					float Result = FMath::RoundToFloat(OriginalPowerValue * PendingPowerValue * 10.f) / 10.f;
					OwnerCharacter->SetAttackPower(Result);
				}
				
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
				
				PlayItemUseEffect(ItemData);
				
				PlayItemUseSound();
				
				return true;
			}
		}
	case EEffectType::Defense:
		{
			if (bIsDefensePotionCooldown)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString());
				
				return false;
			}
			else
			{
				bIsDefensePotionCooldown = true;
				
				PendingDefenseValue = ItemData.BuffValue;
				
				if (bIsBerserkPotionActive)
				{
					float Result = FMath::RoundToFloat(OriginalDefenseValue * PendingDefenseValue / PendingBerserkDefenseValue * 10.0f) / 10.0f;
					OwnerCharacter->SetDefense(Result);
				}
				else
				{
					float Result = FMath::RoundToFloat(OriginalDefenseValue * PendingDefenseValue * 10.0f) / 10.0f;
					OwnerCharacter->SetDefense(Result);
				}

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
			
				PlayItemUseEffect(ItemData);
				
				PlayItemUseSound();
				
				return true;
			}
		}
	case EEffectType::Speed:
		{
			if (bIsSpeedPotionCooldown)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString());
				
				return false;
			}
			else
			{
				bIsSpeedPotionCooldown = true;
				
				PendingSpeedValue = ItemData.BuffValue;
				
				float Result = FMath::RoundToFloat(OriginalSpeedValue * PendingSpeedValue * 10.0f) / 10.0f;
				OwnerCharacter->SetMoveSpeed(Result);

				UE_LOG(LogTemp, Warning, TEXT("[%s] 사용. 현재 이동속도: %f"), *ItemData.ItemData.Name.ToString(), OwnerCharacter->GetMoveSpeed());

				GetWorld()->GetTimerManager().SetTimer(
					SpeedPotionActiveTimerHandle,
					this,
					&UT3ItemUseComponent::EndSpeedPotionEffect,
					ItemData.ActiveTime,
					false);
			
				GetWorld()->GetTimerManager().SetTimer(
						SpeedPotionCoolTimerHandle,
						this,
						&UT3ItemUseComponent::EndSpeedPotionCoolTime,
						ItemData.CoolTime,
						false);
				
				PlayItemUseEffect(ItemData);
				
				PlayItemUseSound();
				
				return true;
			}
		}
	case EEffectType::Berserk:
		{
			if (bIsBerserkPotionCooldown)
			{
				UE_LOG(LogTemp, Error, TEXT("[%s]은 쿨타임 입니다."), *ItemData.ItemData.Name.ToString());
				
				return false;
			}
			else
			{
				bIsBerserkPotionCooldown = true;
				
				PendingBerserkPowerValue = ItemData.BuffValue;
				PendingBerserkDefenseValue = ItemData.DebuffValue;
				
				if (bIsPowerPotionActive)
				{
					float Result = FMath::RoundToFloat(OriginalPowerValue * PendingBerserkPowerValue * PendingPowerValue * 10.0f) / 10.f;
					OwnerCharacter->SetAttackPower(Result);
				}
				else
				{
					float Result = FMath::RoundToFloat(OriginalPowerValue * PendingBerserkPowerValue * 10.0f) / 10.0f;
					OwnerCharacter->SetAttackPower(Result);
				}
				
				if (bIsDefensePotionActive)
				{
					float Result = FMath::RoundToFloat(OriginalDefenseValue * PendingDefenseValue / PendingBerserkDefenseValue * 10.0f) / 10.f;
					OwnerCharacter->SetDefense(Result);
				}
				else
				{
					float Result = FMath::RoundToFloat(OriginalDefenseValue / PendingBerserkDefenseValue * 10.0f) / 10.0f;
					OwnerCharacter->SetDefense(Result);
				}
				
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
			
				PlayItemUseEffect(ItemData);
				
				PlayItemUseSound();
				
				return true;	
			}
		}
	default:
		return false;
	}
}
