// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SAudioResultView;
class SVerticalBox;

/**
 * 출력 섹션 위젯 (제목 + SAudioResultView)
 * Reusable component that extracts the common output section pattern used in all tabs (title + SAudioResultView)
 */
class VARCOSOUND_API SOutputSection : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOutputSection)
		: _Title(FText::FromString(TEXT("Output")))
		, _ShowTitle(true)
	{}
		SLATE_ARGUMENT(FText, Title)
		SLATE_ARGUMENT(bool, ShowTitle)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/**
	 * AudioResultView 가져오기
	 */
	TSharedPtr<SAudioResultView> GetAudioResultView() const { return AudioResultView; }

	/**
	 * 컨테이너에 추가 위젯 삽입 (AudioResultView 위에)
	 */
	void AddCustomWidget(TSharedRef<SWidget> Widget);

	/**
	 * 내용 초기화
	 */
	void ClearContent();

private:
	TSharedPtr<SAudioResultView> AudioResultView;
	TSharedPtr<SVerticalBox> Container;
	TSharedPtr<SVerticalBox> CustomWidgetContainer;  // AudioResultView 위에 커스텀 위젯 추가용
};

