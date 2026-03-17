// Copyright Epic Games, Inc. All Rights Reserved.

#include "VarcoSound.h"
#include "VarcoSoundStyle.h"
#include "VarcoSoundCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SWindow.h"
#include "Tabs/SGeneratorTab.h"
#include "Tabs/SAutoLoopTab.h"
#include "Tabs/SAutoVariationTab.h"
#include "Tabs/SMonsterVoiceTab.h"
#include "Tabs/SBackgroundMusicTab.h"
#include "Utils/AudioUtils.h"
#include "Utils/VarcoSoundSettings.h"
#include "Utils/ToastNotification.h"
#include "Widgets/Images/SImage.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"

static const FName VarcoSoundTabName("VarcoSound"); // Plugin tab name

#define LOCTEXT_NAMESPACE "FVarcoSoundModule"  // Localization namespace definition

void FVarcoSoundModule::StartupModule()
{
    // Function called when the module starts
    
    FVarcoSoundStyle::Initialize();      // Initialize style
    FVarcoSoundStyle::ReloadTextures();  // Reload textures

    FVarcoSoundCommands::Register();     // Register commands
    
    PluginCommands = MakeShareable(new FUICommandList);  // Create UI command list

    // Action mapping for opening plugin window command
    PluginCommands->MapAction(
        FVarcoSoundCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FVarcoSoundModule::PluginButtonClicked),
        FCanExecuteAction());

    // Set callback for menu registration
    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FVarcoSoundModule::RegisterMenus));
    
    // Register tab spawner - display plugin window as a tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(VarcoSoundTabName, FOnSpawnTab::CreateRaw(this, &FVarcoSoundModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("FVarcoSoundTabTitle", "VARCO Sound"))
        .SetMenuType(ETabSpawnerMenuType::Hidden)
        .SetIcon(FSlateIcon(FVarcoSoundStyle::GetStyleSetName(), "VarcoSound.Icon128"));
}

void FVarcoSoundModule::ShutdownModule()
{
    // Function called when the module shuts down - perform cleanup

    // Clean up audio resources - perform first for live coding safety
    FAudioUtils::Stop();
    FAudioUtils::SetSound(nullptr);

    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);

    FVarcoSoundStyle::Shutdown();
    FVarcoSoundCommands::Unregister();

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(VarcoSoundTabName);
}

