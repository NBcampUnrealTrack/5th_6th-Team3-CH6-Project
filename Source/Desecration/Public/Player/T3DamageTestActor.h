// T3DamageTestActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3DamageTestActor.generated.h"

class UBoxComponent;

UCLASS()
class DESECRATION_API AT3DamageTestActor : public AActor
{
    GENERATED_BODY()

public:
    AT3DamageTestActor();

protected:
    virtual void BeginPlay() override;

    // 데미지를 줄 콜리젼 박스
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UBoxComponent> InteractionBox;

    // 테스트하고 싶은 데미지 수치
    UPROPERTY(EditAnywhere, Category = "Damage Setup")
    float DamageAmount = 10.f;

    // 3번 테스트 목적: 데미지 타입 (TSubclassOf를 사용해 에디터에서 선택)
    // 예: UDamageType_Light, UDamageType_Heavy 등을 선택 가능
    UPROPERTY(EditAnywhere, Category = "Damage Setup")
    TSubclassOf<UDamageType> DamageTypeClass;

    // 2번 테스트 목적: 막기/패링 무시 여부 등 (커스텀 데이터 보낼 때 유용)
    UPROPERTY(EditAnywhere, Category = "Damage Setup")
    bool bIsUnblockable = false;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);
};
