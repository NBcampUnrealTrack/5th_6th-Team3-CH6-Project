// T3Taoist_SkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "Player/T3SkillComponentBase.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "T3Taoist_SkillComponent.generated.h"

class AT3TalismanProjectile;
class UAnimMontage;

UENUM(BlueprintType)
enum class EActionType : uint8
{
    None,
    AttackAnim,
    StrongWindAnim,
    SummonTigerAnim,
    AttackSpawn,
    StrongWindSpawn,
    SummonTigerSpawn
};

UCLASS()
class DESECRATION_API UT3Taoist_SkillComponent : public UT3SkillComponentBase
{
	GENERATED_BODY()
	
public:
    UT3Taoist_SkillComponent();

    virtual void ExecuteSkill(int32 SkilSolt) override;
    virtual void ExecuteSkillNotify(int32 Index) override;
    virtual void CancelCurrentSkill() override;

    virtual FSkillData* GetSkillDataByID(int32 SkillID) override;


    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData EmptySkillData;

    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData StrongWindData;

    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData TaoistDodgeData;

    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData ShadowCloneData;

    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData SummonTigerData;

protected:

    virtual void BeginPlay() override;
    void OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted);


    // 부채 무기를 미리 보관해 둘 포인터변수
    UPROPERTY()
    class AT3FanWeapon* FanWeapon;

    // 초기화 함수
    void InitializeFanWeapon();



    // ===== 기본 공격 (부적 날리기)
  
    // 에디터에서 할당할 부적 블루프린트 클래스
    UPROPERTY(EditAnywhere, Category = "Combat|Skill")
    TSubclassOf<class AT3TalismanProjectile> TalismanClass;

    // 부적 스폰 로직
    void SpawnTalisman();


    // ===== 패시브 스킬 (회피 후 다음 공격 or 스킬 1회 30% 강화)
    
    // 회피 후 다음 공격 강화 여부
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Skill|Passive")
    bool bIsSpiritualEmpowered = false;

    // 강화 효과 재발동 쿨타임 (10초)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Passive")
    float EmpowermentCooldown = 10.f;
    FTimerHandle EmpowermentTimerHandle;

    // 쿨타임 중인지 확인 (회피 시 체크용)
    bool bIsOnCooldown = false;
    
public:
    // 외부에서 강화 상태를 변경할 때 사용
    UFUNCTION(BlueprintCallable, Category = "Skill|Passive")
    void SetEmpowermentState(bool bEnabled);

    // 쿨타임 종료 콜백
    void ResetEmpowermentCooldown();

    // 강화 상태인지 확인하는 Getter
    bool IsEmpowered() const { return bIsSpiritualEmpowered; }
    

    // ===== 1스킬 : 장풍

protected:
    void ExecuteStrongWind();
    void SpawnStrongWind();
    UPROPERTY(EditAnywhere, Category = "Combat|Skill")
    float StrongWindSpawnDistance;


    // ====== 2스킬 : 축지법

protected:
    UPROPERTY(EditAnywhere, Category = "Combat|Effect")
    TObjectPtr<UNiagaraSystem> GhostTrailSystem;

    UPROPERTY()
    TObjectPtr<UNiagaraComponent> GhostTrailComponent;

    UFUNCTION(BlueprintCallable)
    void SetGhostTrailActive(bool bActive);

    UFUNCTION(BlueprintCallable)
    void ExecuteTaoistDodge();

    UFUNCTION(BlueprintImplementableEvent)
    void OnTaoistDodgeTriggered();


    // =======  3스킬 : 분신술

    protected:
        UFUNCTION(BlueprintCallable)
        void SpawnShadowClones();

        // 생성된 분신들을 담아둘 배열
        UPROPERTY()
        TArray<TObjectPtr<class AT3TaoistClone>> ActiveClones;

        // 블루프린트에서 설정할 분신 클래스
        UPROPERTY(EditAnywhere, Category = "Skills|Clone")
        TSubclassOf<class AT3TaoistClone> CloneClass;

        void NotifyClonesAction(EActionType ActionType);

        void OnCloneRemoved(class AT3TaoistClone* ExClone);


        // ========= 4스킬 : 호랑이 소환술

   protected:
       UFUNCTION(BlueprintCallable)
       void SummonTiger();

       UPROPERTY(EditAnywhere, Category = "Skills|Tiger")
       TSubclassOf<class AT3TigerAttack> TigerClass;
};
