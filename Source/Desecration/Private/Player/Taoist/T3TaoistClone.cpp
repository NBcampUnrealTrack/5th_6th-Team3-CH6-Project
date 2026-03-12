// T3TaoistClone.cpp


#include "Player/Taoist/T3TaoistClone.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Player/Taoist/T3TaoistCloneController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/Taoist/T3TalismanProjectile.h"
#include "NavigationSystem.h"
#include "Player/T3CombatComponent.h"
#include "Player/T3WeaponBase.h"
#include "Player/Taoist/T3StrongWind.h"
#include "Player/Taoist/T3TigerAttack.h"
#include "NiagaraFunctionLibrary.h"

AT3TaoistClone::AT3TaoistClone()
{
    PrimaryActorTick.bCanEverTick = false;
    bUseControllerRotationYaw = true;
    // AI가 컨트롤할 수 있도록 설정
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AT3TaoistCloneController::StaticClass();
    GetCharacterMovement()->MaxWalkSpeed = 400.f;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
}

void AT3TaoistClone::BeginPlay()
{
    Super::BeginPlay();

    ApplyGlowToEverything();
}

void AT3TaoistClone::ApplyGlowToEverything()
{
    // 1. 이 액터(분신)와 그 자식(무기 등)들이 가진 모든 MeshComponent를 수집
    TArray<UMeshComponent*> AllMeshComps;

    // bIncludeChildren을 true로 설정하여 자식 액터(무기)의 메시까지 포함
    bool bIncludeChildren = true;
    GetComponents<UMeshComponent>(AllMeshComps, bIncludeChildren);

    for (UMeshComponent* MeshComp : AllMeshComps)
    {
        if (!MeshComp) continue;

        // 2. 각 메시가 가진 모든 머터리얼 슬롯 순회
        int32 MaterialCount = MeshComp->GetNumMaterials();
        for (int32 i = 0; i < MaterialCount; ++i)
        {
            // 동적 머터리얼 인스턴스 생성 및 적용
            UMaterialInstanceDynamic* DynMat = MeshComp->CreateDynamicMaterialInstance(i);
            if (DynMat)
            {
                //에서 설정한 파라미터 이름과 일치해야 함
                DynMat->SetVectorParameterValue(TEXT("GlowColor"), CloneGlowColor);
                DynMat->SetScalarParameterValue(TEXT("GlowIntensity"), 0.5f);

                // 나중에 제어하기 위해 배열에 저장
                DynamicMaterials.Add(DynMat);
            }
        }
    }
}

void AT3TaoistClone::Destroyed()
{


    UWorld* World = GetWorld();
    if (World)
    {
        // 펑! 하는 나이아가라 이펙트 재생
        if (DestroyEffect)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                World,
                DestroyEffect,
                GetActorLocation(),
                GetActorRotation()
            );
        }

        // 펑! 하는 사운드 재생
        if (DestroySound)
        {
            UGameplayStatics::PlaySoundAtLocation(this, DestroySound, GetActorLocation());
        }
    }

    // 2. 기존 로직 (델리게이트 호출 및 자식 액터 파괴)
    if (OnCloneDestroyed.IsBound())
    {
        OnCloneDestroyed.Broadcast(this);
    }

    // 나에게 붙어 있는 모든 액터(부채 등) 파괴
    TArray<AActor*> AttachedActors;
    GetAttachedActors(AttachedActors);
    for (AActor* AttachedActor : AttachedActors)
    {
        if (IsValid(AttachedActor))
        {
            AttachedActor->Destroy();
        }
    }

    Super::Destroyed();
}

void AT3TaoistClone::InitializeClone(AT3CharacterBase* InOwner)
{
    if (!IsValid(InOwner)) return;

    OwnerCharacter = InOwner;

    // 체력 70 고정
    SetMaxHP(70.f);
    SetCurrentHP(70.f);

    // AI 업데이트 타이머 시작 (0.5초 간격으로 타겟 추적)
    GetWorldTimerManager().SetTimer(AIUpdateTimer, this, &AT3TaoistClone::UpdateAIBehavior, 0.5f, true);

}

float AT3TaoistClone::GetAttackPower() const
{
    // 본체 공격력의 30% 반환
    return OwnerCharacter->GetAttackPower() * 0.3f;
}

