// T3ANS_WeaponAura.h
// 몽타주 구간 동안 무기 오라 이펙트 활성화 — Begin에서 켜고 End에서 끔
// 노티파이마다 다른 Niagara 에셋 지정 가능

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "T3ANS_WeaponAura.generated.h"

class UNiagaraSystem;

UCLASS(DisplayName = "Boss Weapon Aura")
class DESECRATION_API UT3ANS_WeaponAura : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// 이 노티파이에서 사용할 Niagara 에셋 (노티파이마다 다르게 지정 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aura")
	TObjectPtr<UNiagaraSystem> AuraSystem;

	virtual FString GetNotifyName_Implementation() const override;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
