// T3Taoist_SkillComponent.cpp


#include "Player/Taoist/T3Taoist_SkillComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h" 
#include "Player/Taoist/T3TalismanProjectile.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Player/Taoist/T3StrongWind.h"
#include "Player/Taoist/T3FanWeapon.h"
#include "Player/Taoist/T3TaoistClone.h"
#include "Player/Taoist/T3TigerAttack.h"

UT3Taoist_SkillComponent::UT3Taoist_SkillComponent()
{ }

void UT3Taoist_SkillComponent::InitializeFanWeapon()
{
    // 이미 캐싱되어 있다면 스킵
    if (FanWeapon) return;

    if (OwnerChar && Combat)
    {
        if (IsValid(Combat->GetWeaponBySlot(EEquipSlot::RightHand)))
        FanWeapon = Cast<AT3FanWeapon>(Combat->GetWeaponBySlot(EEquipSlot::RightHand));
        UE_LOG(LogTemp, Display, TEXT("Fan Weapon Equiped"));
    }

}

void UT3Taoist_SkillComponent::BeginPlay()
{
    Super::BeginPlay();
    InitializeFanWeapon();

    // 캐릭터의 메시를 가져와서 나이아가라 컴포넌트 초기화
    if (OwnerChar && GhostTrailSystem)
    {
        GhostTrailComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
            GhostTrailSystem,
            OwnerChar->GetMesh(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            false,       // bAutoDestroy: false
            false        // bAutoActivate: false
        );

    }
}

FSkillData* UT3Taoist_SkillComponent::GetSkillDataByID(int32 SkillID)
{
    switch (SkillID)
    {
    case 1: return &StrongWindData;
    case 2: return &TaoistDodgeData;
    case 3: return &ShadowCloneData;
    case 4: return &SummonTigerData;

    default: return nullptr;
    }
}

void UT3Taoist_SkillComponent::ExecuteSkill(int32 SlotNumber)
{
    // 1. 슬롯 번호(1 or 2)에 따른 ID 추출
    int32 SkillID = (SlotNumber == 1) ? CurrentSkillSlot : NextSkillSlot;

    // 2. ID에 맞는 데이터 가져오기
    FSkillData* TargetData = GetSkillDataByID(SkillID);

    // 3. 마나 & 쿨타임 체크
    if (!TargetData || SkillID == 0) return;
    if (!CanExecuteSkill(*TargetData)) return;

    // 4. 쿨타임 시작 및 스킬 실행
    StartCooldown(SkillID, *TargetData);

    // ID에 따른 분기
    switch (SkillID)
    {
    case 0: // 빈슬롯
        UE_LOG(LogTemp, Warning, TEXT("There is no skill."));    break;
    case 1: // 장풍
        ExecuteStrongWind();    
        UE_LOG(LogTemp, Warning, TEXT("skill1"));
        break;
    case 2: // 축지법
        ExecuteTaoistDodge();
        UE_LOG(LogTemp, Warning, TEXT("skill2"));
        break;

    case 3: // 분신술
        SpawnShadowClones();
        UE_LOG(LogTemp, Warning, TEXT("skill3"));
        break;

    case 4: // 호랑이 소환술
        SummonTiger();
        UE_LOG(LogTemp, Warning, TEXT("skill4"));
        break;

    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown Skill ID: %d"), SkillID);   break;
    }
}


void UT3Taoist_SkillComponent::OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 스킬 사용 상태 해제
    bUsingSkill = false;

    UE_LOG(LogTemp, Log, TEXT("Skill Montage Ended. bUsingSkill set to false. Interrupted: %s"), bInterrupted ? TEXT("True") : TEXT("False"));
}

void UT3Taoist_SkillComponent::ExecuteSkillNotify(int32 Index)
{
    // 공통 노티파이에서 보낸 Index에 따라 분기
    switch (Index)
    {

    case 1: // 장풍
        SpawnStrongWind();
        break;

    case 5: // 기본 공격 (부적 날리기)
        SpawnTalisman();
        NotifyClonesAction(EActionType::AttackSpawn);
        break;

    case 6 : // 부채 펴기
        if (IsValid(FanWeapon)) { FanWeapon->OpenFan(); }
        break;

    case 7: // 부채 접기
        if (IsValid(FanWeapon)) { FanWeapon->CloseFan(); }
        break;

    case 8: // 기본 공격 모션 클론에게 복제
        NotifyClonesAction(EActionType::AttackAnim);
        UE_LOG(LogTemp, Display, TEXT("skill notify 8"));
        break;
    }
}

void UT3Taoist_SkillComponent::CancelCurrentSkill()
{
    
}

