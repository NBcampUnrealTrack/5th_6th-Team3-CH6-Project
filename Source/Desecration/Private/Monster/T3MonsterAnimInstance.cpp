// T3MonsterAnimInstance.cpp

#include "Monster/T3MonsterAnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Monster/T3MonsterBase.h"

float UT3MonsterAnimInstance::Montage_PlayInternal(
	UAnimMontage* MontageToPlay,
	const FMontageBlendSettings& BlendInSettings,
	float InPlayRate,
	EMontagePlayReturnType ReturnValueType,
	float InTimeToStartMontageAt,
	bool bStopAllMontages)
{
	float FinalRate = InPlayRate;

	// Owner 몬스터의 현재 공격 배율을 곱해 슬로우 등 외부 배율을 일괄 반영
	const AT3MonsterBase* Owner = Cast<AT3MonsterBase>(TryGetPawnOwner());
	if (Owner)
	{
		FinalRate *= Owner->GetCurrentAttackRate();
	}

	UE_LOG(LogTemp, Warning, TEXT("T3_AnimInst[%s]: Montage_PlayInternal %s InRate=%.2f -> Final=%.2f (Owner=%s, AttackRate=%.2f)"),
		*GetName(),
		MontageToPlay ? *MontageToPlay->GetName() : TEXT("nullptr"),
		InPlayRate, FinalRate,
		Owner ? *Owner->GetName() : TEXT("nullptr"),
		Owner ? Owner->GetCurrentAttackRate() : -1.f);

	return Super::Montage_PlayInternal(
		MontageToPlay, BlendInSettings, FinalRate, ReturnValueType, InTimeToStartMontageAt, bStopAllMontages);
}
