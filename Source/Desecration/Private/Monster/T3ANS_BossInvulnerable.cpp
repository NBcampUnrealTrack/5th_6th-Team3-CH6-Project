#include "Monster/T3ANS_BossInvulnerable.h"
#include "Monster/T3MidBossMonster.h"
#include "Desecration.h"

FString UT3ANS_BossInvulnerable::GetNotifyName_Implementation() const
{
	return TEXT("Boss i-Frame");
}

void UT3ANS_BossInvulnerable::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	AT3MidBossMonster* Boss = Cast<AT3MidBossMonster>(Owner);
	if (!Boss)
	{
		return;
	}

	Boss->AddStateTag(TAG_Boss_State_Invulnerable);

	UE_LOG(LogDesecration, Verbose, TEXT("T3_MidBoss: i-Frame ON (몽타주:%s, Duration:%.2f)"),
		Animation ? *Animation->GetName() : TEXT("nullptr"), TotalDuration);
}

void UT3ANS_BossInvulnerable::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	AT3MidBossMonster* Boss = Cast<AT3MidBossMonster>(Owner);
	if (!Boss)
	{
		return;
	}

	Boss->RemoveStateTag(TAG_Boss_State_Invulnerable);

	UE_LOG(LogDesecration, Verbose, TEXT("T3_MidBoss: i-Frame OFF (몽타주:%s)"),
		Animation ? *Animation->GetName() : TEXT("nullptr"));
}
