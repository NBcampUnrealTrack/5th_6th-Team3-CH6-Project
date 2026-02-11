// T3MonsterBase.cpp

#include "Monster/T3MonsterBase.h"
#include "Components/CapsuleComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Touch.h"


AT3MonsterBase::AT3MonsterBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AT3MonsterBase::BeginPlay()
{
	Super::BeginPlay();
	HealthComponent = FindComponentByClass<UT3HealthComponent>();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->OnComponentBeginOverlap.AddDynamic(this, &AT3MonsterBase::OnCapsuleBeginOverlap);
	}
}

void AT3MonsterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AT3MonsterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float AT3MonsterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HealthComponent)
	{
		HealthComponent->HandleTakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("T3MonsterBase: HealthComponent is null when taking damage."));
	}

	return ActualDamage;
}

void AT3MonsterBase::OnCapsuleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (OtherActor && OtherActor != this)
    {
        // TargetActor
        AActor* TargetActor = OtherActor;

        APawn* PotentialPawn = Cast<APawn>(OtherActor);
        if (!PotentialPawn)
        {
            AActor* OwnerActor = OtherActor->GetOwner();
            if (Cast<APawn>(OwnerActor))
            {
                TargetActor = OwnerActor;
            }
        }

        UAISense_Touch::ReportTouchEvent(
            GetWorld(),
            this,
            TargetActor,
            TargetActor->GetActorLocation()
        );
    }
}
