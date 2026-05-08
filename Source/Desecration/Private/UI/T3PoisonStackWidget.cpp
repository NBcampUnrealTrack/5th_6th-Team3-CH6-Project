// T3PoisonStackWidget.cpp

#include "UI/T3PoisonStackWidget.h"
#include "Player/T3CharacterBase.h"

void UT3PoisonStackWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 소유 플레이어의 폰에서 캐릭터를 자동으로 찾아 바인딩
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AT3CharacterBase* Char = Cast<AT3CharacterBase>(PC->GetPawn()))
		{
			BindToCharacter(Char);
		}
		else
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[PoisonWidget] NativeConstruct: 소유 Pawn이 AT3CharacterBase가 아님. "
					 "InitializeWidget()으로 수동 바인딩 필요."));
		}
	}
}

void UT3PoisonStackWidget::NativeDestruct()
{
	UnbindFromCharacter();
	Super::NativeDestruct();
}

void UT3PoisonStackWidget::InitializeWidget(AT3CharacterBase* InCharacter)
{
	UnbindFromCharacter();
	BindToCharacter(InCharacter);
}

void UT3PoisonStackWidget::BindToCharacter(AT3CharacterBase* Character)
{
	if (!Character) return;

	OwnerCharacter = Character;
	Character->OnStatChanged.AddDynamic(this, &UT3PoisonStackWidget::HandleStatChanged);
	Character->OnPoisonActivated.AddDynamic(this, &UT3PoisonStackWidget::HandlePoisonActivated);

	// 바인딩 직후 현재 스택 상태를 즉시 반영 (위젯이 늦게 생성됐을 때를 대비)
	BP_OnStackChanged(Character->GetCurrentPoisonStack(), Character->MaxPoisonStack);
	BP_OnPoisonStateChanged(Character->GetIsPoisoned());

	UE_LOG(LogTemp, Log, TEXT("[PoisonWidget] %s 에 바인딩 완료"), *Character->GetName());
}

void UT3PoisonStackWidget::UnbindFromCharacter()
{
	if (!OwnerCharacter) return;

	OwnerCharacter->OnStatChanged.RemoveDynamic(this, &UT3PoisonStackWidget::HandleStatChanged);
	OwnerCharacter->OnPoisonActivated.RemoveDynamic(this, &UT3PoisonStackWidget::HandlePoisonActivated);
	OwnerCharacter = nullptr;
}

void UT3PoisonStackWidget::HandleStatChanged(ET3StatType StatType, float CurrentValue, float MaxValue)
{
	if (StatType != ET3StatType::PoisonStack) return;

	BP_OnStackChanged(FMath::RoundToInt(CurrentValue), FMath::RoundToInt(MaxValue));
}

void UT3PoisonStackWidget::HandlePoisonActivated(bool bIsActive)
{
	BP_OnPoisonStateChanged(bIsActive);
}
