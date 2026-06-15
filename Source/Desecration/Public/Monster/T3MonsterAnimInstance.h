// T3MonsterAnimInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "T3MonsterAnimInstance.generated.h"

struct FMontageBlendSettings;

/**
 * 몬스터 공용 AnimInstance.
 * UAnimInstance::Montage_PlayInternal은 BP K2Node_PlayMontage(UPlayMontageCallbackProxy)와
 * C++ Montage_Play / Montage_PlayWithBlendIn 등 모든 진입이 최종적으로 거치는 단일 virtual 진입점이다.
 * 여기서 Owner 몬스터의 CurrentAttackRate를 곱해 장신구 등 외부 슬로우를 일괄 반영한다.
 */
UCLASS()
class DESECRATION_API UT3MonsterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	virtual float Montage_PlayInternal(
		UAnimMontage* MontageToPlay,
		const FMontageBlendSettings& BlendInSettings,
		float InPlayRate = 1.f,
		EMontagePlayReturnType ReturnValueType = EMontagePlayReturnType::MontageLength,
		float InTimeToStartMontageAt = 0.f,
		bool bStopAllMontages = true) override;
};
