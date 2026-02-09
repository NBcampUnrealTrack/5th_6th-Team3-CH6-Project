// T3SkillComponentBase.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "T3SkillComponentBase.generated.h"


USTRUCT(BlueprintType)
struct FSkillData
{
    GENERATED_BODY()

    // --- 공통 데이터 ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    UTexture2D* SkillIcon = nullptr;

    UPROPERTY(EditAnywhere, Category = "Common")
    UAnimMontage* SkillMontage = nullptr;

    UPROPERTY(EditAnywhere, Category = "Common")
    float ManaCost = 10.f;

    UPROPERTY(EditAnywhere, Category = "Common")
    float Cooldown = 3.f;

    // --- 공격 데이터 ---
    UPROPERTY(EditAnywhere, Category = "Combat")
    float DamageMultiflier = 1.f;

    // --- 투사체 데이터 (필요한 스킬만 입력) ---
    UPROPERTY(EditAnywhere, Category = "Projectile")
    TSubclassOf<class AActor> ProjectileClass;

    UPROPERTY(EditAnywhere, Category = "Projectile")
    float ProjectileSpeed = 1500.f;

    // 런타임 데이터
    float LastActivatedTime = -100.f;
};

UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3SkillComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:
    // 슬롯 1, 2에 장착된 스킬 번호 -> 스킬 갈아끼울때 여기만 수정하면 된다.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Slot_1_SkillID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 Slot_2_SkillID = 1;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bUsingSkill = false;

    // 슬롯 번호를 입력받아 스킬 실행
    virtual void ExecuteSkillNotify(int32 Index);
    virtual void ExecuteSkill(int32 SkillSlot);

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SetSkillSlot(int32 SlotNumber, int32 NewSkillID);

protected:

    virtual void BeginPlay() override;

    UPROPERTY()
    class AT3CharacterBase* OwnerChar;

    UPROPERTY()
    class UT3CombatComponent* Combat;

    // 스킬 사용 가능여부 체크 위한 쿨타임 마나 계산
    virtual bool CanExecuteSkill(FSkillData& Data);
    void StartCooldown(FSkillData& Data);
};


