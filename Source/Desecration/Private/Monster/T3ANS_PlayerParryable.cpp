#include "Monster/T3ANS_PlayerParryable.h"
#include "Monster/T3MidBossMonster.h"
#include "Desecration.h"

FString UT3ANS_PlayerParryable::GetNotifyName_Implementation() const
{
	return TEXT("PlayerParryable");
}

void UT3ANS_PlayerParryable::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) { return; }
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) { return; }

	if (AT3MidBossMonster* MidBoss = Cast<AT3MidBossMonster>(Owner))
	{
		MidBoss->AddStateTag(TAG_Boss_State_PlayerParryable);
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: PlayerParryable 윈도우 오픈 (ANS, %.2f초)"), TotalDuration);
	}
}

void UT3ANS_PlayerParryable::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) { return; }
	AActor* Owner = MeshComp->GetOwner();
	if (!Owner) { return; }

	if (AT3MidBossMonster* MidBoss = Cast<AT3MidBossMonster>(Owner))
	{
		MidBoss->RemoveStateTag(TAG_Boss_State_PlayerParryable);
		UE_LOG(LogDesecration, Log, TEXT("T3_MidBoss: PlayerParryable 윈도우 닫힘 (ANS)"));
	}
}
