// Copyright 2025, Straw Hat,  All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

UENUM(BlueprintType)
enum class EICAssetType : uint8
{
	StaticMesh UMETA(DisplayName = "Static Mesh"),
	SkeletalMesh UMETA(DisplayName = "Skeletal Mesh"),
	Actor
};

UENUM(BlueprintType)
enum class EICBackgroundType : uint8
{
	Transparent,
	Color,
	Texture
};