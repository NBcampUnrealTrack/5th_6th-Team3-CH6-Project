// Copyright Epic Games, Inc. All Rights Reserved.

#include "Utils/ImageCaptureUtils.h"

#if WITH_EDITOR

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "LevelEditor.h"
#include "ILevelEditor.h"
#include "SLevelViewport.h"
#include "Editor.h"
#include "UnrealClient.h"
#include "Misc/Base64.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "Engine/Selection.h"
#include "ShowFlags.h"
#include "TextureResource.h"

static bool EncodePngFromColors(const TArray<FColor>& SrcColors, int32 Width, int32 Height, TArray<uint8>& OutPng)
{
	OutPng.Reset();
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid())
	{
		return false;
	}
	if (!ImageWrapper->SetRaw(SrcColors.GetData(), SrcColors.Num() * sizeof(FColor), Width, Height, ERGBFormat::BGRA, 8))
	{
		return false;
	}
	OutPng = ImageWrapper->GetCompressed(100);
	return OutPng.Num() > 0;
}

bool PngBytesToBase64(const TArray<uint8>& Png, FString& OutBase64)
{
	OutBase64 = FBase64::Encode(Png);
	return !OutBase64.IsEmpty();
}

bool CaptureEditorViewportToPng(const FViewportCaptureOptions& Opts, TArray<uint8>& OutPng)
{
	OutPng.Reset();
	if (!IsInGameThread())
	{
		return false;
	}
	FLevelEditorModule* LevelEditorModule = FModuleManager::LoadModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
	if (!LevelEditorModule)
	{
		return false;
	}
	TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule->GetFirstLevelEditor();
	if (!LevelEditor.IsValid())
	{
		return false;
	}
	TSharedPtr<SLevelViewport> ActiveLevelViewport = LevelEditor->GetActiveViewportInterface();
	if (!ActiveLevelViewport.IsValid())
	{
		return false;
	}
	FViewport* Viewport = ActiveLevelViewport->GetActiveViewport();
	if (!Viewport)
	{
		return false;
	}
	// 옵션 적용: 그리즈모 숨김/AA 비활성 등은 에디터 설정에 의존적이므로 단순 캡쳐만 수행
	TArray<FColor> Bitmap;
	const bool bReadOk = Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(RCM_UNorm, CubeFace_MAX));
	if (!bReadOk || Bitmap.Num() == 0)
	{
		return false;
	}
	// 필요 시 리사이즈
	FIntPoint ViewportSize = Viewport->GetSizeXY();
	int32 SrcW = ViewportSize.X;
	int32 SrcH = ViewportSize.Y;
	int32 DstW = FMath::Max(1, Opts.Width);
	int32 DstH = FMath::Max(1, Opts.Height);
	TArray<FColor> Resized;
	if (SrcW != DstW || SrcH != DstH)
	{
		Resized.SetNumUninitialized(DstW * DstH);
		FImageUtils::ImageResize(SrcW, SrcH, Bitmap, DstW, DstH, Resized, false);
	}
	const TArray<FColor>& FinalColors = (Resized.Num() > 0) ? Resized : Bitmap;
	return EncodePngFromColors(FinalColors, (Resized.Num() > 0 ? DstW : SrcW), (Resized.Num() > 0 ? DstH : SrcH), OutPng);
}

static void ComputeAutoFramingTransform(const FBoxSphereBounds& Bounds, float FOVDeg, FVector& OutLocation, FRotator& OutRotation)
{
	const float Radius = Bounds.SphereRadius;
	const float HalfFovRad = FMath::DegreesToRadians(FOVDeg * 0.5f);
	const float Distance = Radius / FMath::Tan(HalfFovRad);
	OutRotation = FRotator(-10.f, 0.f, 0.f);
	OutLocation = Bounds.Origin + OutRotation.Vector() * (Distance + Radius * 0.5f);
}

static bool CropByAlphaTight(const TArray<FColor>& Src, int32 Width, int32 Height, TArray<FColor>& Out, int32& OutW, int32& OutH)
{
	int32 MinX = Width, MinY = Height, MaxX = -1, MaxY = -1;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FColor& C = Src[Y * Width + X];
			if (C.A > 0)
			{
				MinX = FMath::Min(MinX, X);
				MinY = FMath::Min(MinY, Y);
				MaxX = FMath::Max(MaxX, X);
				MaxY = FMath::Max(MaxY, Y);
			}
		}
	}
	if (MaxX < MinX || MaxY < MinY)
	{
		// 전부 투명 → 그대로 반환
		Out = Src;
		OutW = Width;
		OutH = Height;
		return true;
	}
	OutW = MaxX - MinX + 1;
	OutH = MaxY - MinY + 1;
	Out.SetNumUninitialized(OutW * OutH);
	for (int32 Y = 0; Y < OutH; ++Y)
	{
		FMemory::Memcpy(&Out[Y * OutW], &Src[(MinY + Y) * Width + MinX], sizeof(FColor) * OutW);
	}
	return true;
}

