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

    void SetDamage(float InDamage) { Damage = InDamage; }

protected:
    virtual void Tick(float DeltaTime) override;

    float Damage = 0.f; 

    UFUNCTION()
    void OnAttackOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    void TriggerExplosion(class AActor* TargetActor);

    // 점프 공격 애니메이션
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
    TObjectPtr<UAnimMontage> AttackMontage;

    // 공격 판정용 컬리전 (애니메이션 노티파이에서 제어)
    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<class UBoxComponent> AttackCollision;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class UNiagaraSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* ExplosionSound;

    UPROPERTY(EditAnywhere, Category = "Effects")
    class USoundBase* SpawnSound;

private:
    FVector LaunchDirection;
    float MovementSpeed = 0.f;
    uint8 bIsLaunching : 1; // 비트필드 사용 (언리얼 최적화 스타일)
};