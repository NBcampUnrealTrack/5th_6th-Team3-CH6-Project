// T3ANS_BossInvulnerable.h
// 몽타주 구간 동안 보스에 i-frame(무적) 부여 — 회피(롤) 5~45f 같은 무적 윈도우용
// NotifyBegin에서 Boss.State.Invulnerable 태그 ON, NotifyEnd에서 OFF
// (몽타주 캔슬되어도 NotifyEnd가 자동 호출되므로 태그 누수 없음)

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "T3ANS_BossInvulnerable.generated.h"

UCLASS(DisplayName = "Boss Invulnerable (i-Frame)")
class DESECRATION_API UT3ANS_BossInvulnerable : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
