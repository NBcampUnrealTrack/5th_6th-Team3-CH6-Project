// T3TigerAttack.cpp


#include "Player/Taoist/T3TigerAttack.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AT3TigerAttack::AT3TigerAttack()
{
    PrimaryActorTick.bCanEverTick = true;
    bIsLaunching = false;

    AttackCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollision"));
    AttackCollision->SetupAttachment(RootComponent);
    AttackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // --- 추가 설정 ---
    // 공중에서도 어느 정도 제어가 가능하도록 설정
    GetCharacterMovement()->AirControl = 0.8f;
    // 발사 시 가속도를 확실히 받기 위해 마찰력 조정 (필요 시)
    GetCharacterMovement()->FallingLateralFriction = 0.1f;
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