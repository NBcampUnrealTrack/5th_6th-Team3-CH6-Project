// T3TalismanProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3TalismanProjectile.generated.h"

UCLASS()
class DESECRATION_API AT3TalismanProjectile : public AActor
{
	GENERATED_BODY()
	
public:	

	AT3TalismanProjectile();

protected:
    // 부적의 외형
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* TalismanMesh;

    // 투사체 움직임 컴포넌트
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    class UProjectileMovementComponent* ProjectileMovement;

    // 충돌 처리
    UPROPERTY(VisibleAnywhere, Category = "Collision")
    class UBoxComponent* CollisionBox;

};
