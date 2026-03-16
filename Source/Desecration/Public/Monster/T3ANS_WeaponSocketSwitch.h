// T3ANS_WeaponSocketSwitch.h
// 몽타주 구간 동안 무기를 대체 소켓으로 부착 — 애님팩별 그립 보정용
// 사용법: 팩 2 몽타주에 이 노티파이를 얹으면 Alternative 소켓으로 전환, 끝나면 복귀

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "T3ANS_WeaponSocketSwitch.generated.h"

class UT3BossWeaponComponent;

UCLASS(DisplayName = "Boss Weapon Socket Switch")
class DESECRATION_API UT3ANS_WeaponSocketSwitch : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
