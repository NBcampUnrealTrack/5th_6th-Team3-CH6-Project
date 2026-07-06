// T3Poisonable.h
// 독 데미지를 받을 수 있는 오브젝트(캐릭터, 몬스터 모두)에 구현하는 인터페이스.
// 스택이 MaxPoisonStack에 도달하면 독 효과가 활성화되어 일정 시간 동안 초당 HP%만큼 데미지를 입힌다.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "T3Poisonable.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * 독 VFX/SFX 에셋 묶음 구조체.
 * IT3Poisonable을 구현하는 모든 클래스가 이 구조체 하나만 UPROPERTY로 가진다.
 * 에디터(BP 또는 CDO)에서 에셋을 한 곳에서 설정한다.
 */
USTRUCT(BlueprintType)
struct FT3PoisonFXConfig
{
	GENERATED_BODY()

	// 독 활성화 순간 재생할 이펙트 (아우라/연기 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poison|FX")
	TObjectPtr<UNiagaraSystem> ActivateEffect;

	// 독 활성화 순간 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poison|FX")
	TObjectPtr<USoundBase> ActivateSound;

	// 독 틱 데미지마다 재생할 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poison|FX")
	TObjectPtr<UNiagaraSystem> TickEffect;

	// 독 틱 데미지마다 재생할 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Poison|FX")
	TObjectPtr<USoundBase> TickSound;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UT3Poisonable : public UInterface
{
	GENERATED_BODY()
};

class DESECRATION_API IT3Poisonable
{
	GENERATED_BODY()

public:
	/** 독 스택을 Stacks만큼 추가한다. MaxPoisonStack 도달 시 독이 활성화된다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Poison")
	void ApplyPoisonStack(int32 Stacks);

	/** 현재 독 상태(활성화된 DoT) 여부를 반환한다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Poison")
	bool IsPoisoned() const;
};
