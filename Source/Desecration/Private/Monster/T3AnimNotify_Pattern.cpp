#include "Monster/T3AnimNotify_Pattern.h"
#include "Monster/T3MidBossMonster.h"
#include "Desecration.h"

void UT3AnimNotify_Pattern::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	AT3MidBossMonster* MidBoss = Cast<AT3MidBossMonster>(Owner);
	if (MidBoss)
	{
		MidBoss->HandlePatternNotify(PatternNotifyName);
	}
}

FString UT3AnimNotify_Pattern::GetNotifyName_Implementation() const
{
	// 몽타주 에디터 타임라인에 표시되는 이름
	if (PatternNotifyName.IsNone())
	{
		return TEXT("Pattern Notify");
	}
	return PatternNotifyName.ToString();
}
