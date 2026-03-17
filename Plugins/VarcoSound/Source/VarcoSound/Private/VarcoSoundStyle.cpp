// Copyright Epic Games, Inc. All Rights Reserved.

#include "VarcoSoundStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FVarcoSoundStyle::StyleInstance = nullptr;

void FVarcoSoundStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FVarcoSoundStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FVarcoSoundStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("VarcoSoundStyle"));
	return StyleSetName;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon48x48(48.0f, 48.0f);
const FVector2D Icon128x128(128.0f, 128.0f);
const FVector2D Banner(200.0f, 40.0f);

TSharedRef< FSlateStyleSet > FVarcoSoundStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("VarcoSoundStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("VarcoSound")->GetBaseDir() / TEXT("Resources"));

	Style->Set("VarcoSound.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("PlaceholderButtonIcon"), Icon20x20));
	Style->Set("VarcoSound.Icon128", new IMAGE_BRUSH(TEXT("Icon128"), Icon48x48));
	Style->Set("VarcoSound.Banner", new IMAGE_BRUSH(TEXT("banner"), Banner));

	return Style;
}

void FVarcoSoundStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FVarcoSoundStyle::Get()
{
	return *StyleInstance;
}
