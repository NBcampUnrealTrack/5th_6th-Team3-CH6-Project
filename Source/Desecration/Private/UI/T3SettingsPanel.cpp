#include "UI/T3SettingsPanel.h"

#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "GameSystem/T3GameInstance.h"
#include "GameSystem/T3TitleGameMode.h"
#include "Player/T3TitlePlayerController.h"

void UT3SettingsPanel::NativeConstruct()
{
	Super::NativeConstruct();
	
	//게임 모드
	TitleGameMode = Cast<AT3TitleGameMode>(GetWorld()->GetAuthGameMode());
	if (!TitleGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitleGameMode is NULL"), *GetNameSafe(this));
		return;
	}
	
	//게임 인스턴스
	T3GameInstance = Cast<UT3GameInstance>(GetGameInstance());
	if (!TitleGameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : T3GameInstance is NULL"), *GetNameSafe(this));
		return;
	}
	
	//플레이어 컨트롤러
	TitlePlayerController = Cast<AT3TitlePlayerController>(GetOwningPlayer());
	if (!TitlePlayerController)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : TitlePlayerController is NULL"), *GetNameSafe(this));
		return;
	}
	
	//버튼 바인딩
	ConfirmButton->OnClicked.AddDynamic(this, &ThisClass::OnClickConfirmButton);
}

void UT3SettingsPanel::OnClickConfirmButton()
{
	TitlePlayerController->SetActiveSettingsPanel(false);
}
