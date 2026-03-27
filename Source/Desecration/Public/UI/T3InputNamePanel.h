#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3InputNamePanel.generated.h"

class UButton;
class UEditableTextBox;
class UT3SelectClassWidget;

UCLASS()
class DESECRATION_API UT3InputNamePanel : public UUserWidget
{
	GENERATED_BODY()
	
	friend UT3SelectClassWidget;

protected:
	virtual void NativeOnInitialized() override;
	
private:
	//이름 입력칸의 변화
	UFUNCTION()
	void OnTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
	//확인 버튼
	UFUNCTION()
	void OnClickConfirmButton();
	
	//취소 버튼
	UFUNCTION()
	void OnClickCancelButton();
	
	//이름 입력 칸
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UEditableTextBox> InputNameBox;
	
	//확인 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> ConfirmButton;
	
	//취소 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> CancelButton;
	
	//클래스 선택 위젯
	UPROPERTY()
	TWeakObjectPtr<UT3SelectClassWidget> SelectClassWidget;
};
