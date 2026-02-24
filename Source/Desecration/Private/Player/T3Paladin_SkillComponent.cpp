// T3Paladin_SkillComponent.cpp


#include "Player/T3Paladin_SkillComponent.h"
#include "GameFramework/Character.h"
#include "Engine/World.h" 
#include "Player/T3SwordWaveProjectile.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"

UT3Paladin_SkillComponent::UT3Paladin_SkillComponent()
{
}

FSkillData* UT3Paladin_SkillComponent::GetSkillDataByID(int32 SkillID)
{
    switch (SkillID)
    {
    case 1: return &SwordWaveData;
    case 2: return &ShieldStrikeData;
    // case 3: return &Data;
    case 4: return &JudgmentData;
    default: return nullptr;
    }
}

void UT3Paladin_SkillComponent::ExecuteSkill(int32 SlotNumber)
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
    case 1: // 검격
        ExecuteSwordWave();    break;
    case 2: // 방패찍기
        ShieldStrike();    break;
    case 3: // 도약찍기
        LeafAttack();    break;
    case 4: // 신의심판
        ExecuteJudgment();    break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown Skill ID: %d"), SkillID);   break;
    }
}

void UT3Paladin_SkillComponent::ExecuteSwordWave()
{
    if (!OwnerChar || !SwordWaveData.SkillMontage)
    {
        UE_LOG(LogTemp, Display, TEXT("no montage"))
         return;
    }
    UAnimInstance* AnimInstance = OwnerChar->GetMesh()->GetAnimInstance();
    if (OwnerChar && AnimInstance)
    {
        // 1. 스킬 사용 시작 상태 설정
        bUsingSkill = true;

        // 2. 몽타주 재생 (애니메이션 기반 스킬 실행)
        float Duration = OwnerChar->PlayAnimMontage(SwordWaveData.SkillMontage);
        
        if (Duration > 0.f)
        {
            // 3. 몽타주 종료 델리게이트 바인딩
            FOnMontageEnded MontageEndedDelegate;
            MontageEndedDelegate.BindUObject(this, &UT3Paladin_SkillComponent::OnSkillMontageEnded);
            AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, SwordWaveData.SkillMontage);
        }
        else
        {
            // 재생 실패 시 즉시 상태 초기화
            bUsingSkill = false;
        }
    }
}

void UT3Paladin_SkillComponent::OnSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    // 스킬 사용 상태 해제
    bUsingSkill = false;

    UE_LOG(LogTemp, Log, TEXT("Skill Montage Ended. bUsingSkill set to false. Interrupted: %s"), bInterrupted ? TEXT("True") : TEXT("False"));
}

void UT3Paladin_SkillComponent::ExecuteSkillNotify(int32 Index)
{
    // 공통 노티파이에서 보낸 Index에 따라 분기
    switch (Index)
    {
    case 0: // 검격 (Sword Wave)
        SpawnSwordWaveProjectile();
        break;
    }
}

void UT3Paladin_SkillComponent::CancelCurrentSkill()
{
    CancleJudgmentLaser();
}

void UT3Paladin_SkillComponent::SpawnSwordWaveProjectile()
{
    if (!SwordWaveData.ProjectileClass) return;

    UWorld* World = GetWorld();
    if (World)
    {
        // 위치가 애매하면 소켓을 생성해서 소켓의 위치 가져오기
        FVector SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 10.f;
        FRotator SpawnRotation = GetOwner()->GetActorRotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = Cast<APawn>(GetOwner());

        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AT3SwordWaveProjectile* Projectile = World->SpawnActor<AT3SwordWaveProjectile>(
            SwordWaveData.ProjectileClass,
            SpawnLocation,
            SpawnRotation,
            SpawnParams
        );

        if (Projectile)
        {
            //  데미지, 속도 전달
            float FinalDamage = SwordWaveData.DamageMultiflier * OwnerChar->GetAttackPower();
            Projectile->InitializeProjectile(FinalDamage, SwordWaveData.ProjectileSpeed);
            UE_LOG(LogTemp, Log, TEXT("Paladin SwordWave Launched!"));
        }
    }
}


