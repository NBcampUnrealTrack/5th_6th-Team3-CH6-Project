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
#include "Player/Taoist/T3CharmProjectile.h"

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
        NotifyClonesAction(EActionType::StrongWindAnim);
        UE_LOG(LogTemp, Warning, TEXT("skill1"));
        break;
    case 2: // 축지법
        ExecuteTaoistDodge();
        UE_LOG(LogTemp, Warning, TEXT("skill2"));
        break;

    case 3: // 분신술
        PlayThrowChramMontage(ShadowCloneData);
        UE_LOG(LogTemp, Warning, TEXT("skill3"));
        break;

    case 4: // 호랑이 소환술
        PlayThrowChramMontage(SummonTigerData);
        NotifyClonesAction(EActionType::SummonTigerAnim);
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
        NotifyClonesAction(EActionType::StrongWindSpawn);
        break;

    case 3: // 분신술
        ThrowSummonCharm(ShadowCloneData);
        break;

    case 4: // 호랑이 소환술
        ThrowSummonCharm(SummonTigerData);
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

            // 물리적 충돌 무시 (본체와 부딪히지 않게)
             // 부적의 RootComponent(CollisionBox)를 가져와서 설정
            if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Talisman->GetRootComponent()))
            {
                RootPrim->IgnoreActorWhenMoving(OwnerChar, true);

                // 분신들도 무시
                for (AT3TaoistClone* Clone : ActiveClones)
                {
                    if (Clone) RootPrim->IgnoreActorWhenMoving(Clone, true);
                }
            }

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

