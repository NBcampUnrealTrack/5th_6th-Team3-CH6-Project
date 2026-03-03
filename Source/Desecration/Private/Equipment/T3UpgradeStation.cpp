// Fill out your copyright notice in the Description page of Project Settings.

#include "Equipment/T3UpgradeStation.h"
#include "Desecration.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "Player/T3CharacterBase.h"
#include "Equipment/T3PlayerEquipmentComponent.h"
#include "Equipment/T3TestItemInstance.h"
#include "Item/Component/T3InventoryComponent.h"

// ============================================================================
// 생성자 및 초기화
// ============================================================================

AT3UpgradeStation::AT3UpgradeStation()
{
	PrimaryActorTick.bCanEverTick = false;

	// 루트 컴포넌트로 메쉬 생성
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	// 기본 큐브 메쉬 설정
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		MeshComponent->SetStaticMesh(CubeMesh.Object);
		MeshComponent->SetWorldScale3D(FVector(0.5f, 0.5f, 1.0f));
	}

	// 상호작용 스피어 생성
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionSphere->SetGenerateOverlapEvents(true);

	// 초기화
	PlayerInRange = nullptr;
}

void AT3UpgradeStation::BeginPlay()
{
	Super::BeginPlay();

	// 스피어 반경 업데이트
	InteractionSphere->SetSphereRadius(InteractionRadius);

	// 오버랩 이벤트 바인딩
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AT3UpgradeStation::OnSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AT3UpgradeStation::OnSphereEndOverlap);

	UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] 초기화 완료 (MaxLevel: %d)"), MaxUpgradeLevel);
}

// ============================================================================
// 오버랩 이벤트 (Core)
// ============================================================================

void AT3UpgradeStation::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AT3CharacterBase* Character = Cast<AT3CharacterBase>(OtherActor))
	{
		PlayerInRange = Character;
		UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] 플레이어 감지"));

		// [TEST] F키 입력 바인딩
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			BindInputToPlayer(PC);
		}
	}
}

void AT3UpgradeStation::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AT3CharacterBase* Character = Cast<AT3CharacterBase>(OtherActor))
	{
		if (PlayerInRange == Character)
		{
			// [TEST] F키 입력 해제
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
			{
				UnbindInputFromPlayer(PC);
			}

			PlayerInRange = nullptr;
			UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] 플레이어가 범위를 벗어남"));

			// UI 자동 닫기
			if (bIsUpgradeUIOpen)
			{
				CloseUpgradeUI();
			}
		}
	}
}

// ============================================================================
// IT3Interactable 인터페이스 구현 (Core)
// ============================================================================

void AT3UpgradeStation::Interact_Implementation(AT3CharacterBase* Interactor)
{
	if (!Interactor) return;

	// UI 토글
	if (bIsUpgradeUIOpen)
	{
		CloseUpgradeUI();
	}
	else
	{
		OpenUpgradeUI();
	}
}

bool AT3UpgradeStation::CanInteract_Implementation(AT3CharacterBase* Interactor) const
{
	return PlayerInRange != nullptr && PlayerInRange == Interactor;
}

FText AT3UpgradeStation::GetInteractionPrompt_Implementation() const
{
	return FText::FromString(TEXT("F - 장비 강화"));
}

// ============================================================================
// UI 제어 함수 (Core)
// ============================================================================

void AT3UpgradeStation::OpenUpgradeUI()
{
	if (bIsUpgradeUIOpen) return;

	bIsUpgradeUIOpen = true;

	// 위젯 생성 + Viewport 추가
	if (UpgradeWidgetClass)
	{
		UWorld* World = GetWorld();
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (PC)
		{
			UpgradeWidgetInstance = CreateWidget<UUserWidget>(PC, UpgradeWidgetClass);
			if (UpgradeWidgetInstance)
			{
				UpgradeWidgetInstance->AddToViewport(99);

				// 마우스 커서 표시 + UI 입력 모드
				PC->SetShowMouseCursor(true);
				FInputModeGameAndUI InputMode;
				InputMode.SetWidgetToFocus(UpgradeWidgetInstance->TakeWidget());
				PC->SetInputMode(InputMode);
			}
		}
	}

	OnUpgradeUIOpened.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] UI 열림"));
}

void AT3UpgradeStation::CloseUpgradeUI()
{
	if (!bIsUpgradeUIOpen) return;

	bIsUpgradeUIOpen = false;

	// 위젯 제거
	if (UpgradeWidgetInstance)
	{
		UpgradeWidgetInstance->RemoveFromParent();
		UpgradeWidgetInstance = nullptr;
	}

	// 게임 입력 모드 복원
	UWorld* World = GetWorld();
	if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	OnUpgradeUIClosed.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] UI 닫힘"));
}

