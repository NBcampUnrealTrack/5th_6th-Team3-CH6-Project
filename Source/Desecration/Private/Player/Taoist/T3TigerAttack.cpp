// T3TigerAttack.cpp


#include "Player/Taoist/T3TigerAttack.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/T3CharacterBase.h"
#include "Player/T3CombatComponent.h"
#include "Player/Taoist/T3TaoistClone.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

AT3TigerAttack::AT3TigerAttack()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsLaunching = false;

    AttackCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollision"));
    AttackCollision->SetupAttachment(RootComponent);

    if (AttackCollision)
    {
        AttackCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        AttackCollision->SetCollisionResponseToAllChannels(ECR_Overlap);
        // 적 채널에 대해서만 Overlap 되도록 설정 (예: ECC_GameTraceChannel1)

        // 델리게이트 바인딩
        AttackCollision->OnComponentBeginOverlap.AddDynamic(this, &AT3TigerAttack::OnAttackOverlap);
    }

    // --- 추가 설정 ---
    // 공중에서도 어느 정도 제어가 가능하도록 설정
    GetCharacterMovement()->AirControl = 0.8f;
    // 발사 시 가속도를 확실히 받기 위해 마찰력 조정 (필요 시)
    GetCharacterMovement()->FallingLateralFriction = 0.1f;
}


void AT3TigerAttack::OnAttackOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // 본인이 아니며, 유효한 포인터이고, 적(Enemy) 태그 등을 확인
    if (OtherActor && OtherActor != this)
    {
        UE_LOG(LogTemp, Warning, TEXT("Tiger Hit Target: %s"), *OtherActor->GetName());

        // 1. 이동 중지
        bIsLaunching = false;
        GetCharacterMovement()->Velocity = FVector::ZeroVector;

        // 2. 폭발 및 데미지 로직 실행 (Notify에서 하려던 것을 여기서 직접 호출)
        TriggerExplosion(OtherActor);

        // 3. 중복 실행 방지를 위해 콜리젼 끄기
        AttackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AT3TigerAttack::TriggerExplosion(AActor* TargetActor)
{
    AActor* ActualOwner = GetOwner();
    if (!ActualOwner ||TargetActor->IsA( AT3CharacterBase::StaticClass())) return;

    // 본체든 분신이든 CombatComponent를 가져오는 로직 (본체는 본인 거, 분신은 주인 거)
    UT3CombatComponent* CombatComp = nullptr;

    if (AT3CharacterBase* Char = Cast<AT3CharacterBase>(ActualOwner)) {
        CombatComp = Char->GetCombatComponent();
    }
    else if (AT3TaoistClone* Clone = Cast<AT3TaoistClone>(ActualOwner)) {
        // 분신이라면 분신의 Owner(본체)의 컴포넌트를 사용
        if (Clone->GetOwnerCharacter()) {
            CombatComp = Clone->GetOwnerCharacter()->GetCombatComponent();
        }
    }

    if (IsValid(CombatComp))
    {
        CombatComp->RequestAttackDamage(TargetActor, Damage);
        UE_LOG(LogTemp, Display, TEXT("Tiger Damage Applied: %.1f"), Damage);
    }

    // 1. 이펙트 재생
    if (ExplosionEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
    }

    // 2. 사운드 재생
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }

    // 3. 호랑이 퇴장
    Destroy();
}


void AT3TigerAttack::LaunchTiger(FVector Direction, float Speed)
{
    if (Direction.IsNearlyZero())
    {
        UE_LOG(LogTemp, Error, TEXT("Tiger Error: Launch Direction is Zero!"));
        return;
    }

    LaunchDirection = Direction.GetSafeNormal();
    MovementSpeed = Speed;
    bIsLaunching = true;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        // 1. 이동 모드 확인 로그
        MoveComp->SetMovementMode(EMovementMode::MOVE_Falling);
        UE_LOG(LogTemp, Warning, TEXT("Tiger Status: Movement Mode set to FALLING"));

        // 2. 물리 제약 해제
        MoveComp->MaxWalkSpeed = Speed;
        MoveComp->MaxAcceleration = 10000.f; // 가속도 대폭 상향
        MoveComp->BrakingDecelerationFalling = 0.f; // 공중 감속 제거

        // 3. 속도 직접 주입
        MoveComp->Velocity = LaunchDirection * Speed;
        UE_LOG(LogTemp, Warning, TEXT("Tiger Status: Velocity Set to %s"), *MoveComp->Velocity.ToString());
    }

    SetActorRotation(LaunchDirection.Rotation());

    if (AttackMontage)
    {
        PlayAnimMontage(AttackMontage);
        UE_LOG(LogTemp, Warning, TEXT("Tiger Status: Attack Montage Playing"));
    }

    if (SpawnSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, SpawnSound, GetActorLocation());
    }
}

void AT3TigerAttack::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsLaunching)
    {
        // 1. 강제로 위치를 계산해서 옮깁니다 (깡물리 방식)
        FVector NewLocation = GetActorLocation() + (LaunchDirection * MovementSpeed * DeltaTime);

        // 2. Sweep을 true로 두면 장애물에 걸리고, false면 뚫고 지나갑니다.
        SetActorLocation(NewLocation, true);

        // 디버그 로그 (실제 위치 변화를 더 명확히 확인)
    }
}