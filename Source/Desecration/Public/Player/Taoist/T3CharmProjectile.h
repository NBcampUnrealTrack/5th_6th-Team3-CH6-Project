// T3CharmProjectile.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T3CharmProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNiagaraSystem;

DECLARE_DELEGATE_TwoParams(FOnCharmExploded, FVector, AActor*);

UCLASS()
class DESECRATION_API AT3CharmProjectile : public AActor
{
	GENERATED_BODY()

public:
    AT3CharmProjectile();

    FOnCharmExploded OnCharmExploded;
protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // 초기 발사 함수 (캐릭터에서 호출할 예정)
    void LaunchCharm(FVector LaunchDirection, float Power);

private:
    /** 컴포넌트 설정 */
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<USphereComponent> CollisionComp;

    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComp;

    /** 연출용 변수 */
    bool bIsActive = false;
    FVector Velocity;

    UPROPERTY(EditAnywhere, Category = "Charm|Movement")
    float GravityScale = 0.3f; // 종이니까 중력을 약하게 받도록 (0.1 ~ 0.5 권장)

    UPROPERTY(EditAnywhere, Category = "Charm|Movement")
    float HorizontalDamping = 0.95f; // 공기 저항 (매 프레임 속도 감소율)

    UPROPERTY(EditAnywhere, Category = "Charm|Movement")
    float SwaySpeed = 5.0f; // 좌우 흔들림 속도

    UPROPERTY(EditAnywhere, Category = "Charm|Movement")
    float SwayIntensity = 15.0f; // 좌우 흔들림 강도

    /** 소환 관련 */
    UPROPERTY(EditAnywhere, Category = "Charm|Spawn")
    TSubclassOf<AActor> ActorToSpawn; // 분신이나 호랑이 BP를 여기서 할당

    UPROPERTY(EditAnywhere, Category = "Charm|Spawn")
    TObjectPtr<UNiagaraSystem> SpawnEffect; // 펑 터지는 이펙트

    FTimerHandle ExplosionTimerHandle;
    void HandleExplosion();
};
