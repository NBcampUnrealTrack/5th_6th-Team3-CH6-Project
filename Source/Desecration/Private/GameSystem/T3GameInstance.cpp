#include "GameSystem/T3GameInstance.h"

#include "GameFramework/GameUserSettings.h"
#include "GameSystem/T3SaveGame.h"
#include "Kismet/GameplayStatics.h"

void UT3GameInstance::Init()
{
	Super::Init();
	
	CurrentSettings = TSharedPtr<FSettings>();
	
	if (!GEngine)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : GEngine이 null"), *GetNameSafe(this));
		return;
	}
	UserSettings = GEngine->GetGameUserSettings();
	if (!UserSettings)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : GEngine의 GameUserSettings가 null"), *GetNameSafe(this));
		return;
	}
	
	SavedGameData = UGameplayStatics::LoadGameFromSlot(SAVE_GAME_NAME, 0);
}

FObjectPtr<UT3SaveGame> UT3GameInstance::LoadGame()
{
	return SavedGameData;
}

bool UT3GameInstance::SaveGame()
{
	return UGameplayStatics::SaveGameToSlot(SavedGameData, SAVE_GAME_NAME, 0);
}

void UT3GameInstance::SetScreenMode(ET3ScreenMode ScreenMode)
{
	CurrentSettings->ScreenMode = ScreenMode;
	UserSettings->SetFullscreenMode(static_cast<EWindowMode::Type>(ScreenMode));
	UserSettings->ApplySettings(true);
}
