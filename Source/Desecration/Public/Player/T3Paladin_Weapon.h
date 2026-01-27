// T3Paladin_Weapon.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3Paladin_Weapon.generated.h"

UCLASS()
class DESECRATION_API AT3Paladin_Weapon : public AActor
{
	GENERATED_BODY()
	
public:
    AT3Paladin_Weapon();

    // 충돌체 (칼날 부분에 배치)
    UPROPERTY(VisibleAnywhere, Category = "Combat")
   TObjectPtr<class UBoxComponent> WeaponCollision;

    // 무기 외형
    UPROPERTY(VisibleAnywhere, Category = "Visual")
    TObjectPtr<class UStaticMeshComponent> WeaponMesh;

    // 데미지 활성화/비활성화 함수
    void SetWeaponCollisionEnabled(ECollisionEnabled::Type NewType);

protected:
    virtual void BeginPlay() override;

    // 오버랩 이벤트 함수
    UFUNCTION()
    void OnWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
    // 한 번의 휘두름에 동일 대상을 여러 번 타격하지 않도록 저장하는 리스트
    TArray<AActor*> AlreadyHitActors;
};

