// Fill out your copyright notice in the Description page of Project Settings.

#include "Equipment/T3UpgradeStation.h"
#include "Desecration.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Player/T3CharacterBase.h"
#include "Equipment/T3PlayerEquipmentComponent.h"
#include "Equipment/T3TestItemInstance.h"

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

	// 델리게이트 발송 (Widget Blueprint에서 수신)
	OnUpgradeUIOpened.Broadcast();

	UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] UI 열림"));
}

void AT3UpgradeStation::CloseUpgradeUI()
{
	if (!bIsUpgradeUIOpen) return;

	bIsUpgradeUIOpen = false;

	// 델리게이트 발송 (Widget Blueprint에서 수신)
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

	// 최대 레벨 체크
	int32 NextLevel = ItemInstance->CurrentLevel + 1;
	UIData.bIsMaxLevel = (NextLevel > MaxUpgradeLevel);
	UIData.bCanUpgrade = !UIData.bIsMaxLevel;

	// 테이블에서 표시 정보 및 다음 레벨 스탯 조회
	int32 NextLevelIdx = NextLevel - 1; // 배열 인덱스

	if (EquipmentType == ET3EquipmentType::Weapon)
	{
		if (EquipComp->WeaponTable)
		{
			const FT3WeaponDataRow* Row = EquipComp->WeaponTable->FindRow<FT3WeaponDataRow>(ItemInstance->ItemID, TEXT("UIData"));
			if (Row)
			{
				UIData.DisplayName = Row->DisplayName;
				UIData.Icon = Row->Icon;

				if (!UIData.bIsMaxLevel && Row->LevelStats.IsValidIndex(NextLevelIdx))
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

				if (!UIData.bIsMaxLevel && Row->LevelStats.IsValidIndex(NextLevelIdx))
				{
					UIData.NextLevelStat = Row->LevelStats[NextLevelIdx].FixedDefensePower;
				}
			}
		}
	}

	return UIData;
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

	// 강화 시도
	bool bSuccess = EquipComp->TryUpgrade(EquipmentType, MaxUpgradeLevel);

	if (bSuccess)
	{
		// 성공 델리게이트 발송
		OnUpgradeSuccess.Broadcast(EquipmentType);

		FString TypeName = (EquipmentType == ET3EquipmentType::Weapon) ? TEXT("무기") : TEXT("방어구");
		float NewStat = (EquipmentType == ET3EquipmentType::Weapon)
			? EquipComp->GetCurrentAttackPower()
			: EquipComp->GetCurrentDefensePower();

		UE_LOG(LogDesecration, Log, TEXT("[UpgradeStation] %s 강화 성공! 새 스탯: %.1f"), *TypeName, NewStat);
	}
	else
	{
		// 실패 델리게이트 발송
		OnUpgradeFailed.Broadcast(FText::FromString(TEXT("강화에 실패했습니다. (최대 레벨이거나 데이터 없음)")));

		UE_LOG(LogDesecration, Warning, TEXT("[UpgradeStation] 강화 실패"));
	}

	return bSuccess;
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
	// 임시로 MaxUpgradeLevel 덮어쓰기
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
