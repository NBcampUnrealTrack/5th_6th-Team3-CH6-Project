#include "Monster/T3MonsterBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/T3DamageTypes.h"

AT3MonsterBase::AT3MonsterBase() {
  PrimaryActorTick.bCanEverTick = true;
  CurrentHP = 0.0f;
  CurrentATK = 0.0f;
  bIsDead = false;
  bIsAttacking = false;
  bIsAttacking = false;
  HeavyDamageThreshold = 50.0f;
  bDropWeaponOnDeath = true;
  bHideWeaponOnDeath = false;
  bIsWeaponDropped = false;
}

void AT3MonsterBase::BeginPlay() {
  Super::BeginPlay();

  if (MonsterDataTable) {
    static const FString ContextString(TEXT("MonsterData"));
    if (FMonsterStats *Stats = MonsterDataTable->FindRow<FMonsterStats>(
            MonsterRowName, ContextString)) {
      CurrentHP = Stats->MaxHP;
      CurrentATK = Stats->AttackDamage;
    }
  }
}

float AT3MonsterBase::TakeDamage(float DamageAmount,
                                 FDamageEvent const &DamageEvent,
                                 AController *EventInstigator,
                                 AActor *DamageCauser) {
  if (bIsDead) {
    return 0.0f;
  }

  const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent,
                                               EventInstigator, DamageCauser);
  CurrentHP -= ActualDamage;

  const FT3DamageEvent* T3Event = static_cast<const FT3DamageEvent*>(&DamageEvent);

  if (T3Event)
  {
      EHitIntensity Intensity = T3Event->HitIntensity;
  }

  // [Fallback] Update LastKnownLocation via AI Controller (in case Perception
  // doesn't fire)
  if (!bIsDead && DamageCauser) {
    if (AController *MyController = GetController()) {
      if (UBlackboardComponent *BB =
              MyController->FindComponentByClass<UBlackboardComponent>()) {
        // Get attacker location
        FVector AttackerLocation = DamageCauser->GetActorLocation();
        if (!AttackerLocation.IsNearlyZero()) {
          BB->SetValueAsVector(TEXT("LastKnownLocation"), AttackerLocation);

// [DEBUG] Visualize LKL if enabled
#if !UE_BUILD_SHIPPING
          DrawDebugSphere(GetWorld(), AttackerLocation, 50.0f, 12,
                          FColor::Red, // Different color for fallback path
                          false, 2.0f);
#endif
        }
      }
    }
  }

  // Death Check
  if (CurrentHP <= 0.0f) {
    bIsDead = true;
    // Determine which Death Montage to play based on last hit
    UAnimMontage *DeathMontageToPlay = (ActualDamage >= HeavyDamageThreshold)
                                           ? DeathMontage_Heavy
                                           : DeathMontage_Light;

    // If Heavy Death is null, fallback to Light. If Light is null, logic
    // handles it.
    if (!DeathMontageToPlay) {
      DeathMontageToPlay = DeathMontage_Light;
    }

    if (DeathMontageToPlay) {
      StopAnimMontage(); // Stop any current action
      PlayAnimMontage(DeathMontageToPlay);

      // Setup callback for Ragdoll
      if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance()) {
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &AT3MonsterBase::OnMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate, DeathMontageToPlay);
      }
    } else {
      // Immediate Ragdoll if no montage
      EnableRagdoll();
    }

    OnDeath();
    return ActualDamage;
  }

  // Hit Reaction Logic
  bool bIsHeavyHit = (ActualDamage >= HeavyDamageThreshold);

  if (bIsHeavyHit) {
    // [Heavy Hit]
    // 1. Stop current animation
    StopAnimMontage();

    // 2. Broadcast End Signal only if we were attacking
    if (bIsAttacking) {
      OnPerformAttackEnd.Broadcast();
      // Note: We do NOT set bIsAttacking = false here directly.
      // The BT Task will receive the broadcast, finish, and call
      // SetIsAttacking(false).
    }

    // 3. Play Heavy Hit Montage
    if (HitReactMontage_Heavy) {
      PlayAnimMontage(HitReactMontage_Heavy);
    }

    // 4. Knockback / Launch (Forceful)
    if (GetCharacterMovement()) {
      GetCharacterMovement()->StopMovementImmediately();
    }

    if (DamageCauser) {
      FVector Direction =
          (GetActorLocation() - DamageCauser->GetActorLocation())
              .GetSafeNormal();
      FVector LaunchVelocity = Direction * 500.0f + FVector(0.0f, 0.0f, 200.0f);
      LaunchCharacter(LaunchVelocity, true, true);

      // Clamp velocity to prevent excessive speed (once after launch)
      if (UCharacterMovementComponent *Movement = GetCharacterMovement()) {
        FVector CurrentVelocity = Movement->Velocity;
        float MaxSpeed = 900.0f; // Maximum allowed speed
        if (CurrentVelocity.Size() > MaxSpeed) {
          Movement->Velocity = CurrentVelocity.GetSafeNormal() * MaxSpeed;
        }
      }
    }
  } else {
    // [Light Hit]
    // 1. Check Iron Body (Ignore if Attacking)
    if (bIsAttacking) {
      return ActualDamage;
    }

    // 2. Play Light Hit Montage
    if (HitReactMontage_Light) {
      PlayAnimMontage(HitReactMontage_Light);
    }

    // 3. Small Knockback
    if (GetCharacterMovement()) {
      GetCharacterMovement()->StopMovementImmediately();
    }

    if (DamageCauser) {
      FVector Direction =
          (GetActorLocation() - DamageCauser->GetActorLocation())
              .GetSafeNormal();
      FVector LaunchVelocity = Direction * 200.0f;
      LaunchCharacter(LaunchVelocity, true, true);

      // Clamp velocity to prevent excessive speed (once after launch)
      if (UCharacterMovementComponent *Movement = GetCharacterMovement()) {
        FVector CurrentVelocity = Movement->Velocity;
        float MaxSpeed = 600.0f; // Maximum allowed speed for light hit
        if (CurrentVelocity.Size() > MaxSpeed) {
          Movement->Velocity = CurrentVelocity.GetSafeNormal() * MaxSpeed;
        }
      }
    }
  }

  return ActualDamage;
}

