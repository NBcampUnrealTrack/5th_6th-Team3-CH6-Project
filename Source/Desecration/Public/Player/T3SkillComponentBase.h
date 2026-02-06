// T3SkillComponentBase.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "T3SkillComponentBase.generated.h"


// 스킬 데이터 테이블 구조체
#pragma region SkillDataTable
USTRUCT(BlueprintType)
struct FSkillAttributes : public FTableRowBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "General")
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    TSoftObjectPtr<UTexture2D> SkillIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    float ManaCost = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    float Cooldown = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    float DamageMultiplier = 1.0f; // 1.0 = 100% (캐릭터 공격력 비례)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
    float StunAmount = 0.f;

    // 패시브 및 특수 효과를 위한 섹션
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special")
    bool bIsPassive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Special")
    FGameplayTag SpecialEffectTag;
   
};

#pragma endregion

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3SkillComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:
    // 슬롯 1, 2에 장착된 스킬 번호
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Slot_1_SkillID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Slot_2_SkillID = 1;

    // 슬롯 번호를 입력받아 스킬 실행
    virtual void ExecuteSkillNotify(int32 Index);
    virtual void ExecuteSkill(int32 SkillSlot);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SetSkillSlot(int32 SlotNumber, int32 NewSkillID);

    // 데이터 테이블에서 스킬 정보를 가져오는 함수
    FSkillAttributes* GetSkillRow(int32 SkillID);

protected:
    UPROPERTY()
    TObjectPtr<UDataTable> MySkillTable;
};


