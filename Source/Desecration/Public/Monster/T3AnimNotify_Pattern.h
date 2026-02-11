// T3AnimNotify_Pattern.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "T3AnimNotify_Pattern.generated.h"

// ============================================================
// 패턴 노티파이 — 몽타주에 배치하면 자동으로 HandlePatternNotify 호출
// AnimBP 연결 불필요. 몽타주 에디터에서 이름만 설정하면 됨.
// ============================================================

UCLASS(BlueprintType, meta = (DisplayName = "Pattern Notify"))
class DESECRATION_API UT3AnimNotify_Pattern : public UAnimNotify
{
	GENERATED_BODY()

public:
	// 노티파이 이름 (Slow, Fast, Normal, Step_1, AttackStart, AttackEnd 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pattern")
	FName PatternNotifyName;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