void UT3Taoist_SkillComponent::SpawnTalisman()
{
    if (!TalismanClass || !OwnerChar)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnTalisman: 필수 변수가 설정되지 않았습니다!"));
        return;
    }

    UWorld* World = GetWorld();
    if (World)
    {
        // 1. 스폰 위치 및 회전 설정
        FVector SpawnLocation = OwnerChar->GetActorLocation() + OwnerChar->GetActorForwardVector() * 100.f;
        FRotator SpawnRotation = OwnerChar->GetActorRotation();
        FTransform SpawnTransform(SpawnRotation, SpawnLocation);

        // 2. 디퍼드 스폰 시작 (인스턴스 생성, BeginPlay는 아직)
        AT3TalismanProjectile* Talisman = World->SpawnActorDeferred<AT3TalismanProjectile>(
            TalismanClass,
            SpawnTransform,
            OwnerChar,   // Owner
            OwnerChar,   // Instigator
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

        if (Talisman)
        {
            // 3. 데이터 주입 (BeginPlay 호출 전)
            // 기본 공격(부적)의 데미지 계산 로직 적용
            float InitialDamage = OwnerChar->GetAttackPower();

            // 만약 패시브(SetEmpowermentState) 강화 상태라면 데미지 30% 증가
            if (IsEmpowered())
            {
                InitialDamage *= 1.3f;
                SetEmpowermentState(false); // 강화 소모
            }

            Talisman->SetDamage(InitialDamage); // 부적 클래스에 SetDamage 함수가 정의되어 있어야 합니다.

            // 4. 스폰 완료 (이때 BeginPlay가 호출됨)
            Talisman->FinishSpawning(SpawnTransform);
        }
    }
}

void UT3Taoist_SkillComponent::SetEmpowermentState(bool bEnabled)
{
    // 쿨타임 중이면 강화될 수 없음
    if (bEnabled && bIsOnCooldown) return;

    bIsSpiritualEmpowered = bEnabled;

    if (bIsSpiritualEmpowered)
    {
        // 부채 빛나는 효과 넣기
        UE_LOG(LogTemp, Log, TEXT("도력 강화 활성화!"));
    }
    else
    {
        // 공격이 끝났으므로 쿨타임 시작
        bIsOnCooldown = true;
        GetWorld()->GetTimerManager().SetTimer(
            EmpowermentTimerHandle,
            this,
            &UT3Taoist_SkillComponent::ResetEmpowermentCooldown,
            EmpowermentCooldown,
            false
        );

        // 부채 빛나는 효과 비활성화
        UE_LOG(LogTemp, Log, TEXT("도력 사용 완료, 쿨타임 시작"));
    }
}

void UT3Taoist_SkillComponent::ResetEmpowermentCooldown()
{
    bIsOnCooldown = false;
    UE_LOG(LogTemp, Log, TEXT("도력 강화 재사용 가능"));
}

void UT3Taoist_SkillComponent::ExecuteStrongWind()
{
    // 1. 유효성 검사
    if (!OwnerChar || !StrongWindData.SkillMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("ExecuteStrongWind: OwnerChar or Montage is null!"));
        return;
    }

    UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance();
    if (AnimInstance)
    {
        // 2. 스킬 사용 중 상태 설정
        bUsingSkill = true;

        // 3. 몽타주 재생
        float Duration = OwnerChar->PlayAnimMontage(StrongWindData.SkillMontage);

        if (Duration > 0.f)
        {
            // 4. 몽타주 종료 델리게이트 바인딩
            FOnMontageEnded MontageEndedDelegate;
            MontageEndedDelegate.BindUObject(this, &UT3Taoist_SkillComponent::OnSkillMontageEnded);
            AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, StrongWindData.SkillMontage);

        }
        else
        {
            bUsingSkill = false;
        }
    }
}

void UT3Taoist_SkillComponent::SpawnStrongWind()
{
    if (!StrongWindData.ProjectileClass) // ProjectileClass 변수를 이펙트 액터용으로 재활용
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnStrongWind: 이펙트 액터 클래스가 설정되지 않았습니다!"));
        return;
    }

    UWorld* World = GetWorld();
    if (World && OwnerChar)
    {
        // 1. 스폰 위치: 캐릭터 발밑에서 전방으로 약간 띄움
        FVector SpawnLocation = OwnerChar->GetActorLocation() + (OwnerChar->GetActorForwardVector() * StrongWindSpawnDistance);

        // 2. 스폰 회전: 캐릭터가 보는 방향
        FRotator SpawnRotation = OwnerChar->GetActorRotation();

        //  디퍼드 스폰 시작 (액터 인스턴스만 생성)
        AT3StrongWind* StrongWindActor = World->SpawnActorDeferred<AT3StrongWind>(
            StrongWindData.ProjectileClass,
            FTransform(SpawnRotation, SpawnLocation),
            OwnerChar,
            OwnerChar,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

        if (StrongWindActor)
        {
            // BeginPlay가 호출되기 전에 미리 데미지 전달
            float FinalDamage = StrongWindData.DamageMultiflier * OwnerChar->GetAttackPower();

            if (IsEmpowered())
            {
                FinalDamage *= 1.3f;
                SetEmpowermentState(false); // 강화 소모
            }

            StrongWindActor->SetDamage(FinalDamage);

            // 스폰 완료
            StrongWindActor->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
        }
    }
}