void AT3MonsterBase::OnDeath() {
  // Disable Collision
  if (GetCapsuleComponent()) {
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  }

  // Detach Controller
  DetachFromControllerPendingDestroy();

  // Destroy after delay (e.g., 5 seconds)
  SetLifeSpan(5.0f);
}

void AT3MonsterBase::SetIsAttacking(bool bValue) { bIsAttacking = bValue; }

void AT3MonsterBase::OnMontageEnded(UAnimMontage *Montage, bool bInterrupted) {
  // Trigger Ragdoll after Death Montage ends
  // Check if this montage matches one of our death montages
  if (Montage == DeathMontage_Light || Montage == DeathMontage_Heavy) {
    EnableRagdoll();
  }
}

void AT3MonsterBase::EnableRagdoll() {
  // 0. Drop Weapon First
  DropWeapon();

  // 1. Capsule Collision Disable
  if (GetCapsuleComponent()) {
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  }

  // 2. Mesh Physics Enable
  if (GetMesh()) {
    GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
    GetMesh()->SetSimulatePhysics(true);
    GetMesh()->SetAllBodiesSimulatePhysics(true);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    // 3. Angular Velocity Damping (1/10) - Apply once
    // Get current angular velocity and scale it down
    // Note: We apply this to the root or all bodies depending on effect.
    // Usually applying to all bodies works best for stabilizing.

    // For UE5, we can iterate bodies or use a simpler approach if available.
    // SetAllPhysicsAngularVelocityInDegrees is a good candidate if we want to
    // reset/scale. However, reading current velocity for each body might be
    // heavy. Let's try scaling the Root Bone's angular velocity which often
    // drives the spin.

    if (UPrimitiveComponent *MeshComp = Cast<UPrimitiveComponent>(GetMesh())) {
      // Option A: Scale global angular velocity (simple)
      FVector CurrentAngularVel =
          MeshComp->GetPhysicsAngularVelocityInDegrees();
      MeshComp->SetPhysicsAngularVelocityInDegrees(CurrentAngularVel * 0.1f);

      // Option B: Iterate all bodies (more robust for complex skeletons)
      // But for now, let's stick to the user request "1/10 damping".
      // We can use SetAllPhysicsAngularVelocityInDegrees but we need to know
      // the values effectively. Actually, "Damping" usually refers to the
      // 'AngularDamping' property, but the user asked to "scale the velocity by
      // 1/10".

      // Let's iterate bodies to be safe and accurate for the whole ragdoll
      FBodyInstance *RootBody = GetMesh()->GetBodyInstance();
      if (RootBody) {
        FVector RootAngVel = RootBody->GetUnrealWorldAngularVelocityInRadians();
        RootBody->SetAngularVelocityInRadians(RootAngVel * 0.1f, false);
      }

      // Apply to all bodies if possible (SkeletalMeshComponent specific APIs)
      // Iterate over all bodies
      for (FBodyInstance *Body : GetMesh()->Bodies) {
        if (Body) {
          FVector AngVel = Body->GetUnrealWorldAngularVelocityInRadians();
          Body->SetAngularVelocityInRadians(AngVel * 0.1f, false);
        }
      }
    }
  }
}

void AT3MonsterBase::DropWeapon() {
  if (bIsWeaponDropped || !WeaponComponent) {
    return;
  }

  bIsWeaponDropped = true;

  if (bHideWeaponOnDeath) {
    WeaponComponent->SetHiddenInGame(true);
    WeaponComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  } else if (bDropWeaponOnDeath) {
    WeaponComponent->DetachFromComponent(
        FDetachmentTransformRules::KeepWorldTransform);
    // Set collision to ignore Pawn (player) but collide with world
    WeaponComponent->SetCollisionProfileName(TEXT("IgnoreOnlyPawn"));
    WeaponComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WeaponComponent->SetSimulatePhysics(true);

    // Optional: Apply small damping or impulse
    if (UPrimitiveComponent *PrimComp =
            Cast<UPrimitiveComponent>(WeaponComponent)) {
      PrimComp->SetAngularDamping(1.0f);
      PrimComp->SetLinearDamping(0.1f);
    }
  }
}

void AT3MonsterBase::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  // Tick 로직
}

void AT3MonsterBase::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  Super::SetupPlayerInputComponent(PlayerInputComponent);
  // 입력 바인딩
}