void UT3Paladin_SkillComponent::ExecuteJudgment()
{
    if (!OwnerChar || !JudgmentData.SkillMontage) return;

    // 1. 상태 설정 (집중 시작)
    bUsingSkill = true;

    // 2. 애니메이션 재생 (4초 이상 지속되는 몽타주)
    OwnerChar->PlayAnimMontage(JudgmentData.SkillMontage);

    // 3. 장판 생성
    SpawnJudgmentArea();

    UE_LOG(LogTemp, Log, TEXT("신의 심판 시전: 기 모으는 중..."));
}

void UT3Paladin_SkillComponent::SpawnJudgmentArea()
{
    if (!OwnerChar || !Combat) return;

    // 1. 위치 결정 (록온 대상 여부에 따른 분기)  -> 록온 시 록온 대상 주변
    AActor* Target = Combat->GetCurrentTarget();
    float DistanceToTarget = Target ? OwnerChar->GetDistanceTo(Target) : 0.f;
    if (Target && DistanceToTarget <= 1000.f)
    {
        JudgmentTargetLocation = Target->GetActorLocation();
    }
    else
    {
        // 록온 대상이 없을 경우 전방 500 유닛 위치
        JudgmentTargetLocation = OwnerChar->GetActorLocation() + (OwnerChar->GetActorForwardVector() * JudgementExexcuteDistance);
    }

    // 2. 바닥 좌표 보정 (LineTrace)
    FVector TraceStart = JudgmentTargetLocation + FVector(0, 0, 500.f); // 위에서 아래로 쏜다
    FVector TraceEnd = JudgmentTargetLocation - FVector(0, 0, 1000.f);
    FHitResult GroundHit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerChar); // 시전자 자신은 제외

    // ECC_Visibility 채널을 사용하여 지면(Static Mesh 등)을 감지
    if (GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
    {
        // 실제 바닥 좌표(ImpactPoint)로 위치를 고정
        JudgmentTargetLocation = GroundHit.ImpactPoint;
    }

    // 3. 장판 이펙트 생성
    if (JudgmentAreaIndicatorClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = OwnerChar;
        GetWorld()->SpawnActor<AActor>(JudgmentAreaIndicatorClass, JudgmentTargetLocation, FRotator::ZeroRotator, SpawnParams);
    }

    // 3.2초 뒤 레이저 빔 실행 타이머
    GetWorld()->GetTimerManager().SetTimer(JudgmentTimerHandle, this, &UT3Paladin_SkillComponent::SpawnJudgmentLaser, 2.0f, false);
}

void UT3Paladin_SkillComponent::SpawnJudgmentLaser()
{
    // 1. 레이저 이펙트 소환
    if (JudgmentLaserClass)
    {
        CurrentJudgmentLaserActor = GetWorld()->SpawnActor<AActor>(JudgmentLaserClass, JudgmentTargetLocation, FRotator::ZeroRotator);
    }

    // 2. 다단 히트 데미지 시작 (0.4초 간격으로 5번)
    ApplyJudgmentDamage(5);
}

void UT3Paladin_SkillComponent::ApplyJudgmentDamage(int32 RemainingHits)
{
    if (RemainingHits <= 0 || !OwnerChar)
    {
        FinishJudgmentSkill();
        return;
    }

    // --- 디버깅 범위 표시 (0.4초간 유지되는 구체) ---
    DrawDebugSphere(GetWorld(), JudgmentTargetLocation, JudgementExexcuteRange, 32, FColor::Red, false, 0.4f, 0, 2.0f);

    // 범위 내 적 감지 (반경 500)
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(JudgementExexcuteRange);
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerChar);

    bool bHit = GetWorld()->OverlapMultiByChannel(OverlapResults, JudgmentTargetLocation, FQuat::Identity, ECC_Pawn, Sphere, Params);

    if (bHit)
    {
        float DamagePerHit = OwnerChar->GetAttackPower() * 2.0f; // 총 10배 중 1타당 2배

        for (auto& Result : OverlapResults)
        {
            if (UPrimitiveComponent* OverlappedComp = Result.GetComponent())
            {
                // 캡슐 컴포넌트가 아닌 위젯이나 다른 컴포넌트면 무시
                if (!OverlappedComp->IsA(UCapsuleComponent::StaticClass())) continue;
            }

            AActor* HitActor = Result.GetActor();
            ACharacter* TargetCharacter = Cast<ACharacter>(HitActor);
            if (IsValid(TargetCharacter))
            {
                UGameplayStatics::ApplyDamage(TargetCharacter, DamagePerHit, OwnerChar->GetController(), OwnerChar, nullptr);
                UE_LOG(LogTemp, Log, TEXT("신의 심판 적중: %s 에게 %.1f 데미지"), *TargetCharacter->GetName(), DamagePerHit);

                // 개별 피격 대상 디버그 라인
                DrawDebugLine(GetWorld(), JudgmentTargetLocation, TargetCharacter->GetActorLocation(), FColor::Yellow, false, 0.4f, 0, 1.0f);
            }
        }
    }

    // 0.4초 후 다음 타격 예약 (재귀적 타이머)
    if (RemainingHits > 1)
    {
        FTimerDelegate TimerDel;
        TimerDel.BindUObject(this, &UT3Paladin_SkillComponent::ApplyJudgmentDamage, RemainingHits - 1);
        GetWorld()->GetTimerManager().SetTimer(JudgmentTimerHandle, TimerDel, 0.4f, false);
    }

    else
    {
        FinishJudgmentSkill();
    }
}