TSharedRef<SDockTab> FVarcoSoundModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    // Create tabs and initialize each tab instance
    GeneratorTab = SNew(SGeneratorTab);
    AutoLoopTab = SNew(SAutoLoopTab);
    AutoVariationTab = SNew(SAutoVariationTab);
    MonsterVoiceTab = SNew(SMonsterVoiceTab);
    BackgroundMusicTab = SNew(SBackgroundMusicTab);
    
    return SNew(SDockTab)  // Create new docking tab
        .TabRole(ETabRole::NomadTab)  // This tab is a movable nomad tab
        [
            SNew(SVerticalBox)  // Create vertical box layout
            
            // Banner header with buttons
            +SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 0.0f, 0.0f, 4.0f)
            [
                SNew(SHorizontalBox)
                // Banner image
                +SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(8.0f, 4.0f)
                [
                    SNew(SBox)
                    .HeightOverride(24.0f)
                    [
                        SNew(SImage)
                        .Image(FVarcoSoundStyle::Get().GetBrush("VarcoSound.Banner"))
                    ]
                ]
                // Spacer
                +SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNullWidget::NullWidget
                ]
                // Help button
                +SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(4.0f, 4.0f)
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                    .ContentPadding(FMargin(6.0f))
                    .OnClicked_Lambda([]() -> FReply
                    {
                        FPlatformProcess::LaunchURL(TEXT("https://forms.gle/triRfsxgotKr8MsWA"), nullptr, nullptr);
                        return FReply::Handled();
                    })
                    .ToolTipText(LOCTEXT("VarcoSoundFeedbackTooltip", "Report bugs or request features"))
                    [
                        SNew(SImage)
                        .Image(FAppStyle::Get().GetBrush("Icons.Help"))
                    ]
                ]
                // Settings button
                +SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Center)
                .Padding(4.0f, 4.0f, 8.0f, 4.0f)
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                    .ContentPadding(FMargin(6.0f))
                    .OnClicked(FOnClicked::CreateRaw(this, &FVarcoSoundModule::OnSettingsButtonClicked))
                    .ToolTipText(LOCTEXT("VarcoSoundSettingsTooltip", "Open VarcoSound settings"))
                    [
                        SNew(SImage)
                        .Image(FAppStyle::Get().GetBrush("Icons.Settings"))
                    ]
                ]
            ]
            
            // Tab buttons area
            +SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(SWrapBox)  // WrapBox for automatic line wrapping based on window size
                .UseAllottedSize(true)
                +SWrapBox::Slot()  // First tab button
                .Padding(2.0f)
                [
                    SAssignNew(TabButtons[0], SCheckBox)
                    .Style(FAppStyle::Get(), "DetailsView.SectionButton")
                    .OnCheckStateChanged_Raw(this, &FVarcoSoundModule::OnTabCheckChanged, 0)
                    .IsChecked_Raw(this, &FVarcoSoundModule::GetTabCheckState, 0)
                    .Padding(FMargin(12.0f, 4.0f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Sound Effect")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    ]
                ]
                +SWrapBox::Slot()  // Second tab button
                .Padding(2.0f)
                [
                    SAssignNew(TabButtons[1], SCheckBox)
                    .Style(FAppStyle::Get(), "DetailsView.SectionButton")
                    .OnCheckStateChanged_Raw(this, &FVarcoSoundModule::OnTabCheckChanged, 1)
                    .IsChecked_Raw(this, &FVarcoSoundModule::GetTabCheckState, 1)
                    .Padding(FMargin(12.0f, 4.0f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Auto Loop")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    ]
                ]
                +SWrapBox::Slot()  // Third tab button
                .Padding(2.0f)
                [
                    SAssignNew(TabButtons[2], SCheckBox)
                    .Style(FAppStyle::Get(), "DetailsView.SectionButton")
                    .OnCheckStateChanged_Raw(this, &FVarcoSoundModule::OnTabCheckChanged, 2)
                    .IsChecked_Raw(this, &FVarcoSoundModule::GetTabCheckState, 2)
                    .Padding(FMargin(12.0f, 4.0f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Variation")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    ]
                ]
                +SWrapBox::Slot()  // Fourth tab button
                .Padding(2.0f)
                [
                    SAssignNew(TabButtons[3], SCheckBox)
                    .Style(FAppStyle::Get(), "DetailsView.SectionButton")
                    .OnCheckStateChanged_Raw(this, &FVarcoSoundModule::OnTabCheckChanged, 3)
                    .IsChecked_Raw(this, &FVarcoSoundModule::GetTabCheckState, 3)
                    .Padding(FMargin(12.0f, 4.0f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Monster Voice")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    ]
                ]
                +SWrapBox::Slot()  // Fifth tab button
                .Padding(2.0f)
                [
                    SAssignNew(TabButtons[4], SCheckBox)
                    .Style(FAppStyle::Get(), "DetailsView.SectionButton")
                    .OnCheckStateChanged_Raw(this, &FVarcoSoundModule::OnTabCheckChanged, 4)
                    .IsChecked_Raw(this, &FVarcoSoundModule::GetTabCheckState, 4)
                    .Padding(FMargin(12.0f, 4.0f))
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(TEXT("Music")))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    ]
                ]
            ]
            +SVerticalBox::Slot()  // 두 번째 슬롯: 탭 콘텐츠 영역
            .FillHeight(1.f)  // 남은 공간 모두 차지
            [
                SAssignNew(ContentSwitcher, SWidgetSwitcher)  // 위젯 전환기 생성 및 변수에 할당
                .WidgetIndex(0)  // 기본적으로 첫 번째 탭 표시
                + SWidgetSwitcher::Slot()  // 첫 번째 탭 콘텐츠
                [
                    GeneratorTab.ToSharedRef()  // 생성기 탭
                ]
                + SWidgetSwitcher::Slot()  // 두 번째 탭 콘텐츠
                [
                    AutoLoopTab.ToSharedRef()  // 자동 루프 탭
                ]
                + SWidgetSwitcher::Slot()  // 세 번째 탭 콘텐츠
                [
                    AutoVariationTab.ToSharedRef()  // 자동 변형 탭
                ]
                + SWidgetSwitcher::Slot()  // 네 번째 탭 콘텐츠
                [
                    MonsterVoiceTab.ToSharedRef()  // 몬스터 음성 탭
                ]
                + SWidgetSwitcher::Slot()  // 다섯 번째 탭 콘텐츠
                [
                    BackgroundMusicTab.ToSharedRef()  // 배경음악 생성 탭
                ]
            ]
        ];
}

ECheckBoxState FVarcoSoundModule::GetTabCheckState(int32 TabIndex) const
{
	return (CurrentTabIndex == TabIndex) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FVarcoSoundModule::OnTabCheckChanged(ECheckBoxState NewState, int32 TabIndex)
{
	if (NewState == ECheckBoxState::Checked && ContentSwitcher.IsValid())
	{
		// Pause any playback in all tabs when switching (preserve positions)
		if (GeneratorTab.IsValid())
		{
			GeneratorTab->PauseAllPlayback();
		}
		if (AutoLoopTab.IsValid())
		{
			AutoLoopTab->PauseAllPlayback();
		}
		if (AutoVariationTab.IsValid())
		{
			AutoVariationTab->PauseAllPlayback();
		}
		if (MonsterVoiceTab.IsValid())
		{
			MonsterVoiceTab->PauseAllPlayback();
		}
		if (BackgroundMusicTab.IsValid())
		{
			BackgroundMusicTab->PauseAllPlayback();
		}

		CurrentTabIndex = TabIndex;
		ContentSwitcher->SetActiveWidgetIndex(TabIndex);
		
		// Invalidate all tab buttons to update their visual state
		for (int32 i = 0; i < 5; i++)
		{
			if (TabButtons[i].IsValid())
			{
				TabButtons[i]->Invalidate(EInvalidateWidget::Paint);
			}
		}
	}
}

FReply FVarcoSoundModule::OnSettingsButtonClicked()
{
	ShowSettingsWindow();
	return FReply::Handled();
}

void FVarcoSoundModule::ShowSettingsForApiKeyIssue(const FText& Message)
{
	ApiKeyIssueMessage = Message;
	bShowApiKeyIssueMessage = true;

	// Opens or brings to front
	ShowSettingsWindow();

	// Focus API key input for quick correction
	if (SettingsApiKeyTextBox.IsValid())
	{
		FSlateApplication::Get().SetKeyboardFocus(SettingsApiKeyTextBox, EFocusCause::SetDirectly);
	}
}

void FVarcoSoundModule::ShowSettingsWindow()
{
	if (SettingsWindow.IsValid())
	{
		if (TSharedPtr<SWindow> ExistingWindow = SettingsWindow.Pin())
		{
			ExistingWindow->BringToFront(true);
			FSlateApplication::Get().SetAllUserFocus(ExistingWindow, EFocusCause::SetDirectly);
		}
		return;
	}

	const FString CurrentKey = VarcoSoundSettings::LoadApiKey();
	const FString OutputRootDir = VarcoSoundSettings::LoadOutputRootDirectory();

	bSettingsShowApiKey = false;

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("VarcoSoundSettingsTitle", "VarcoSound Settings"))
		.ClientSize(FVector2D(520.f, 490.f))
		.SupportsMaximize(false)
		.SupportsMinimize(false)
		.IsTopmostWindow(true);

	Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>&)
	{
		SettingsApiKeyTextBox.Reset();
		SettingsRecordingDirTextBox.Reset();
		SettingsWindow.Reset();
		bShowApiKeyIssueMessage = false;
		ApiKeyIssueMessage = FText::GetEmpty();
	}));

	// Load Icon128.png directly
	FString IconPath = IPluginManager::Get().FindPlugin("VarcoSound")->GetBaseDir() / TEXT("Resources/Icon128.png");
	
	Window->SetContent(
		SNew(SBorder)
		.Padding(16.f)
		[
			SNew(SVerticalBox)

			// Banner
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.Padding(FMargin(12.f))
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SImage)
						.Image(new FSlateImageBrush(IconPath, FVector2D(48.f, 48.f)))
						.DesiredSizeOverride(FVector2D(48.f, 48.f))
					]
					+ SHorizontalBox::Slot()
					.Padding(16.f, 0.f, 0.f, 0.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("VarcoSoundSettingsBannerTitle", "VARCO Sound"))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
					]
				]
			]

			// API Key section title
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 16.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("VarcoSoundSettingsApiKeyLabel", "Enter your API key"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			]

			// API Key input
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SAssignNew(SettingsApiKeyTextBox, SEditableTextBox)
					.HintText(LOCTEXT("VarcoSoundSettingsApiKeyHint", "OPENAPI_KEY"))
					.Text(FText::FromString(CurrentKey))
					.IsPassword(true)
					.OnTextChanged_Lambda([this](const FText&)
					{
						// Hide guidance when user starts editing
						bShowApiKeyIssueMessage = false;
						ApiKeyIssueMessage = FText::GetEmpty();
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.f, 0.f, 0.f, 0.f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
					.ContentPadding(FMargin(4.f))
					.OnClicked_Lambda([this]() -> FReply
					{
						bSettingsShowApiKey = !bSettingsShowApiKey;
						if (SettingsApiKeyTextBox.IsValid())
						{
							SettingsApiKeyTextBox->SetIsPassword(!bSettingsShowApiKey);
						}
						return FReply::Handled();
					})
					[
						SNew(SImage)
						.Image_Lambda([this]() -> const FSlateBrush*
						{
							return bSettingsShowApiKey
								? FAppStyle::Get().GetBrush("Icons.Hidden")
								: FAppStyle::Get().GetBrush("Icons.Visible");
						})
					]
				]
			]

			// API Key guidance banner (shown when API key is missing/invalid/expired)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.Visibility_Lambda([this]()
				{
					return bShowApiKeyIssueMessage ? EVisibility::Visible : EVisibility::Collapsed;
				})
				.Padding(FMargin(10.f, 8.f))
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
				.BorderBackgroundColor(FLinearColor(0.35f, 0.0f, 0.0f, 0.18f))
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.25f, 0.25f, 1.0f)))
					.Text_Lambda([this]()
					{
						if (!ApiKeyIssueMessage.IsEmpty())
						{
							return ApiKeyIssueMessage;
						}
						return LOCTEXT("VarcoSoundApiKeyIssueDefault", "Please enter your API key in Settings.");
					})
				]
			]

			// Download locations title
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 16.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("VarcoSoundSettingsDownloadLabel", "Download locations"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			]

			// Output root directory
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VarcoSoundSettingsOutputRootLabel", "Output root"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 4.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					[
						SAssignNew(SettingsRecordingDirTextBox, SEditableTextBox)
						.Text(FText::FromString(OutputRootDir))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(6.f, 0.f, 0.f, 0.f)
					[
						SNew(SButton)
						.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
						.ContentPadding(FMargin(4.f))
						.OnClicked_Lambda([this]() -> FReply
						{
							OpenDirectoryPicker(SettingsRecordingDirTextBox, LOCTEXT("VarcoSoundOutputRootDialogTitle", "Select output root directory"));
							return FReply::Handled();
						})
						[
							SNew(STextBlock)
							.Text(LOCTEXT("VarcoSoundSettingsBrowse", "Browse"))
						]
					]
				]
			]

			// Debug logging toggle
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 16.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("VarcoSoundSettingsDebugLabel", "Developer Options"))
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			]
			
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([]() -> ECheckBoxState
				{
					return VarcoSoundSettings::IsDebugLoggingEnabled() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([](ECheckBoxState NewState)
				{
					VarcoSoundSettings::SetDebugLogging(NewState == ECheckBoxState::Checked);
				})
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VarcoSoundSettingsDebugLogging", "Enable debug logging (for development)"))
				]
			]

			// Documentation button
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 20.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("VarcoSoundSettingsDocumentation", "📖 Documentation"))
				.ToolTipText(LOCTEXT("VarcoSoundSettingsDocumentationTooltip", "Open VARCO Sound Plugin Documentation"))
				.OnClicked_Lambda([]() -> FReply
				{
					FPlatformProcess::LaunchURL(TEXT("https://api.varco.ai/ko/docs/plugin"), nullptr, nullptr);
					return FReply::Handled();
				})
			]

			// Action buttons
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0.f, 16.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("VarcoSoundSettingsSetDefault", "Set Default"))
					.OnClicked_Lambda([this]() -> FReply
					{
						const FString DefaultOutputRootDir = VarcoSoundSettings::GetDefaultOutputRootDirectory();
						
						if (SettingsRecordingDirTextBox.IsValid())
						{
							SettingsRecordingDirTextBox->SetText(FText::FromString(DefaultOutputRootDir));
						}
						
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(SButton)
					.Text(LOCTEXT("VarcoSoundSettingsCancel", "Cancel"))
					.OnClicked_Lambda([this]() -> FReply
					{
						CloseSettingsWindow();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("VarcoSoundSettingsSave", "Save"))
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.OnClicked_Lambda([this]() -> FReply
					{
						const FString NewKey = SettingsApiKeyTextBox.IsValid() ? SettingsApiKeyTextBox->GetText().ToString().TrimStartAndEnd() : FString();
						const FString NewOutputRootDir = SettingsRecordingDirTextBox.IsValid() ? SettingsRecordingDirTextBox->GetText().ToString().TrimStartAndEnd() : FString();

						VarcoSoundSettings::SaveApiKey(NewKey);
						VarcoSoundSettings::SaveOutputRootDirectory(NewOutputRootDir);

						IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
						PlatformFile.CreateDirectoryTree(*VarcoSoundSettings::LoadOutputRootDirectory());

						ApplyApiKeyToAllTabs(NewKey);
						bShowApiKeyIssueMessage = false;
						ApiKeyIssueMessage = FText::GetEmpty();
						CloseSettingsWindow();
						return FReply::Handled();
					})
				]
			]
		]
	);

	SettingsWindow = Window;
	FSlateApplication::Get().AddWindow(Window);
}

