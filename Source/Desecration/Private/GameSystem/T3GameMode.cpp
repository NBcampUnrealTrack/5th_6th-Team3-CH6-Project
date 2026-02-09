#include "GameSystem/T3GameMode.h"

#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3SaveGame.h"
#include "Player/T3CharacterBase.h"

void AT3GameMode::BeginPlay()
{
	Super::BeginPlay();
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!T3GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
}

bool AT3GameMode::SaveGame(const AT3CharacterBase* Character)
{
	//캐릭터 정보를 저장된 게임 데이터에 저장한다. 
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	SaveGame->CurrentHP = Character->GetCurrentHP();
	SaveGame->MaxMana = Character->GetMaxMana();
	SaveGame->CurrentMana = Character->GetCurrentMana();
	SaveGame->MaxStamina = Character->GetMaxStamina();
	SaveGame->CurrentStamina = Character->GetCurrentStamina();
	SaveGame->AttackPower = Character->GetAttackPower();
	SaveGame->CriticalChance = Character->GetCriticalChance();
	SaveGame->CriticalDamage = Character->GetCriticalDamage();
	SaveGame->MoveSpeed = Character->GetMoveSpeed();
	
	//저장
	return T3GameInstance->SaveGame();
}

void AT3GameMode::SetCharacterBySavedData(AT3CharacterBase* Character)
{
	//저장된 게임 데이터에서 캐릭터 정보를 가져온다. 
	const TObjectPtr<UT3SaveGame> SaveGame = T3GameInstance->GetSavedGameData();
	Character->SetCurrentHP(SaveGame->CurrentHP);
	Character->SetCurrentMana(SaveGame->CurrentMana);
	Character->SetCurrentStamina(SaveGame->CurrentStamina);
	Character->SetAttackPower(SaveGame->AttackPower);
	Character->SetCriticalChance(SaveGame->CriticalChance);
	Character->SetCriticalDamage(SaveGame->CriticalDamage);
	Character->SetMoveSpeed(SaveGame->MoveSpeed);
}
