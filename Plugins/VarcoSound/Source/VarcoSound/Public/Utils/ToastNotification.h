// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

/**
 * Toast notification utility for VarcoSound plugin
 * Provides centralized notification display functions
 */
namespace VarcoSoundToast
{
	/**
	 * Show a notification toast with an action hyperlink
	 * @param Message The message text to display
	 * @param State The completion state (Success, Fail, None, etc.)
	 * @param ActionText The hyperlink text (e.g., "Open Settings")
	 * @param Action The delegate executed when the hyperlink is clicked
	 */
	inline void ShowNotificationWithAction(
		const FText& Message,
		SNotificationItem::ECompletionState State,
		const FText& ActionText,
		const FSimpleDelegate& Action)
	{
		FNotificationInfo Info(Message);
		Info.bFireAndForget = true;
		Info.FadeOutDuration = 0.5f;
		Info.ExpireDuration = 3.0f;
		Info.bUseThrobber = false;
		Info.HyperlinkText = ActionText;
		Info.Hyperlink = Action;
		
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(State);
		}
	}

	/**
	 * Show a notification toast with customizable completion state
	 * @param Message The message text to display
	 * @param State The completion state (Success, Fail, None, etc.)
	 */
	inline void ShowNotification(const FText& Message, SNotificationItem::ECompletionState State)
	{
		FNotificationInfo Info(Message);
		Info.bFireAndForget = true;
		Info.FadeOutDuration = 0.5f;
		Info.ExpireDuration = 3.0f;
		Info.bUseThrobber = false;
		
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		if (Notification.IsValid())
		{
			Notification->SetCompletionState(State);
		}
	}

	/**
	 * Show a success notification
	 * @param Message The message text to display
	 */
	inline void ShowSuccess(const FText& Message)
	{
		ShowNotification(Message, SNotificationItem::CS_Success);
	}

	/**
	 * Show a warning notification
	 * @param Message The message text to display
	 */
	inline void ShowWarning(const FText& Message)
	{
		ShowNotification(Message, SNotificationItem::CS_Pending);
	}

	/**
	 * Show a warning notification with an action hyperlink
	 */
	inline void ShowWarningWithAction(const FText& Message, const FText& ActionText, const FSimpleDelegate& Action)
	{
		ShowNotificationWithAction(Message, SNotificationItem::CS_Pending, ActionText, Action);
	}

	/**
	 * Show a failure/error notification
	 * @param Message The message text to display
	 */
	inline void ShowError(const FText& Message)
	{
		ShowNotification(Message, SNotificationItem::CS_Fail);
	}

	/**
	 * Show an error notification with an action hyperlink
	 */
	inline void ShowErrorWithAction(const FText& Message, const FText& ActionText, const FSimpleDelegate& Action)
	{
		ShowNotificationWithAction(Message, SNotificationItem::CS_Fail, ActionText, Action);
	}

	/**
	 * Show an info notification
	 * @param Message The message text to display
	 */
	inline void ShowInfo(const FText& Message)
	{
		ShowNotification(Message, SNotificationItem::CS_None);
	}
}

