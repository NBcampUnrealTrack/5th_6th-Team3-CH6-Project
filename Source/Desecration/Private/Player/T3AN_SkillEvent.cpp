// T3AN_SkillEvent.cpp


#include "Player/T3AN_SkillEvent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3SkillComponentBase.h"
#include "Player/Taoist/T3TigerAttack.h"


void UT3AN_SkillEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    // 호랑이 스킬을 인덱스 10으로 
    if (SkillIndex == 10)
    {
        AT3TigerAttack* Tiger = Cast<AT3TigerAttack>(MeshComp->GetOwner());
        if (IsValid(Tiger))
        {
            UE_LOG(LogTemp, Display, TEXT("Tiger Attack!!"));
            Tiger->Destroy();
        }
    }

    else
    {
        // 1. 캐릭터 베이스로 캐스팅하여 데이터 에셋에 접근하거나 컴포넌트 호출
        AT3CharacterBase* Character = Cast<AT3CharacterBase>(MeshComp->GetOwner());
        if (Character)
        {
            // 2. 캐릭터가 가지고 있는 스킬 컴포넌트를 찾음
            UT3SkillComponentBase* SkillComp = Character->FindComponentByClass<UT3SkillComponentBase>();

            if (SkillComp)
            {
                // 3. 실제 스폰 로직 실행
                SkillComp->ExecuteSkillNotify(SkillIndex);
            }
        }
    }
}