static FIntRect ComputeAlphaTightBounds(const TArray<FColor>& Src, int32 Width, int32 Height)
{
	int32 MinX = Width, MinY = Height, MaxX = -1, MaxY = -1;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			const FColor& C = Src[Y * Width + X];
			if (C.A > 0)
			{
				MinX = FMath::Min(MinX, X);
				MinY = FMath::Min(MinY, Y);
				MaxX = FMath::Max(MaxX, X);
				MaxY = FMath::Max(MaxY, Y);
			}
		}
	}
	if (MaxX < MinX || MaxY < MinY)
	{
		return FIntRect();
	}
	return FIntRect(MinX, MinY, MaxX + 1, MaxY + 1);
}

static void CropColors(const TArray<FColor>& Src, int32 Width, int32 Height, const FIntRect& Rect, TArray<FColor>& Out)
{
	const int32 OutW = Rect.Width();
	const int32 OutH = Rect.Height();
	Out.SetNumUninitialized(OutW * OutH);
	for (int32 Y = 0; Y < OutH; ++Y)
	{
		FMemory::Memcpy(&Out[Y * OutW], &Src[(Rect.Min.Y + Y) * Width + Rect.Min.X], sizeof(FColor) * OutW);
	}
}

static bool EncodeColorsToPNG(const TArray<FColor>& Src, int32 Width, int32 Height, TArray<uint8>& OutPng)
{
	return EncodePngFromColors(Src, Width, Height, OutPng);
}