// ============================================================================
// 장비 정보 조회 (Core - Widget Blueprint에서 호출)
// ============================================================================

UT3PlayerEquipmentComponent* AT3UpgradeStation::GetPlayerEquipmentComponent() const
{
	if (!PlayerInRange) return nullptr;
	return PlayerInRange->FindComponentByClass<UT3PlayerEquipmentComponent>();
}

UT3InventoryComponent* AT3UpgradeStation::GetPlayerInventoryComponent() const
{
	if (!PlayerInRange) return nullptr;
	return PlayerInRange->FindComponentByClass<UT3InventoryComponent>();
}

FT3UpgradeUIData AT3UpgradeStation::GetWeaponUIData() const
{
	return GetEquipmentUIData(ET3EquipmentType::Weapon);
}

FT3UpgradeUIData AT3UpgradeStation::GetArmorUIData() const
{
	return GetEquipmentUIData(ET3EquipmentType::Armor);
}

FT3UpgradeUIData AT3UpgradeStation::GetEquipmentUIData(ET3EquipmentType EquipmentType) const
{
	FT3UpgradeUIData UIData;
	UIData.EquipmentType = EquipmentType;

	UT3PlayerEquipmentComponent* EquipComp = GetPlayerEquipmentComponent();
	if (!EquipComp) return UIData;

	// 장비 인스턴스 가져오기
	UT3TestItemInstance* ItemInstance = nullptr;
	if (EquipmentType == ET3EquipmentType::Weapon)
	{
		ItemInstance = EquipComp->WeaponInstance;
		UIData.CurrentStat = EquipComp->GetCurrentAttackPower();
	}
	else
	{
		ItemInstance = EquipComp->ArmorInstance;
		UIData.CurrentStat = EquipComp->GetCurrentDefensePower();
	}

	if (!ItemInstance)
	{
		UIData.bIsEquipped = false;
		return UIData;
	}

	// 기본 정보 설정
	UIData.bIsEquipped = true;
	UIData.ItemID = ItemInstance->ItemID;
	UIData.CurrentLevel = ItemInstance->CurrentLevel;

	int32 NextLevel = ItemInstance->CurrentLevel + 1;
	int32 NextLevelIdx = NextLevel - 1; // 배열 인덱스

	// 테이블에서 표시 정보 및 다음 레벨 스탯 조회
	// 최대 레벨은 데이터 테이블의 LevelStats 배열 크기로 결정
	if (EquipmentType == ET3EquipmentType::Weapon)
	{
		if (EquipComp->WeaponTable)
		{
			const FT3WeaponDataRow* Row = EquipComp->WeaponTable->FindRow<FT3WeaponDataRow>(ItemInstance->ItemID, TEXT("UIData"));
			if (Row)
			{
				UIData.DisplayName = Row->DisplayName;
				UIData.Icon = Row->Icon;

				// 데이터 테이블 기준 최대 레벨 판정
				UIData.bIsMaxLevel = !Row->LevelStats.IsValidIndex(NextLevelIdx);

				if (!UIData.bIsMaxLevel)
				{
					UIData.NextLevelStat = Row->LevelStats[NextLevelIdx].FixedAttackPower;
				}
			}
		}
	}
	else
	{
		if (EquipComp->ArmorTable)
		{
			const FT3ArmorDataRow* Row = EquipComp->ArmorTable->FindRow<FT3ArmorDataRow>(ItemInstance->ItemID, TEXT("UIData"));
			if (Row)
			{
				UIData.DisplayName = Row->DisplayName;
				UIData.Icon = Row->Icon;

				// 데이터 테이블 기준 최대 레벨 판정
				UIData.bIsMaxLevel = !Row->LevelStats.IsValidIndex(NextLevelIdx);

				if (!UIData.bIsMaxLevel)
				{
					UIData.NextLevelStat = Row->LevelStats[NextLevelIdx].FixedDefensePower;
				}
			}
		}
	}

	UIData.bCanUpgrade = !UIData.bIsMaxLevel;

	// 사용 가능한 강화석이 없으면 강화 불가
	ET3UpgradeStoneGrade TempGrade;
	if (!SelectLowestAvailableStone(UIData.CurrentLevel, TempGrade))
	{
		UIData.bCanUpgrade = false;
	}

	return UIData;
}

// ============================================================================
// 강화석 조회 (Core)
// ============================================================================

int32 AT3UpgradeStation::GetStoneCount(ET3UpgradeStoneGrade Grade) const
{
	UT3InventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp) return 0;

	switch (Grade)
	{
	case ET3UpgradeStoneGrade::Normal:    return InvComp->GetNormalStoneCount();
	case ET3UpgradeStoneGrade::Epic:      return InvComp->GetEpicStoneCount();
	case ET3UpgradeStoneGrade::Legendary: return InvComp->GetLegendaryStoneCount();
	default: return 0;
	}
}

