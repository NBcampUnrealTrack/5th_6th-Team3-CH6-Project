// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Equipment/T3EquipmentTypes.h"
#include "T3UpgradeStation.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UInputAction;
class UInputMappingContext;
class AT3CharacterBase;
class UT3PlayerEquipmentComponent;

// ============================================================================
// IT3Interactable 인터페이스
// 상호작용 가능한 오브젝트가 구현하는 인터페이스
// ============================================================================
UINTERFACE(MinimalAPI, Blueprintable)
class UT3Interactable : public UInterface
{
	GENERATED_BODY()
};

class IT3Interactable
{
	GENERATED_BODY()

public:
	// 상호작용 실행
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AT3CharacterBase* Interactor);

	// 상호작용 가능 여부
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AT3CharacterBase* Interactor) const;

	// 상호작용 프롬프트 텍스트
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt() const;
};

// ============================================================================
// FT3UpgradeUIData - UI에 표시할 장비 정보 구조체
// ============================================================================
USTRUCT(BlueprintType)
struct FT3UpgradeUIData
{
	GENERATED_BODY()

	// 장비 ID (RowName, 코드용)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FName ItemID = NAME_None;

	// 장비 표시 이름 (UI용, 현지화 가능)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FText DisplayName;

	// 장비 아이콘
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// 장비 타입
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	ET3EquipmentType EquipmentType = ET3EquipmentType::Weapon;

	// 현재 강화 레벨
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	int32 CurrentLevel = 0;

	// 현재 스탯 (공격력 or 방어력)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	float CurrentStat = 0.0f;

	// 다음 레벨 스탯 (강화 미리보기)
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	float NextLevelStat = 0.0f;

	// 강화 가능 여부
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bCanUpgrade = false;

	// 최대 레벨 도달 여부
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsMaxLevel = false;

	// 장비 장착 여부
	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	bool bIsEquipped = false;
};

// ============================================================================
// 델리게이트 선언 (UI 갱신용)
// ============================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeUIOpened);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeUIClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeSuccess, ET3EquipmentType, UpgradedType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeFailed, FText, FailReason);

// ============================================================================
// AT3UpgradeStation - 강화 스테이션 액터
// ============================================================================
UCLASS()
class DESECRATION_API AT3UpgradeStation : public AActor, public IT3Interactable
{
	GENERATED_BODY()

public:
	AT3UpgradeStation();

protected:
	virtual void BeginPlay() override;

	// ========================================================================
	// 컴포넌트
	// ========================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	// ========================================================================
	// 설정
	// ========================================================================
	// 상호작용 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float InteractionRadius = 200.0f;

	// 강화 최대 레벨 제한
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 MaxUpgradeLevel = 10;

	// ========================================================================
	// 강화석 아이콘 (에디터에서 설정)
	// ========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons")
	TObjectPtr<UTexture2D> NormalStoneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons")
	TObjectPtr<UTexture2D> RareStoneIcon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|StoneIcons")
	TObjectPtr<UTexture2D> EpicStoneIcon;

	// ========================================================================
	// [임시] 강화석 보유량 - 추후 플레이어 측으로 이관 예정
	// ========================================================================
#pragma region TEMP_CURRENCY

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Currency")
	int32 NormalStoneCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Currency")
	int32 RareStoneCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|Currency")
	int32 EpicStoneCount = 3;

#pragma endregion TEMP_CURRENCY

	// ========================================================================
	// 상태
	// ========================================================================
	// 현재 범위 내 플레이어
	UPROPERTY(BlueprintReadOnly, Category = "State")
	TObjectPtr<AT3CharacterBase> PlayerInRange;

	// UI 열림 상태
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsUpgradeUIOpen = false;

	// 위젯 클래스 (에디터에서 WBP_UpgradeUI 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> UpgradeWidgetClass;

	// 현재 생성된 위젯 인스턴스
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	TObjectPtr<UUserWidget> UpgradeWidgetInstance;

	// ========================================================================
	// 오버랩 이벤트 (Core)
	// ========================================================================
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	// ========================================================================
	// IT3Interactable 인터페이스 구현 (Core)
	// ========================================================================
	virtual void Interact_Implementation(AT3CharacterBase* Interactor) override;
	virtual bool CanInteract_Implementation(AT3CharacterBase* Interactor) const override;
	virtual FText GetInteractionPrompt_Implementation() const override;

	// ========================================================================
	// UI 델리게이트 (Blueprint에서 바인딩)
	// ========================================================================
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeUIOpened OnUpgradeUIOpened;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeUIClosed OnUpgradeUIClosed;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeSuccess OnUpgradeSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnUpgradeFailed OnUpgradeFailed;

	// ========================================================================
	// UI 제어 함수 (Core - Widget Blueprint에서 호출)
	// ========================================================================
	// UI 열기/닫기
	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	void OpenUpgradeUI();

	UFUNCTION(BlueprintCallable, Category = "Upgrade|UI")
	void CloseUpgradeUI();