void UT3Taoist_SkillComponent::SetGhostTrailActive(bool bActive)
{
    if (!GhostTrailComponent) return;

    if (bActive)
    {
        GhostTrailComponent->Activate(true);
    }
    else
    {
        GhostTrailComponent->Deactivate();
    }
}

void UT3Taoist_SkillComponent::ExecuteTaoistDodge()
{
    FT3PlayerInputState& PIS = OwnerChar->PlayerInputState;

    float CurrentAngle = PIS.InputYawOffset;
    PIS.RollDirection = OwnerChar->GetRollDirection(CurrentAngle);

    SetGhostTrailActive(true);
    OnTaoistDodgeTriggered();
}

void UT3Taoist_SkillComponent::SpawnShadowClones()
{
    // 1. 역순으로 순회하여 안전하게 제거
     // 인덱스를 뒤에서부터 앞으로 훑으면 중간에 요소가 삭제되어도 인덱스 꼬임이나 크래시가 발생하지 않습니다.
    for (int32 i = ActiveClones.Num() - 1; i >= 0; --i)
    {
        if (ActiveClones.IsValidIndex(i) && IsValid(ActiveClones[i]))
        {
            // Destroy()를 호출하면 OnCloneRemoved가 실행되어 배열에서 알아서 빠집니다.
            ActiveClones[i]->Destroy();
        }
    }

    // 혹시라도 남아있을 찌꺼기 정리
    ActiveClones.Empty();

    // 2. 새로운 분신 스폰
    for (int32 i = 0; i < 2; i++)
    {
        FVector SpawnOffset = (i == 0) ? OwnerChar->GetActorRightVector() * 150.f : OwnerChar->GetActorRightVector() * -150.f;
        FVector SpawnLocation = OwnerChar->GetActorLocation() + SpawnOffset;

        AT3TaoistClone* NewClone = GetWorld()->SpawnActorDeferred<AT3TaoistClone>(
            CloneClass,
            FTransform(OwnerChar->GetActorRotation(), SpawnLocation),
            OwnerChar,
            OwnerChar,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

        if (NewClone)
        {
            NewClone->InitializeClone(OwnerChar);

            // 분신 파괴 시 배열에서 제거하는 델리게이트
            NewClone->OnCloneDestroyed.AddUObject(this, &UT3Taoist_SkillComponent::OnCloneRemoved);

            NewClone->FinishSpawning(FTransform(OwnerChar->GetActorRotation(), SpawnLocation));
            ActiveClones.Add(NewClone);
        }
    }
}

void UT3Taoist_SkillComponent::OnCloneRemoved(AT3TaoistClone* ExClone)
{
    if (ExClone)
    {
        // 배열에서 해당 분신 포인터를 찾아 제거합니다.
        ActiveClones.Remove(ExClone);
        UE_LOG(LogTemp, Warning, TEXT("분신이 제거되었습니다. 남은 분신 수: %d"), ActiveClones.Num());
    }
}

void UT3Taoist_SkillComponent::NotifyClonesAction(EActionType ActionType)
{
    for (AT3TaoistClone* Clone : ActiveClones)
    {
        if (IsValid(Clone))
        {
            Clone->ExecuteMirrorAction(ActionType);
        }
    }
}

void UT3Taoist_SkillComponent::SummonTiger()
{
    if (!TigerClass) return;

    AActor* Owner = GetOwner();
    if (!IsValid(Owner)) return;

    FVector SpawnLocation = Owner->GetActorLocation()
        + (Owner->GetActorForwardVector() * 200.f);
    FRotator SpawnRotation = Owner->GetActorRotation(); // 플레이어가 보는 방향과 동일하게
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.Instigator = Owner->GetInstigator();

    AT3TigerAttack* SummonedTiger = GetWorld()->SpawnActor<AT3TigerAttack>(TigerClass, SpawnLocation, SpawnRotation, SpawnParams);

    if (SummonedTiger)
    {
        // 소환 직후 전방으로 날아가는 속도 설정 (예: 1200.f)
        SummonedTiger->LaunchTiger(Owner->GetActorForwardVector(), 600.f);
    }
}