UTexture2D* AT3UpgradeStation::GetStoneIcon(ET3UpgradeStoneGrade Grade) const
{
	switch (Grade)
	{
	case ET3UpgradeStoneGrade::Normal:    return NormalStoneIcon;
	case ET3UpgradeStoneGrade::Epic:      return EpicStoneIcon;
	case ET3UpgradeStoneGrade::Legendary: return LegendaryStoneIcon;
	default: return nullptr;
	}
}

TArray<ET3UpgradeStoneGrade> AT3UpgradeStation::GetAvailableStones(int32 CurrentEquipmentLevel) const
{
	TArray<ET3UpgradeStoneGrade> AvailableStones;

	if (CanUseStone(ET3UpgradeStoneGrade::Normal, CurrentEquipmentLevel) && GetStoneCount(ET3UpgradeStoneGrade::Normal) > 0)
	{
		AvailableStones.Add(ET3UpgradeStoneGrade::Normal);
	}
	if (CanUseStone(ET3UpgradeStoneGrade::Epic, CurrentEquipmentLevel) && GetStoneCount(ET3UpgradeStoneGrade::Epic) > 0)
	{
		AvailableStones.Add(ET3UpgradeStoneGrade::Epic);
	}
	if (CanUseStone(ET3UpgradeStoneGrade::Legendary, CurrentEquipmentLevel) && GetStoneCount(ET3UpgradeStoneGrade::Legendary) > 0)
	{
		AvailableStones.Add(ET3UpgradeStoneGrade::Legendary);
	}

	return AvailableStones;
}

bool AT3UpgradeStation::CanUseStone(ET3UpgradeStoneGrade Grade, int32 CurrentEquipmentLevel) const
{
	int32 NextLevel = CurrentEquipmentLevel + 1;
	return NextLevel <= GetMaxLevelForStone(Grade);
}

int32 AT3UpgradeStation::GetMaxLevelForStone(ET3UpgradeStoneGrade Grade)
{
	switch (Grade)
	{
	case ET3UpgradeStoneGrade::Normal:    return 3;
	case ET3UpgradeStoneGrade::Epic:      return 5;
	case ET3UpgradeStoneGrade::Legendary: return 7;
	default: return 0;
	}
}

// ============================================================================
// 강화 실행 (Core - Widget Blueprint에서 호출)
// ============================================================================

bool AT3UpgradeStation::UpgradeWeapon()
{
	return UpgradeEquipment(ET3EquipmentType::Weapon);
}

bool AT3UpgradeStation::UpgradeArmor()
{
	return UpgradeEquipment(ET3EquipmentType::Armor);
}

bool AT3UpgradeStation::UpgradeEquipment(ET3EquipmentType EquipmentType)
{
	UT3PlayerEquipmentComponent* EquipComp = GetPlayerEquipmentComponent();
	if (!EquipComp)
	{
		OnUpgradeFailed.Broadcast(FText::FromString(TEXT("장비 컴포넌트를 찾을 수 없습니다.")));
		return false;
	}

	// 현재 장비 레벨 조회
	UT3TestItemInstance* ItemInstance = (EquipmentType == ET3EquipmentType::Weapon)
		? EquipComp->WeaponInstance
		: EquipComp->ArmorInstance;

	if (!ItemInstance)
	{
		OnUpgradeFailed.Broadcast(FText::FromString(TEXT("장착된 장비가 없습니다.")));
		return false;
	}

	// 사용 가능한 최하급 강화석 자동 선택
	ET3UpgradeStoneGrade SelectedGrade;
	if (!SelectLowestAvailableStone(ItemInstance->CurrentLevel, SelectedGrade))
	{
		OnUpgradeFailed.Broadcast(FText::FromString(TEXT("사용 가능한 강화석이 없습니다.")));
		UE_LOG(LogDesecration, Warning, TEXT("[UpgradeStation] 강화 실패 - 사용 가능한 강화석 없음 (레벨: %d)"),
			ItemInstance->CurrentLevel);
		return false;
	}

	// 강화 시도 (MaxAllowedLevel = 강화석 등급별 최대 레벨)
	int32 MaxAllowedLevel = GetMaxLevelForStone(SelectedGrade);
	bool bSuccess = EquipComp->TryUpgrade(EquipmentType, MaxAllowedLevel);

	if (bSuccess)
	{
		// 강화석 1개 차감
		ConsumeStone(SelectedGrade);

		// 성공 델리게이트 발송
		OnUpgradeSuccess.Broadcast(EquipmentType);

		FString TypeName = (EquipmentType == ET3EquipmentType::Weapon) ? TEXT("무기") : TEXT("방어구");
		float NewStat = (EquipmentType == ET3EquipmentType::Weapon)
			? EquipComp->GetCurrentAttackPower()
			: EquipComp->GetCurrentDefensePower();

		UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] %s 강화 성공! 새 스탯: %.1f (사용 강화석: %d등급, 남은 수량: %d)"),
			*TypeName, NewStat, static_cast<uint8>(SelectedGrade), GetStoneCount(SelectedGrade));
	}
	else
	{
		// 실패 델리게이트 발송
		OnUpgradeFailed.Broadcast(FText::FromString(TEXT("강화에 실패했습니다. (최대 레벨이거나 데이터 없음)")));

		UE_LOG(LogDesecration, Warning, TEXT("[UpgradeStation] 강화 실패"));
	}

	return bSuccess;
}