void FVarcoSoundModule::CloseSettingsWindow()
{
	if (SettingsWindow.IsValid())
	{
		if (TSharedPtr<SWindow> Window = SettingsWindow.Pin())
		{
			FSlateApplication::Get().RequestDestroyWindow(Window.ToSharedRef());
		}
	}

	SettingsApiKeyTextBox.Reset();
	SettingsRecordingDirTextBox.Reset();
	SettingsWindow.Reset();
}

void FVarcoSoundModule::ApplyApiKeyToAllTabs(const FString& InApiKey)
{
	if (GeneratorTab.IsValid())
	{
		GeneratorTab->SetApiKey(InApiKey);
	}
	if (AutoLoopTab.IsValid())
	{
		AutoLoopTab->SetApiKey(InApiKey);
	}
	if (AutoVariationTab.IsValid())
	{
		AutoVariationTab->SetApiKey(InApiKey);
	}
	if (MonsterVoiceTab.IsValid())
	{
		MonsterVoiceTab->SetApiKey(InApiKey);
	}
	if (BackgroundMusicTab.IsValid())
	{
		BackgroundMusicTab->SetApiKey(InApiKey);
	}
}

void FVarcoSoundModule::PluginButtonClicked()
{
    // 플러그인 버튼 클릭 시 호출되는 함수 - 플러그인 탭 열기
    FGlobalTabmanager::Get()->TryInvokeTab(VarcoSoundTabName);
}