void UT3Paladin_SkillComponent::CancleJudgmentLaser()
{
    // 스킬 사용 중이 아니면 실행할 필요 없음
    if (!bUsingSkill) return;

    // 1. 진행 중인 모든 타이머(대기, 다단히트) 제거
    GetWorld()->GetTimerManager().ClearTimer(JudgmentTimerHandle);

    if (CurrentJudgmentLaserActor)
    {
        FinishJudgmentSkill();
    }

    // 2. 상태 변수 초기화
    bUsingSkill = false;

    UE_LOG(LogTemp, Warning, TEXT("신의 심판 스킬이 캔슬되었습니다."));
}

// 공통 정리 함수 (정상 종료 & 필요 시 캔슬에서도 재활용 가능)
void UT3Paladin_SkillComponent::FinishJudgmentSkill()
{
    bUsingSkill = false;

    if (IsValid(CurrentJudgmentLaserActor))
    {
        CurrentJudgmentLaserActor->Destroy();
        CurrentJudgmentLaserActor = nullptr;
        OwnerChar->StopAnimMontage(JudgmentData.SkillMontage);
    }

    UE_LOG(LogTemp, Log, TEXT("신의 심판 스킬이 정상 종료되어 액터를 제거했습니다."));
}




// 팔라딘 전용 신성 게이지 로직

void UT3Paladin_SkillComponent::AddResource(float Amount)
{
    AddHolyGauge(Amount);
}

void UT3Paladin_SkillComponent::AddHolyGauge(float Amount)
{
    if (bIsHolyMode) return; // 이미 강화 상태면 무시

    HolyGauge = FMath::Clamp(HolyGauge + Amount, 0.f, MaxHolyGauge);

    // UI 업데이트 델리게이트 호출
    OnResourceChanged.Broadcast(HolyGauge);

    UE_LOG(LogTemp, Display, TEXT("Add HolyGauge : %f"),Amount);
    if (HolyGauge >= MaxHolyGauge)
    {
        ActivateHolyMode();
    }
}

void UT3Paladin_SkillComponent::ActivateHolyMode()
{
    bIsHolyMode = true;

    // 1. 공격 속도/딜레이 감소 적용
    AttackSpeedMultiplier += 0.2f;

    // 2. 20초 뒤 복구 예약
    GetWorld()->GetTimerManager().SetTimer(HolyModeTimerHandle, this, &UT3Paladin_SkillComponent::DeactivateHolyMode, 20.f, false);

    // 홀리모드 활성화 전달
    OnHolyModeChanged.Broadcast(true);

    UE_LOG(LogTemp, Warning, TEXT("Holy Mode Activated!"));
}

void UT3Paladin_SkillComponent::DeactivateHolyMode()
{
    bIsHolyMode = false;
    HolyGauge = 0.f; // 게이지 소모

    // 1. 공격 속도/딜레이 리셋
    AttackSpeedMultiplier = 1.0f;
    OnResourceChanged.Broadcast(HolyGauge);

    // 홀리모드 비활성화 전달
    OnHolyModeChanged.Broadcast(false);

    UE_LOG(LogTemp, Warning, TEXT("Holy Mode Deactivated!"));
}