bool AT3UpgradeStation::GetNextStoneGrade(int32 CurrentEquipmentLevel, ET3UpgradeStoneGrade& OutGrade) const
{
	return SelectLowestAvailableStone(CurrentEquipmentLevel, OutGrade);
}

bool AT3UpgradeStation::SelectLowestAvailableStone(int32 CurrentEquipmentLevel, ET3UpgradeStoneGrade& OutGrade) const
{
	// 낮은 등급부터 순회하여 사용 가능한 첫 번째 강화석 선택
	// Normal → Epic → Legendary 순서
	const ET3UpgradeStoneGrade Priority[] = {
		ET3UpgradeStoneGrade::Normal,
		ET3UpgradeStoneGrade::Epic,
		ET3UpgradeStoneGrade::Legendary
	};

	for (ET3UpgradeStoneGrade Grade : Priority)
	{
		if (CanUseStone(Grade, CurrentEquipmentLevel) && GetStoneCount(Grade) > 0)
		{
			OutGrade = Grade;
			return true;
		}
	}

	return false;
}

void AT3UpgradeStation::ConsumeStone(ET3UpgradeStoneGrade Grade)
{
	UT3InventoryComponent* InvComp = GetPlayerInventoryComponent();
	if (!InvComp) return;

	switch (Grade)
	{
	case ET3UpgradeStoneGrade::Normal:
		InvComp->SetNormalStoneCount(InvComp->GetNormalStoneCount() - 1);
		break;
	case ET3UpgradeStoneGrade::Epic:
		InvComp->SetEpicStoneCount(InvComp->GetEpicStoneCount() - 1);
		break;
	case ET3UpgradeStoneGrade::Legendary:
		InvComp->SetLegendaryStoneCount(InvComp->GetLegendaryStoneCount() - 1);
		break;
	}
}

// ============================================================================
// [TEST] 테스트용 코드 - 정식 Interaction 시스템 연동 후 제거 예정
// ============================================================================
#pragma region TEST_CODE

void AT3UpgradeStation::HandleInteractInput()
{
	if (PlayerInRange)
	{
		UE_LOG(LogDesecration, Log, TEXT("[TEST] F키 입력 감지"));
		Interact_Implementation(PlayerInRange);
	}
}

void AT3UpgradeStation::BindInputToPlayer(APlayerController* PC)
{
	if (!PC) return;

	EnableInput(PC);

	if (!InputComponent)
	{
		InputComponent = NewObject<UInputComponent>(this, TEXT("StationInputComponent"));
		InputComponent->RegisterComponent();
	}

	// F키 직접 바인딩 (레거시 방식)
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &AT3UpgradeStation::HandleInteractInput);

	UE_LOG(LogDesecration, Log, TEXT("[TEST] F키 바인딩 완료"));
}

void AT3UpgradeStation::UnbindInputFromPlayer(APlayerController* PC)
{
	if (!PC) return;

	DisableInput(PC);

	if (InputComponent)
	{
		InputComponent->ClearActionBindings();
	}

	UE_LOG(LogDesecration, Log, TEXT("[TEST] F키 바인딩 해제"));
}

// [TEST] 레거시 강화 함수 (Deprecated)
bool AT3UpgradeStation::TryUpgradeWeapon(int32 MaxLevel)
{
	int32 OriginalMax = MaxUpgradeLevel;
	MaxUpgradeLevel = MaxLevel;

	bool bResult = UpgradeWeapon();

	MaxUpgradeLevel = OriginalMax;
	return bResult;
}

bool AT3UpgradeStation::TryUpgradeArmor(int32 MaxLevel)
{
	int32 OriginalMax = MaxUpgradeLevel;
	MaxUpgradeLevel = MaxLevel;

	bool bResult = UpgradeArmor();

	MaxUpgradeLevel = OriginalMax;
	return bResult;
}

#pragma endregion TEST_CODE
