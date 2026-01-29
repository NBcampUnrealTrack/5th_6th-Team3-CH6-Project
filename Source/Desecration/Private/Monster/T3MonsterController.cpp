// Fill out your copyright notice in the Description page of Project Settings.

#include "Monster/T3MonsterController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"

// Console Variable for LKL Debug Visualization
static TAutoConsoleVariable<int32> CVarDebugLKL(
    TEXT("ai.DebugLastKnownLocation"), 0,
    TEXT("Toggle LastKnownLocation debug visualization.\n0: Off, 1: On"),
    ECVF_Default);
#include "Perception/AISense_Sight.h"

AT3MonsterController::AT3MonsterController() {
  // Initialize Blackboard Key Names
  BBKey_TargetActor = TEXT("TargetActor");
  BBKey_LastKnownLocation = TEXT("LastKnownLocation");

  // Create Perception Component
  AIPerception =
      CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
  SetPerceptionComponent(*AIPerception);

  // Create and Configure Sight Sense
  SightConfig =
      CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

  if (SightConfig) {
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2000.0f;
    SightConfig->PeripheralVisionAngleDegrees = 60.0f;

    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
  }

  // Create and Configure Damage Sense
  DamageConfig =
      CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));

  if (DamageConfig) {
    DamageConfig->SetMaxAge(5.0f); // How long damage info persists
    AIPerception->ConfigureSense(*DamageConfig);
  }
}

void AT3MonsterController::OnPossess(APawn *InPawn) {
  Super::OnPossess(InPawn);

  // Run Behavior Tree
  if (BehaviorTreeAsset) {
    RunBehaviorTree(BehaviorTreeAsset);
  }

  // Bind Perception Delegate
  if (AIPerception) {
    AIPerception->OnTargetPerceptionUpdated.AddDynamic(
        this, &AT3MonsterController::OnTargetDetected);
  }

  // Save Home Location for Patrol/Return behavior
  if (UBlackboardComponent *BB = GetBlackboardComponent()) {
    if (InPawn) {
      FVector HomeLocation = InPawn->GetActorLocation();
      BB->SetValueAsVector(TEXT("HomeLocation"), HomeLocation);
      BB->SetValueAsFloat(TEXT("PatrolAngle"), 0.0f); // Initialize patrol angle
    }
  }
}

void AT3MonsterController::OnTargetDetected(AActor *Actor,
                                            FAIStimulus Stimulus) {
  // Ensure Actor is valid
  if (!Actor)
    return;

  // Get Blackboard
  UBlackboardComponent *BB = GetBlackboardComponent();
  if (!BB)
    return;
  
  if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>()) {
    // [DAMAGE = Clue Only]
    // DO NOT set TargetActor - only update LastKnownLocation

    FVector ClueLocation = Stimulus.StimulusLocation;

    // Validate location (avoid zero vector)
    if (!ClueLocation.IsNearlyZero()) {
      BB->SetValueAsVector(BBKey_LastKnownLocation, ClueLocation);

      // [DEBUG] Visualize LKL if enabled
      if (CVarDebugLKL.GetValueOnGameThread() > 0) {
        DrawDebugSphere(GetWorld(), ClueLocation,
                        50.0f, // Radius
                        12,    // Segments
                        FColor::Yellow,
                        false, // Persistent
                        3.0f   // Duration
        );
        DrawDebugDirectionalArrow(GetWorld(), ClueLocation + FVector(0, 0, 100),
                                  ClueLocation, 50.0f, FColor::Orange, false,
                                  3.0f);
      }
    }
  }
}
