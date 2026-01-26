// Fill out your copyright notice in the Description page of Project Settings.


#include "Monster/T3MonsterController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GameFramework/Character.h"

AT3MonsterController::AT3MonsterController()
{
	// Initialize Blackboard Key Name
	BBKey_TargetActor = TEXT("TargetActor");

	// Create Perception Component
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerception);

	// Create and Configure Sight Sense
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	if (SightConfig)
	{
		SightConfig->SightRadius = 1500.0f;
		SightConfig->LoseSightRadius = 2000.0f;
		SightConfig->PeripheralVisionAngleDegrees = 90.0f;

		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

		AIPerception->ConfigureSense(*SightConfig);
		AIPerception->SetDominantSense(SightConfig->GetSenseImplementation());
	}
}

void AT3MonsterController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Run Behavior Tree
	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}

	// Bind Perception Delegate
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AT3MonsterController::OnTargetDetected);
	}
}

void AT3MonsterController::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{
    // Ensure Actor is valid
    if (!Actor) return;

    // Check if successfully sensed (seen)
    if (Stimulus.WasSuccessfullySensed())
    {
        // Update Blackboard
        if (GetBlackboardComponent())
        {
            GetBlackboardComponent()->SetValueAsObject(BBKey_TargetActor, Actor);
        }

        // Focus on the target (for strafing/aiming)
        SetFocus(Actor);
    }
}