void UT3Taoist_SkillComponent::SpawnSingleShadowClone(FVector ExplosionLocation, AActor* Spawner)
{
    // 1. 기존 분신 제거는 스킬 시전 시점(ThrowSummonCharm)에서 이미 Empty() 했다고 가정해.
    // 만약 여기서 하면 첫 번째 부적 터질 때 지우고, 두 번째 부적 터질 때 첫 번째 걸 또 지우니까 안 돼!

    if (!CloneClass || !OwnerChar) return;

    // 2. 부적이 터진 그 위치(ExplosionLocation)에 그대로 스폰
    AT3TaoistClone* NewClone = GetWorld()->SpawnActorDeferred<AT3TaoistClone>(
        CloneClass,
        FTransform(OwnerChar->GetActorRotation(), ExplosionLocation),
        OwnerChar, OwnerChar, ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (NewClone)
    {
        NewClone->InitializeClone(OwnerChar);
        NewClone->OnCloneDestroyed.AddUObject(this, &UT3Taoist_SkillComponent::OnCloneDestroyed);

        // 부적 터진 위치 확정
        NewClone->FinishSpawning(FTransform(OwnerChar->GetActorRotation(), ExplosionLocation));

        // 관리 리스트에 추가
        ActiveClones.Add(NewClone);
    }
}

void UT3Taoist_SkillComponent::OnCloneDestroyed(AT3TaoistClone* DestroyedClone)
{
    if (DestroyedClone)
    {
        ActiveClones.Remove(DestroyedClone);
        UE_LOG(LogTemp, Display, TEXT("Clone removed from array. Remaining: %d"), ActiveClones.Num());
    }
}

void UT3Taoist_SkillComponent::DestroyAllActiveClones()
{
    for (int32 i = ActiveClones.Num() - 1; i >= 0; --i)
    {
        if (IsValid(ActiveClones[i]))
        {
            ActiveClones[i]->Destroy();
        }
    }
    ActiveClones.Empty();
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

void UT3Taoist_SkillComponent::SummonTigerAtLocation(FVector ExplosionLocation, AActor* Spawner)
{
    // Spawner가 없으면 기본적으로 OwnerChar(본체)를 주인으로 설정
    AActor* ActualOwner = IsValid(Spawner) ? Spawner : OwnerChar;

    if (!TigerClass || !OwnerChar) return;


    // --- 데미지 계산 로직 ---
    float FinalDamage = 0.f;

    if (AT3TaoistClone* Clone = Cast<AT3TaoistClone>(ActualOwner))
    {
        // 1. 분신이 소환한 경우
        FinalDamage = Clone->GetAttackPower() * SummonTigerData.DamageMultiflier;
    }
    else
    {
        // 2. 본체가 소환한 경우
        FinalDamage = OwnerChar->GetAttackPower() * SummonTigerData.DamageMultiflier;
    }

    FVector SpawnLocation = ExplosionLocation;

    // 바닥 감지 로직
    FHitResult HitResult;
    FVector Start = ExplosionLocation + FVector(0.f, 0.f, 100.f); // 위에서
    FVector End = ExplosionLocation - FVector(0.f, 0.f, 1000.f);   // 아래로 쏨

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(ActualOwner);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
    {
        // 충돌 지점(바닥)에서 호랑이의 캡슐 절반 높이만큼 올린 위치가 정확한 스폰 지점
        SpawnLocation = HitResult.Location + FVector(0.f, 0.f, 50.f);
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = ActualOwner;
    SpawnParams.Instigator = ActualOwner->GetInstigator();
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AT3TigerAttack* SummonedTiger = GetWorld()->SpawnActorDeferred<AT3TigerAttack>(
        TigerClass, 
        FTransform(ActualOwner->GetActorRotation(), SpawnLocation),
        ActualOwner,
        ActualOwner->GetInstigator(), 
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn
    );

    if (IsValid(SummonedTiger)) 
    {
        SummonedTiger->SetDamage(FinalDamage);
        SummonedTiger->FinishSpawning(FTransform(ActualOwner->GetActorRotation(), SpawnLocation));
        SummonedTiger->LaunchTiger(ActualOwner->GetActorForwardVector(), 600.f);
    }
}

void UT3Taoist_SkillComponent::ThrowSummonCharm(const FSkillData& SkillData)
{
    // 분신술일 때는 기존 분신 제거 + 분신은 이 스킬을 따라하지 않음
    if (&SkillData == &ShadowCloneData)
    {
        DestroyAllActiveClones();
        // 본체만 던짐
        SpawnCharmInternal(OwnerChar, SkillData);
    }
    else
    {
        // 호랑이 소환 등 다른 스킬은 본체 + 분신 모두 던짐
        SpawnCharmInternal(OwnerChar, SkillData);

        for (AT3TaoistClone* Clone : ActiveClones)
        {
            if (IsValid(Clone))
            {
                SpawnCharmInternal(Clone, SkillData);
            }
        }
    }
}

void UT3Taoist_SkillComponent::PlayThrowChramMontage(const FSkillData& SkillData)
{
    // 1. 애니메이션 재생 (부적 던지는 모션)
    if (SkillData.SkillMontage)
    {
        OwnerChar->PlayAnimMontage(SkillData.SkillMontage);
    }
}

void UT3Taoist_SkillComponent::SpawnCharmInternal(AActor* Spawner, const FSkillData& SkillData)
{
    if (!Spawner || !SkillData.ProjectileClass) return;

    // 분신술이면 2개, 아니면 1개
    int32 Count = (&SkillData == &ShadowCloneData) ? 2 : 1;

    for (int32 i = 0; i < Count; i++)
    {
        float BaseRightOffset = 70.f; // 애니메이션이 왼쪽으로 치우쳐서 보정 값
        FVector SideOffset = Spawner->GetActorRightVector() * BaseRightOffset;

        if (Count > 1)
        {
            SideOffset += Spawner->GetActorRightVector() * (i == 0 ? -60.f : 60.f);
        }

        FVector SpawnLocation = Spawner->GetActorLocation()
            + (Spawner->GetActorForwardVector() * SpawnForwardVector)
            + (Spawner->GetActorUpVector() * SpawnUpVector)
            + SideOffset;

        RotationOffset = FRotator(0.f, 10.f, 0.f);
        FRotator SpawnRotation = Spawner->GetActorRotation() + RotationOffset;

        AT3CharmProjectile* Charm = GetWorld()->SpawnActorDeferred<AT3CharmProjectile>(
            SkillData.ProjectileClass, FTransform(SpawnRotation, SpawnLocation),
            Spawner, OwnerChar, ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

        if (Charm)
        {
            if (&SkillData == &SummonTigerData)
                Charm->OnCharmExploded.BindUObject(this, &UT3Taoist_SkillComponent::SummonTigerAtLocation);
            else if (&SkillData == &ShadowCloneData)
                Charm->OnCharmExploded.BindUObject(this, &UT3Taoist_SkillComponent::SpawnSingleShadowClone);

            Charm->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));

            // [방향 조절] 분신술은 좌우로 벌어지게, 호랑이는 중앙으로
            FVector ThrowDir = Spawner->GetActorForwardVector() + Spawner->GetActorUpVector() * 0.5f;
            if (Count > 1)
            {
                ThrowDir += Spawner->GetActorRightVector() * (i == 0 ? -0.3f : 0.3f);
            }

            Charm->LaunchCharm(ThrowDir.GetSafeNormal(), ThrowSpeed);
        }
    }
}
