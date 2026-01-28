// T3Paladin_SkillComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "T3Paladin_SkillComponent.generated.h"

class AT3SwordWaveProjectile;
class UAnimMontage;


USTRUCT(BlueprintType)
struct FSkillData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Skill")
    TSubclassOf<class AT3SwordWaveProjectile> ProjectileClass;

    UPROPERTY(EditAnywhere, Category = "Skill")
    float Damage = 50.f;

    UPROPERTY(EditAnywhere, Category = "Skill")
    float Speed = 1500.f;

    UPROPERTY(EditAnywhere, Category = "Skill")
    UAnimMontage* SkillMontage = nullptr;;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DESECRATION_API UT3Paladin_SkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
    
    UT3Paladin_SkillComponent();

    // 스킬 실행 함수
    UFUNCTION(BlueprintCallable, Category = "Skill")
    void ExecuteSwordWave();

    // 실제 투사체 스폰 (AnimNotify에서 호출될 용도)
    void SpawnSwordWaveProjectile();

private:
    UPROPERTY(EditAnywhere, Category = "Skill Data")
    FSkillData SwordWaveData;
		
};
