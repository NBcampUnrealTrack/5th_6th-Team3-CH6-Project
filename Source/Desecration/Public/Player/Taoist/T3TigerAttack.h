#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "T3TigerAttack.generated.h"

UCLASS()
class DESECRATION_API AT3TigerAttack : public ACharacter
{
    GENERATED_BODY()

public:
    AT3TigerAttack();

    // 전방으로 날아가게 하는 함수
    void LaunchTiger(FVector Direction, float Speed);

protected:
    virtual void Tick(float DeltaTime) override;

    // 점프 공격 애니메이션
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TObjectPtr<UAnimMontage> AttackMontage;

    // 공격 판정용 컬리전 (애니메이션 노티파이에서 제어)
    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<class UBoxComponent> AttackCollision;

private:
    FVector LaunchDirection;
    float MovementSpeed = 0.f;
    uint8 bIsLaunching : 1; // 비트필드 사용 (언리얼 최적화 스타일)
};