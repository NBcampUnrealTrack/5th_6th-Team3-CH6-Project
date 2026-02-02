// T3AN_SkillEvent.cpp


#include "Player/T3AN_SkillEvent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3SkillComponentBase.h"


void UT3AN_SkillEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp || !MeshComp->GetOwner()) return;

    // 1. 캐릭터 베이스로 캐스팅하여 데이터 에셋에 접근하거나 컴포넌트 호출
    AT3CharacterBase* Character = Cast<AT3CharacterBase>(MeshComp->GetOwner());
    if (Character)
    {
        // 2. 캐릭터가 가지고 있는 스킬 컴포넌트를 찾음
        // (직업별로 다른 컴포넌트가 붙어있어도 부모 타입인 Base로 찾으면 다 잡힙니다)
        UT3SkillComponentBase* SkillComp = Character->FindComponentByClass<UT3SkillComponentBase>();

        if (SkillComp)
        {
            // 3. 실제 스폰 로직 실행 (전달받은 SkillIndex를 활용)
            // 컴포넌트 내부에 SpawnProjectileByIndex(int32 Index) 같은 함수를 만들어두면 좋습니다.
            SkillComp->ExecuteSkillNotify(SkillIndex);
        }
    }
}