void AT3TaoistClone::ExecuteMirrorAction(EActionType ActionType)
{
    switch (ActionType)
    {
    case EActionType::AttackAnim:
        PlayAnimMontage(AttackMontage, 0.7f);
        UE_LOG(LogTemp, Display, TEXT("AttackAnim"));
        break;

    case EActionType::AttackSpawn:
        SpawnTalisman();
        UE_LOG(LogTemp, Display, TEXT("AttackSpawn"));
        break;

    case EActionType::StrongWindAnim:
        PlayAnimMontage(StrongWindMontage);
        break;

    case EActionType::SummonTigerAnim:
        PlayAnimMontage(SummonTigerMontage);
        break;

    case EActionType::StrongWindSpawn:
        break;

    case EActionType::SummonTigerSpawn:
        break;
    }
}

void AT3TaoistClone::UpdateAIBehavior()
{
    if (!IsValid(OwnerCharacter))
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] OwnerCharacter가 유효하지 않아 파괴됩니다."), *GetName());
        Destroy();
        return;
    }

    AActor* ClosestEnemy = nullptr;
    float MinDistance = 3000.f; // 인식 범위 30m

    TArray<AActor*> OutActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APawn::StaticClass(), OutActors);


    for (AActor* Actor : OutActors)
    {
        // 자기 자신이나 본체, 혹은 본체가 소유한 다른 객체 제외
        if (Actor == OwnerCharacter || Actor == this || Actor->GetOwner() == OwnerCharacter)
        {
            continue;
        }

        // 2. 추가 제외: 다른 분신 (클래스 체크)
        if (Actor->IsA(AT3TaoistClone::StaticClass())) continue;

        // 로그 2: 후보군 발견 및 거리 체크
        float Distance = OwnerCharacter->GetDistanceTo(Actor);

        if (Distance < MinDistance)
        {
            MinDistance = Distance;
            ClosestEnemy = Actor;
        }
    }

    AT3TaoistCloneController* AIC = Cast<AT3TaoistCloneController>(GetController());
    if (AIC)
    {

        // 적이 있으면 적을 추적, 없으면 본체(OwnerCharacter)를 추적
        AActor* FinalTarget = ClosestEnemy ? ClosestEnemy : OwnerCharacter;

        if (FinalTarget)
        {
            AIC->UpdateTargetTracking(FinalTarget);

            // 도착했으므로 무작위 이동 타이머 시작 (이미 돌아가고 있다면 무시)
            if (!GetWorldTimerManager().IsTimerActive(IdleWanderTimer))
            {
                ResetIdleWanderTimer();
            }
        }
        else
        {
            // 아직 이동 중이거나 타겟이 멀어지면 무작위 이동 타이머 취소
            GetWorldTimerManager().ClearTimer(IdleWanderTimer);
            AIC->UpdateTargetTracking(FinalTarget);
        }
        }
    
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] AI Controller를 캐스트할 수 없습니다!"), *GetName());
    }
}

void AT3TaoistClone::ResetIdleWanderTimer()
{
    // 3초 ~ 10초 사이 랜덤 시간 설정
    float NextWaitTime = FMath::FRandRange(3.0f, 5.0f);
    GetWorldTimerManager().SetTimer(IdleWanderTimer, this, &AT3TaoistClone::StartIdleWander, NextWaitTime, false);
}

