// Copyright 2025, Straw Hat,  All Rights Reserved.


#include "ICItemCreatorWidget.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"

UTexture2D* UICItemCreatorWidget::CreateTextureFromRenderTarget(UTextureRenderTarget2D* InRenderTarget, FString InName, FString InSavePath, TextureCompressionSettings InCompressionSettings, TextureMipGenSettings InMipSettings, TextureGroup InTextureGroup)
{
	if (!InRenderTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("RenderTarget must be non-null."));
		return nullptr;
	}
	else if (!InRenderTarget->GetResource())
	{
		UE_LOG(LogTemp, Warning, TEXT("RenderTarget has been released."));
		return nullptr;
	}
	else
	{
		FString OutPackageName;
		FString OutAssetName;
		if (!InSavePath.StartsWith(TEXT("/Game/")))
		{
			InSavePath.InsertAt(0, TEXT("/Game/"));
		}
		if (InName.IsEmpty())
		{
			InName = "T_";
		}
		FString FullPath = InSavePath / InName;

		IAssetTools& AssetTools = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.CreateUniqueAssetName(FullPath, TEXT(""), OutPackageName, OutAssetName);

		//create a static 2d texture
		UObject* NewObj = InRenderTarget->ConstructTexture2D(CreatePackage( *OutPackageName), OutAssetName, RenderTarget->GetMaskedFlags() | RF_Public | RF_Standalone, CTF_Default | CTF_AllowMips, nullptr);
		UTexture2D* NewTex = Cast<UTexture2D>(NewObj);

		if (NewTex != nullptr)
		{
			//package needs saving
			NewObj->MarkPackageDirty();

			//Notify the asset registry
			FAssetRegistryModule::AssetCreated(NewObj);

			//Update Compression and Mip settings
			NewTex->CompressionSettings = InCompressionSettings;
			NewTex->MipGenSettings = InMipSettings;
			NewTex->LODGroup = InTextureGroup;
			NewTex->PostEditChange();

			UEditorAssetLibrary::SaveLoadedAsset(NewObj);

			return NewTex;
		}
		UE_LOG(LogTemp, Warning, TEXT("Failed to create a new texture."));
	}
	return nullptr;
}

void UICItemCreatorWidget::RefreshAnimPosition(USkeletalMeshComponent* InSkeletalMesh)
{
	InSkeletalMesh->RefreshBoneTransforms();
	InSkeletalMesh->UpdateComponentToWorld();
	InSkeletalMesh->FinalizeBoneTransform();
	InSkeletalMesh->MarkRenderTransformDirty();
	InSkeletalMesh->MarkRenderDynamicDataDirty();
}
