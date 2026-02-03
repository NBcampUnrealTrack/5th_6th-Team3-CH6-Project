// Copyright 2025, Straw Hat,  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "ICEnums.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ICItemCreatorWidget.generated.h"

class UNiagaraSystem;

UCLASS()
class ICONCREATOR_API UICItemCreatorWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:

	/**
	* Creates a new Static Texture from a Render Target 2D.
	* Only works in the editor
	*/
	UFUNCTION(BlueprintCallable, Category = "Icon Creator")
	UTexture2D* CreateTextureFromRenderTarget(UTextureRenderTarget2D* InRenderTarget, FString InName, FString InSavePath, TextureCompressionSettings InCompressionSettings = TC_EditorIcon, TextureMipGenSettings InMipSettings = TMGS_FromTextureGroup, TextureGroup InTextureGroup = TEXTUREGROUP_UI);

	UFUNCTION(BlueprintCallable, Category = "Icon Creator")
	void RefreshAnimPosition(USkeletalMeshComponent* InSkeletalMesh);

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset")
		EICAssetType AssetType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset", meta = (EditCondition = "AssetType == EICAssetType::StaticMesh", EditConditionHides))
		TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset", meta = (EditCondition = "AssetType == EICAssetType::SkeletalMesh", EditConditionHides))
		TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset", meta = (EditCondition = "AssetType == EICAssetType::SkeletalMesh", EditConditionHides))
		TObjectPtr<UAnimationAsset> Anim;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset", meta = (EditCondition = "AssetType == EICAssetType::SkeletalMesh", EditConditionHides))
		float AnimPosition;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset", meta = (EditCondition = "AssetType == EICAssetType::Actor", EditConditionHides))
		TSubclassOf<AActor> Actor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset")
		TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset Position")
		bool Center = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset Position")
		FRotator AssetRotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Asset Position", meta = (AllowPreserveRatio))
		FVector AssetScale = FVector(1, 1, 1);

	/** Camera field of view (in degrees). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera", Interp, meta = (UIMin = "5.0", UIMax = "170", ClampMin = "0.001", ClampMax = "360.0"))
		float FieldOfView = 90;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		float Distance = 300;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		float ZoomStep = 5;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		FVector CameraLocation;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Camera")
		FRotator CameraRotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lighting")
		bool CastShadows1 = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lighting")
		bool CastShadows2 = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lighting")
		bool CastShadows3 = true;



	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		FString IconName = "T_";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		FString SavePath = "/Game/IconCreator";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		FVector2D Resolution = FVector2D(512, 512);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save")
		TEnumAsByte<enum TextureCompressionSettings> CompressionSettings = TextureCompressionSettings::TC_EditorIcon;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Save", meta = (DisplayName = "TextureGroup"))
		TEnumAsByte<enum TextureGroup> IconTextureGroup = TextureGroup::TEXTUREGROUP_UI;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview", meta = (AllowPrivateAccess = "true", DisplayThumbnail = "true", AllowedClasses = "/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
		TObjectPtr<UObject> PreviewImage;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview")
		FVector2D IconSize = FVector2D(50, 50);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview")
		EICBackgroundType Background;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview", meta = (EditCondition = "Background == EICBackgroundType::Color", EditConditionHides))
		FLinearColor Color;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Preview", meta = (EditCondition = "Background == EICBackgroundType::Texture", EditConditionHides))
		UTexture2D* Texture;


};