	UFUNCTION(BlueprintPure, Category = "Upgrade|UI")
	bool IsUpgradeUIOpen() const { return bIsUpgradeUIOpen; }

	// ========================================================================
	// 장비 정보 조회 (Core - Widget Blueprint에서 호출)
	// ========================================================================
	// 무기 정보 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Data")
	FT3UpgradeUIData GetWeaponUIData() const;

	// 방어구 정보 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Data")
	FT3UpgradeUIData GetArmorUIData() const;

	// 특정 타입 장비 정보 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Data")
	FT3UpgradeUIData GetEquipmentUIData(ET3EquipmentType EquipmentType) const;

	// ========================================================================
	// 강화석 조회 (Core - Widget Blueprint에서 직접 호출)
	// 강화석은 장비별 데이터가 아닌 스테이션 공유 자원이므로
	// UIData가 아닌 UpgradeStation 레퍼런스에서 직접 접근
	// ========================================================================

	// 특정 등급 강화석 보유량 조회
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	int32 GetStoneCount(ET3UpgradeStoneGrade Grade) const;

	// 특정 등급 강화석 아이콘 조회
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	UTexture2D* GetStoneIcon(ET3UpgradeStoneGrade Grade) const;

	// 현재 장비 레벨에 사용 가능한 강화석 등급 목록 조회
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Currency")
	TArray<ET3UpgradeStoneGrade> GetAvailableStones(int32 CurrentEquipmentLevel) const;

	// 특정 등급 강화석이 해당 레벨에 사용 가능한지
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	bool CanUseStone(ET3UpgradeStoneGrade Grade, int32 CurrentEquipmentLevel) const;

	// 강화석 최대 적용 레벨 조회 (Normal→3, Rare→5, Epic→7)
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	static int32 GetMaxLevelForStone(ET3UpgradeStoneGrade Grade);

	// 현재 장비 레벨에서 자동 선택될 강화석 등급 조회 (UI 표시용)
	// 사용 가능한 강화석이 없으면 false 반환
	UFUNCTION(BlueprintPure, Category = "Upgrade|Currency")
	bool GetNextStoneGrade(int32 CurrentEquipmentLevel, ET3UpgradeStoneGrade& OutGrade) const;

	// ========================================================================
	// 강화 실행 (Core - Widget Blueprint에서 호출)
	// 사용 가능한 강화석 중 최하급부터 자동 소모
	// ========================================================================
	// 무기 강화 (버튼 클릭 시)
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Action")
	bool UpgradeWeapon();

	// 방어구 강화 (버튼 클릭 시)
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Action")
	bool UpgradeArmor();

	// 특정 타입 장비 강화
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Action")
	bool UpgradeEquipment(ET3EquipmentType EquipmentType);

private:
	// 현재 레벨에서 사용 가능한 최하급 강화석 자동 선택
	// 사용 가능한 등급이 없으면 false 반환
	bool SelectLowestAvailableStone(int32 CurrentEquipmentLevel, ET3UpgradeStoneGrade& OutGrade) const;

	// 강화석 1개 차감
	void ConsumeStone(ET3UpgradeStoneGrade Grade);

public:

	// ========================================================================
	// 유틸리티 (Core)
	// ========================================================================
	// 플레이어가 범위 내에 있는지
	UFUNCTION(BlueprintPure, Category = "Upgrade|Utility")
	bool IsPlayerInRange() const { return PlayerInRange != nullptr; }

	// 범위 내 플레이어 반환
	UFUNCTION(BlueprintPure, Category = "Upgrade|Utility")
	AT3CharacterBase* GetPlayerInRange() const { return PlayerInRange; }

	// EquipmentComponent 조회 헬퍼
	UFUNCTION(BlueprintPure, Category = "Upgrade|Utility")
	UT3PlayerEquipmentComponent* GetPlayerEquipmentComponent() const;

// ============================================================================
// [TEST] 테스트용 코드 - 정식 Interaction 시스템 연동 후 제거 예정
// ============================================================================
#pragma region TEST_CODE

protected:
	// [TEST] F키 입력 처리 (Character 팀의 Interaction 시스템으로 대체 예정)
	void HandleInteractInput();
	void BindInputToPlayer(APlayerController* PC);
	void UnbindInputFromPlayer(APlayerController* PC);

public:
	// [TEST] 레거시 강화 함수 (MaxLevel 파라미터 버전) - 삭제 예정
	UFUNCTION(BlueprintCallable, Category = "Upgrade|Test", meta = (DeprecatedFunction, DeprecationMessage = "Use UpgradeWeapon() instead"))
	bool TryUpgradeWeapon(int32 MaxLevel = 10);

	UFUNCTION(BlueprintCallable, Category = "Upgrade|Test", meta = (DeprecatedFunction, DeprecationMessage = "Use UpgradeArmor() instead"))
	bool TryUpgradeArmor(int32 MaxLevel = 10);

#pragma endregion TEST_CODE
};
