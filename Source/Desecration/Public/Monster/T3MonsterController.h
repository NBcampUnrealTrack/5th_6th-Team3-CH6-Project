// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "T3MonsterController.generated.h"

class UAISenseConfig_Sight;

/**
 * 
 */
UCLASS()
class DESECRATION_API AT3MonsterController : public AAIController
{
	GENERATED_BODY()
	
public:
	AT3MonsterController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

protected:
	// AI Perception Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	class UAIPerceptionComponent* AIPerception;

	// Sight Sense Config
	class UAISenseConfig_Sight* SightConfig;

	// Behavior Tree Asset
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	class UBehaviorTree* BehaviorTreeAsset;

	// Blackboard Key for Target
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	FName BBKey_TargetActor;
};
