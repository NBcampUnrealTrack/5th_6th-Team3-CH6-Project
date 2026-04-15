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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common")
    FText SkillName = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common")
    FText Description = FText::GetEmpty();
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    UTexture2D* SkillIcon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Common")
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
// SkillID(int32), CooldownTime(float)을 전달
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillCooldownStarted, int32, SkillID, float, CooldownTime);

// 게이지 업데이트 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnResourceChanged, float, CurrentAmount);

// 스킬 활성화 여부 전달 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillUnlockStateChanged, int32, SkillID, bool, bIsUnlocked);

// 스킬 장착 여부 전달 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillEquipStateChanged, int32, SkillID, bool, bIsEquipped);


UCLASS( Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3SkillComponentBase : public UActorComponent
{
    GENERATED_BODY()

public:
    // 최대 장착 가능 스킬 수
    static constexpr int32 MAX_SKILL_SLOTS = 4;

    // 장착된 스킬 ID 배열 (최대 4개). [0]이 항상 현재 활성 슬롯.
    // SwapSkills() 호출 시 [0]이 뒤로 순환: [A,B,C,D] → [B,C,D,A]
    UPROPERTY(BlueprintReadOnly, Category = "Skill")
    TArray<int32> EquippedSkillIDs;

    // BP 호환 getter - HUD 위젯에서 현재/다음 슬롯을 가져올 때 사용
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    int32 GetCurrentSkillSlot() const { return EquippedSkillIDs.IsValidIndex(0) ? EquippedSkillIDs[0] : 0; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    int32 GetNextSkillSlot() const { return EquippedSkillIDs.IsValidIndex(1) ? EquippedSkillIDs[1] : 0; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill")
    bool bUsingSkill = false;

    // 몽타주 종료 콜백 함수
    UFUNCTION()
    void OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted);
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void BasicAttackCount();

    //  == UI팀 전용
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnSkillSlotUpdated OnSkillSlotUpdated;
    
    // UI팀이 현재 장착된 모든 스킬 정보를 한 번에 가져가고 싶을 때 (HUD 슬롯 1=현재, 슬롯 2=다음)
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void GetCurrentEquippedSkills(FSkillData& OutSlot1, FSkillData& OutSlot2)
    {
        FSkillData* D1 = GetSkillDataByID(GetCurrentSkillSlot());
        FSkillData* D2 = GetSkillDataByID(GetNextSkillSlot());
        OutSlot1 = D1 ? *D1 : FSkillData();
        OutSlot2 = D2 ? *D2 : FSkillData();
    }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
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
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    FText GetSkillNameByID(int32 SkillID)
    {
        FSkillData* Data = GetSkillDataByID(SkillID);
        
        if (Data)
        {
            return Data->SkillName;
        }
        else
        {
            return FText::GetEmpty();
        }
    }
    
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Skill")
    FText GetSkillInfoByID(int32 SkillID)
    {
        FSkillData* Data = GetSkillDataByID(SkillID);
        
        if (Data)
        {
            return Data->Description;
        }
        else
        {
            return FText::GetEmpty();
        }
    }
    
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void CancelCurrentSkill();

    // 슬롯 번호를 입력받아 스킬 실행
    virtual void ExecuteSkillNotify(int32 Index);
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void ExecuteSkill(int32 SkillSlot);
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void ExecuteSkill_Completed(int32 SkillSlot);

    // 스킬 스왑 함수 (CombatComponent에서 호출)
    void SwapSkills();

    void StartCooldown(int32 SkillID, FSkillData& Data);

    UPROPERTY(BlueprintAssignable, Category = "Events | UI")
    FOnSkillCooldownStarted OnSkillCooldownStarted;

    // 남은 쿨다운 시간과 비율을 가져오는 함수
    float GetRemainingCooldown(int32 SkillID);
    float GetCooldownRemainingRatio(int32 SkillID);

    // 신성 게이지 등 캐릭터의 게이지를 추가하는 함수 (직업별 상이)
    UFUNCTION(BlueprintCallable, Category = "Skill")
    virtual void AddResource(float Amount) {  }

    // 게이지 델리게이트
    UPROPERTY(BlueprintAssignable, Category = "Gauge")
    FOnResourceChanged OnResourceChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
    float AttackSpeedMultiplier = 1.0f;

protected:

    virtual void BeginPlay() override;

    // HUD용 슬롯 업데이트 브로드캐스트 (슬롯 인덱스 1=현재, 2=다음 고정)
    void BroadcastSlotUpdated();

    UPROPERTY()
    class AT3CharacterBase* OwnerChar;

    UPROPERTY()
    class UT3CombatComponent* Combat;

    // 스킬 사용 가능여부 체크 위한 쿨타임 마나 계산
    virtual bool CanExecuteSkill(FSkillData& Data);



public:

    // ===== 사운드 

    void PlaySkillEffectSound(USoundBase* Sound, float Volume = 1.0f);

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* AttackVoice;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* BlockVoice;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill1Voice;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill2Voice;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill3Voice;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill4Voice;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* AttackSound;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* BlockSound;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill1Sound;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill2Sound;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill3Sound;

    UPROPERTY(EditAnywhere, Category = "Sound")
    class USoundBase* Skill4Sound;


    // === 스킬 활성화 여부
    public:

        //  스킬 해금 상태 확인
        UFUNCTION(BlueprintCallable, Category = "Skill")
        bool IsSkillUnlocked(int32 SkillID) const;

        // 스킬 해금/잠금 설정
        UFUNCTION(BlueprintCallable, Category = "Skill")
        void SetSkillUnlockState(int32 SkillID, bool bUnlock);

        // 모든 스킬의 해금 상태를 전달
        UPROPERTY(BlueprintAssignable, Category = "Events | UI")
        FOnSkillUnlockStateChanged OnSkillUnlockStateChanged;

        // 위젯 바인딩 후 호출 - 현재 모든 스킬 해금 상태를 브로드캐스트
        UFUNCTION(BlueprintCallable, Category = "Skill")
        void BroadcastCurrentUnlockStates();
    
    // 스킬 ID와 해금 여부를 매핑
    UPROPERTY(EditAnywhere, Category = "Skill | Data")
    TMap<int32, bool> SkillUnlockStates;


    // === 스킬 장착 여부

    public:
        UPROPERTY(BlueprintAssignable, Category = "Events")
        FOnSkillEquipStateChanged OnSkillEquipStateChanged;

        // 특정 스킬의 장착 여부를 확인하는 함수
        UFUNCTION(BlueprintCallable, Category = "Skill")
        bool IsSkillEquipped(int32 SkillID) const;

protected:
    // 장착 상태를 관리할 맵 (ID, 장착여부)
    UPROPERTY()
    TMap<int32, bool> SkillEquipStates;

    // 초기화 시 1번만 장착되게 설정
    void InitializeDefaultSlots();
};


