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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common")
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSkillSlotUpdated, int32, SlotIndex, int32, SkillID, const FSkillData&, SkillData);


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3SkillComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:
    // 슬롯 1, 2에 장착된 스킬 번호 -> 스킬 갈아끼울때 여기만 수정하면 된다. 기본 빈스킬.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 CurrentSkillSlot = 1; // 메인 슬롯

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    int32 NextSkillSlot = 2; // 서브 슬롯

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bUsingSkill = false;

    // 몽타주 종료 콜백 함수
    UFUNCTION()
    void OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    //  == UI팀 전용
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillSlotUpdated OnSkillSlotUpdated;
    
    // UI팀이 현재 장착된 모든 스킬 정보를 한 번에 가져가고 싶을 때
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void GetCurrentEquippedSkills(FSkillData& OutSlot1, FSkillData& OutSlot2)
    {
        OutSlot1 = *GetSkillDataByID(CurrentSkillSlot);
        OutSlot2 = *GetSkillDataByID(NextSkillSlot);
    }
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    UTexture2D* GetSkillIconByID(int32 SkillID)
    {
        FSkillData* Data = GetSkillDataByID(SkillID);
        return (Data) ? Data->SkillIcon : nullptr;
    }

    UFUNCTION(BlueprintCallable, Category = "Skill")
    void SetSkillSlot( int32 NewSkillID, bool bIsEquip);

    UFUNCTION(BlueprintCallable, Category = "Skill")

    int32 GetSkillIDBySlotIndex(int32 Index) const;

    virtual FSkillData* GetSkillDataByID(int32 SkillID) { return nullptr; }
    


    // 슬롯 번호를 입력받아 스킬 실행
    virtual void ExecuteSkillNotify(int32 Index);
    virtual void ExecuteSkill(int32 SkillSlot);

    // 스킬 스왑 함수 (CombatComponent에서 호출)
    void SwapSkills();


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


