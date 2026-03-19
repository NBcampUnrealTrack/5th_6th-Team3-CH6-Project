// T3CharacterDataAsset.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "T3PlayerInputState.h"
#include "Player/T3SkillComponentBase.h"
#include "T3CharacterDataAsset.generated.h"

// 캐릭터 데미지 타입
UENUM(BlueprintType)
enum class EDamageType : uint8
{
    Physical    UMETA(DisplayName = "Physical"),
    Magical     UMETA(DisplayName = "Magical")
};

// 무기 장착 위치 정의
UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
    RightHand      UMETA(DisplayName = "Right Hand"),
    LeftHand         UMETA(DisplayName = "Left Hand"),
    Back               UMETA(DisplayName = "Back")
};

USTRUCT(BlueprintType)
struct FWeaponEquipInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Equipment")
    TSubclassOf<class AT3WeaponBase> WeaponClass;

    UPROPERTY(EditAnywhere, Category = "Equipment")
    FName SocketName;

    // 소켓 부착 후 미세 조정을 위한 Transform
    UPROPERTY(EditAnywhere, Category = "Equipment")
    FTransform RelativeTransform = FTransform::Identity;
};

UCLASS()
class DESECRATION_API UT3CharacterDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    ECharacterClass CharacterClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    EDamageType PrimaryDamageType; 
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    TSubclassOf<UT3SkillComponentBase> SkillComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
    TObjectPtr<UDataTable> SkillDataTable; // 이 직업 전용 스킬 테이블

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TObjectPtr<USkeletalMesh> CharacterMesh;

    // 직업별 장착 무기 맵 (슬롯 -> 장착 정보)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    TMap<EEquipSlot, FWeaponEquipInfo> WeaponMap;


    // 기본 스탯
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
    float MaxHealth = 150.f; 
};