void FVarcoSoundModule::RegisterMenus()
{
    // 메뉴 등록 함수
    FToolMenuOwnerScoped OwnerScoped(this);  // 메뉴 소유자 설정

    {
        // 레벨 에디터 메인 메뉴의 Window 메뉴에 항목 추가
        UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
        {
            FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
            Section.AddMenuEntryWithCommandList(FVarcoSoundCommands::Get().OpenPluginWindow, PluginCommands);
        }
    }

    {
        // 레벨 에디터 툴바에 버튼 추가
        UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
        {
            FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
            {
                FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FVarcoSoundCommands::Get().OpenPluginWindow));
                Entry.SetCommandList(PluginCommands);
            }
        }
    }
}

void FVarcoSoundModule::OpenDirectoryPicker(const TSharedPtr<SEditableTextBox>& TargetTextBox, const FText& DialogTitle)
{
	if (!TargetTextBox.IsValid())
	{
		return;
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	FString InitialDirectory = TargetTextBox->GetText().ToString();
	InitialDirectory.TrimStartAndEndInline();
	if (InitialDirectory.IsEmpty())
	{
		InitialDirectory = FPaths::ProjectContentDir();
	}

	InitialDirectory = FPaths::ConvertRelativePathToFull(InitialDirectory);

	void* ParentWindowHandle = nullptr;
	if (SettingsWindow.IsValid())
	{
		if (TSharedPtr<SWindow> ParentWindow = SettingsWindow.Pin())
		{
			if (ParentWindow->GetNativeWindow().IsValid())
			{
				ParentWindowHandle = ParentWindow->GetNativeWindow()->GetOSWindowHandle();
			}
		}
	}

	FString ChosenDirectory;
	if (DesktopPlatform->OpenDirectoryDialog(ParentWindowHandle, DialogTitle.ToString(), InitialDirectory, ChosenDirectory))
	{
		FPaths::NormalizeDirectoryName(ChosenDirectory);
		TargetTextBox->SetText(FText::FromString(ChosenDirectory));
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FVarcoSoundModule, VarcoSound)  // 모듈 구현 매크로 - 엔진에 이 모듈을 등록