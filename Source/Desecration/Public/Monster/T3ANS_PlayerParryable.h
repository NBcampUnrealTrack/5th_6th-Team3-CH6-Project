// T3ANS_PlayerParryable.h
// 몽타주 구간 동안 플레이어 패링 가능 윈도우 활성화
// 사용법: 공격 몽타주에서 패링 가능 구간에 이 노티파이를 배치

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "T3ANS_PlayerParryable.generated.h"

UCLASS(DisplayName = "Boss PlayerParryable Window")
class DESECRATION_API UT3ANS_PlayerParryable : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
