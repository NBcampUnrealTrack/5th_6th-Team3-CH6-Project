// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

// Toast notification utilities
namespace VarcoSoundToast
{
	/** Show success toast notification */
	void ShowSuccess(const FText& Message);
	
	/** Show warning toast notification */
	void ShowWarning(const FText& Message);
	
	/** Show error toast notification */
	void ShowError(const FText& Message);
	
	/** Show info toast notification */
	void ShowInfo(const FText& Message);
}

class FToolBarBuilder;
class FMenuBuilder;
class SGeneratorTab;
class SAutoLoopTab;
class SAutoVariationTab;
class SMonsterVoiceTab;
class SBackgroundMusicTab;
class SEditableTextBox;
class SWindow;
struct FSlateDynamicImageBrush;


class FVarcoSoundModule : public IModuleInterface
{
public:
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
    /** This function will be bound to Command (by default it will bring up plugin window) */
    void PluginButtonClicked();

	/**
	 * Open the Settings window (or bring it to front) and show an API key guidance message.
	 * Intended to be called when API key is missing/invalid/expired.
	 */
	void ShowSettingsForApiKeyIssue(const FText& Message);
    
private:
    void RegisterMenus();
    TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
    
    // Tab instances
    TSharedPtr<SGeneratorTab> GeneratorTab;
    TSharedPtr<SAutoLoopTab> AutoLoopTab;
    TSharedPtr<SAutoVariationTab> AutoVariationTab;
    TSharedPtr<SMonsterVoiceTab> MonsterVoiceTab;
    TSharedPtr<SBackgroundMusicTab> BackgroundMusicTab;
    
    // Widget switcher
    TSharedPtr<class SWidgetSwitcher> ContentSwitcher;
    
    // Tab switch processing functions
    ECheckBoxState GetTabCheckState(int32 TabIndex) const;
    void OnTabCheckChanged(ECheckBoxState NewState, int32 TabIndex);
    
    // Currently selected tab index
    int32 CurrentTabIndex = 0;
    
    // Tab buttons
    TSharedPtr<class SCheckBox> TabButtons[5];
    
private:
    TSharedPtr<class FUICommandList> PluginCommands;

    FReply OnSettingsButtonClicked();
    void ShowSettingsWindow();
    void CloseSettingsWindow();
    void ApplyApiKeyToAllTabs(const FString& InApiKey);

    TWeakPtr<SWindow> SettingsWindow;
    TSharedPtr<SEditableTextBox> SettingsApiKeyTextBox;
    TSharedPtr<SEditableTextBox> SettingsRecordingDirTextBox;
    TSharedPtr<FSlateDynamicImageBrush> SettingsBannerBrush;
    bool bSettingsShowApiKey = false;
	bool bShowApiKeyIssueMessage = false;
	FText ApiKeyIssueMessage;

    void OpenDirectoryPicker(const TSharedPtr<SEditableTextBox>& TargetTextBox, const FText& DialogTitle);
};
