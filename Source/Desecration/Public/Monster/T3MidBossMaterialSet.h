// T3MidBossMaterialSet.h
// 중간보스 스테이지별 머티리얼 세트 (DataAsset)

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "T3MidBossMaterialSet.generated.h"

/**
 * 중간보스 머티리얼 세트
 * 스켈레탈 메시 슬롯(0~5) + 무기 슬롯(1) 머티리얼을 한 세트로 관리
 * DA_DarkKnight_Set1, DA_DarkKnight_Set2 등으로 에디터에서 생성하여 사용
 */
UCLASS(BlueprintType)
class DESECRATION_API UT3MidBossMaterialSet : public UDataAsset
{
	GENERATED_BODY()

public:
	// 본체 스켈레탈 메시 머티리얼 (슬롯 순서대로 — 비어있는 슬롯은 원본 유지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MaterialSet")
	TArray<TObjectPtr<UMaterialInterface>> BodyMaterials;

	// 무기 머티리얼 (nullptr이면 원본 유지)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MaterialSet")
	TObjectPtr<UMaterialInterface> WeaponMaterial;

	// 디졸브 경계 발광색 오버라이드 (머티리얼의 DissolveEv 파라미터)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MaterialSet|Dissolve")
	bool bOverrideDissolveColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MaterialSet|Dissolve", meta = (EditCondition = "bOverrideDissolveColor"))
	FLinearColor DissolveColor = FLinearColor(0.3f, 1.5f, 3.0f);

	// 프레넬 림 색상 오버라이드 (머티리얼의 ColorFresnel 파라미터)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MaterialSet|Dissolve")
	bool bOverrideFresnelColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MaterialSet|Dissolve", meta = (EditCondition = "bOverrideFresnelColor"))
	FLinearColor FresnelColor = FLinearColor(0.f, 0.235f, 0.422f);
};