bool CaptureSelectedActorsToPng(const FActorCaptureOptions& Opts, TArray<uint8>& OutPng)
{
	OutPng.Reset();

	if (!IsInGameThread())
	{
		return false;
	}

	if (!GEditor)
	{
		return false;
	}

	USelection* Selection = GEditor->GetSelectedActors();
	if (!Selection || Selection->Num() == 0)
	{
		return false;
	}

	UWorld* World = nullptr;
	for (FLevelEditorViewportClient* ViewportClient : GEditor->GetLevelViewportClients())
	{
		if (ViewportClient && ViewportClient->GetWorld())
		{
			World = ViewportClient->GetWorld();
			break;
		}
	}
	if (!World)
	{
		World = GEditor->GetEditorWorldContext().World();
	}
	if (!World)
	{
		return false;
	}

	// RenderTarget 생성
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(GetTransientPackage());
	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->InitCustomFormat(Opts.Width, Opts.Height, PF_B8G8R8A8, false);
	RenderTarget->ClearColor = FLinearColor(0, 0, 0, Opts.bTransparentBG ? 0.f : 1.f);
	RenderTarget->UpdateResourceImmediate(true);

	// SceneCapture 생성
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!CaptureActor)
	{
		return false;
	}

	UCameraComponent* CamComp = NewObject<UCameraComponent>(CaptureActor);
	CamComp->FieldOfView = Opts.FOV;
	CamComp->AttachToComponent(CaptureActor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	CamComp->RegisterComponent();

	USceneCaptureComponent2D* CaptureComp = CaptureActor->GetCaptureComponent2D();
	CaptureComp->FOVAngle = Opts.FOV;
	CaptureComp->CaptureSource = ESceneCaptureSource::SCS_SceneColorSceneDepth;
	CaptureComp->TextureTarget = RenderTarget;
	CaptureComp->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	// 가독성 개선 (그림자/광원 단순화 + 과한 후처리 제거)
	CaptureComp->ShowFlags.SetAtmosphere(false);
	CaptureComp->ShowFlags.SetFog(false);
	CaptureComp->ShowFlags.SetPostProcessing(true);
	CaptureComp->ShowFlags.SetAntiAliasing(true);
	CaptureComp->ShowFlags.SetDynamicShadows(false);
	CaptureComp->ShowFlags.SetDirectionalLights(true);
	CaptureComp->ShowFlags.SetPointLights(false);
	CaptureComp->ShowFlags.SetSpotLights(false);
	CaptureComp->ShowFlags.SetTranslucency(true);
	CaptureComp->bCaptureEveryFrame = false;
	CaptureComp->bCaptureOnMovement = false;

	// 선택된 액터 수집 및 바운즈 계산
	FBoxSphereBounds CombinedBounds(EForceInit::ForceInitToZero);
	TArray<AActor*> Actors;
	Selection->GetSelectedObjects<AActor>(Actors);
	for (AActor* Actor : Actors)
	{
		if (!Actor) { continue; }
		CaptureComp->ShowOnlyActors.Add(Actor);
		const FBox BoundsBox = Actor->GetComponentsBoundingBox(true);
		const FBoxSphereBounds Bounds(BoundsBox);
		if (CombinedBounds.SphereRadius <= 0.f)
		{
			CombinedBounds = Bounds;
		}
		else
		{
			const FBox BoxA = FBox::BuildAABB(CombinedBounds.Origin, CombinedBounds.BoxExtent);
			const FBox Union = BoxA + BoundsBox;
			CombinedBounds = FBoxSphereBounds(Union);
		}
	}

	if (Actors.Num() == 0)
	{
		CaptureActor->Destroy();
		return false;
	}

	// 카메라 오토프레이밍: 현재 뷰 시점 우선 사용
	FVector Center = CombinedBounds.Origin;
	float Radius = CombinedBounds.SphereRadius;
	Radius = FMath::Max(Radius, 1.f);
	const float HalfFovRad = FMath::DegreesToRadians(Opts.FOV * 0.5f);
	const float Distance = Radius / FMath::Tan(HalfFovRad);

	// 기본 바라보는 방향: 에디터 카메라가 있으면 그것을, 없으면 X축 기준
	FRotator ViewRot = FRotator::ZeroRotator;
	FVector ViewDir = FVector(1, 0, 0);
	for (FLevelEditorViewportClient* VC : GEditor->GetLevelViewportClients())
	{
		if (VC && VC->Viewport && VC->IsPerspective())
		{
			ViewRot = VC->GetViewRotation();
			ViewDir = ViewRot.Vector();
			break;
		}
	}

	const FVector CamLocation = Center - ViewDir * (Distance * 1.2f);
	CaptureActor->SetActorLocation(CamLocation);
	CaptureActor->SetActorRotation(UKismetMathLibrary::FindLookAtRotation(CamLocation, Center));

	// 캡쳐 직전 포스트프로세스 조정
	CaptureComp->PostProcessSettings.bOverride_AutoExposureMethod = true;
	CaptureComp->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	CaptureComp->PostProcessSettings.bOverride_AutoExposureBias = true;
	CaptureComp->PostProcessSettings.AutoExposureBias = 1.0f;
	CaptureComp->PostProcessSettings.bOverride_VignetteIntensity = true;
	CaptureComp->PostProcessSettings.VignetteIntensity = 0.0f;
	CaptureComp->PostProcessSettings.bOverride_BloomIntensity = true;
	CaptureComp->PostProcessSettings.BloomIntensity = 0.0f;

	// 캡쳐
	CaptureComp->CaptureScene();

	// 픽셀 읽기
	FTextureRenderTargetResource* RTRes = RenderTarget->GameThread_GetRenderTargetResource();
	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(Opts.Width * Opts.Height);
	FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
	const bool bOK = RTRes->ReadPixels(Pixels, ReadFlags, FIntRect(0, 0, Opts.Width, Opts.Height));
	if (!bOK)
	{
		CaptureActor->Destroy();
		return false;
	}

	int32 FinalW = Opts.Width;
	int32 FinalH = Opts.Height;
	const TArray<FColor>* SourceForEncode = &Pixels;
	TArray<FColor> Cropped;
	if (Opts.bCropToBounds)
	{
		const FIntRect BoundsRect = ComputeAlphaTightBounds(Pixels, Opts.Width, Opts.Height);
		if (!BoundsRect.IsEmpty())
		{
			CropColors(Pixels, Opts.Width, Opts.Height, BoundsRect, Cropped);
			SourceForEncode = &Cropped;
			FinalW = BoundsRect.Width();
			FinalH = BoundsRect.Height();
		}
	}

	EncodeColorsToPNG(*SourceForEncode, FinalW, FinalH, OutPng);

	// 정리
	CaptureActor->Destroy();
	return OutPng.Num() > 0;
}

#endif // WITH_EDITOR