void AT3TaoistClone::StartIdleWander()
{
    AT3TaoistCloneController* AIC = Cast<AT3TaoistCloneController>(GetController());
    if (!AIC || !OwnerCharacter) return;

    // 1. 현재 추적 중인 타겟 가져오기 (컨트롤러의 Focus 대상)
    AActor* CurrentTarget = AIC->GetFocusActor();
    if (!CurrentTarget) return;

    // 2. 타겟으로의 방향 벡터 계산
    FVector ToTarget = (CurrentTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal();

    // 3. 타겟 방향의 수직(오른쪽) 벡터 구하기
    FVector RightSide = FVector::CrossProduct(FVector::UpVector, ToTarget);

    // 4. 왼쪽(-1) 또는 오른쪽(1) 중 랜덤 결정
    float DirectionMultiplier = FMath::RandBool() ? 1.f : -1.f;

    // 5. 옆으로 100~200 사이만큼 이동 (앞뒤 이동 배제)
    float WanderDistance = FMath::FRandRange(100.f, 200.f);
    FVector WanderGoal = GetActorLocation() + (RightSide * DirectionMultiplier * WanderDistance);

    // 6. 길찾기 가능한 위치인지 확인 후 이동 (NavMesh 상의 위치)
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (NavSys)
    {
        FNavLocation ProjectedLocation;
        if (NavSys->ProjectPointToNavigation(WanderGoal, ProjectedLocation))
        {
            AIC->MoveToLocation(ProjectedLocation.Location, 10.f);
        }
    }

    // 다음 랜덤 이동 예약
    ResetIdleWanderTimer();
}

float AT3TaoistClone::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    // 가해자가 본체(OwnerCharacter)라면 데미지 0 처리
    if (DamageCauser == OwnerCharacter || (EventInstigator && EventInstigator->GetPawn() == OwnerCharacter))
    {
        return 0.f;
    }

    // 1. 부모 클래스(CharacterBase)의 기본 데미지 처리 호출 
    float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // 2. 체력 확인 
    if (GetCurrentHP() <= 0.f)
    {
        // 3. 사망 처리: 분신 파괴
        // 필요하다면 여기서 사망 애니메이션이나 이펙트를 재생할 수 있습니다.
        UE_LOG(LogTemp, Warning, TEXT("[%s] 분신의 체력이 소진되어 파괴됩니다."), *GetName());
        Destroy();
    }

    return ActualDamage;
}

void AT3TaoistClone::SpawnTalisman()
{
    // 1. 필수 변수 체크 
    if (!OwnerCharacter) return;

    // 만약 분신 자체에 TalismanClass를 설정하지 않았다면 본체 컴포넌트에서 찾아옴
    if (!TalismanClass) return;


    UWorld* World = GetWorld();
    if (World && TalismanClass)
    {
        // 2. 스폰 위치 및 회전 설정 (분신 정면 1m 앞)
        FVector SpawnLocation = GetActorLocation() + GetActorForwardVector() * 100.f;
        FRotator SpawnRotation = GetActorRotation();
        FTransform SpawnTransform(SpawnRotation, SpawnLocation);

        // 3. 디퍼드 스폰 시작

        AT3TalismanProjectile* Talisman = World->SpawnActorDeferred<AT3TalismanProjectile>(
            TalismanClass,
            SpawnTransform,
            this,           // Owner: 분신
            OwnerCharacter, // Instigator: 본체
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

        if (Talisman)
        {

            float FinalDamage = GetAttackPower();

            Talisman->SetDamage(FinalDamage);

            // 5. 스폰 완료
            Talisman->FinishSpawning(SpawnTransform);

            UE_LOG(LogTemp, Log, TEXT("[%s] 분신이 부적을 발사했습니다! 데미지: %.1f"), *GetName(), FinalDamage);
        }
    }
}

void AT3TaoistClone::SpawnStrongWind()
{
   
    if (!StrongWindClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnStrongWind: 이펙트 액터 클래스가 설정되지 않았습니다!"));
        return;
    }

    UWorld* World = GetWorld();
    if (World && OwnerCharacter)
    {
        // 1. 스폰 위치: 캐릭터 발밑에서 전방으로 약간 띄움
        FVector SpawnLocation = this->GetActorLocation() + (this->GetActorForwardVector() * StrongWindSpawnDistance);

        // 2. 스폰 회전: 캐릭터가 보는 방향
        FRotator SpawnRotation = this->GetActorRotation();

        //  디퍼드 스폰 시작 (액터 인스턴스만 생성)
        AT3StrongWind* StrongWindActor = World->SpawnActorDeferred<AT3StrongWind>(
            StrongWindClass,
            FTransform(SpawnRotation, SpawnLocation),
            this,
            OwnerCharacter,
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn
        );

        if (StrongWindActor)
        {
            // BeginPlay가 호출되기 전에 미리 데미지 전달
            float FinalDamage = StrongWindDamageMultiflier * GetAttackPower();

            StrongWindActor->SetDamage(FinalDamage);

            // 스폰 완료
            StrongWindActor->FinishSpawning(FTransform(SpawnRotation, SpawnLocation));
        }
    }
}


