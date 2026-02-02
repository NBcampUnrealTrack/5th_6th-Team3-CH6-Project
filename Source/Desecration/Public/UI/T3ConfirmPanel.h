#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T3ConfirmPanel.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClickConfirmButtonAction);

UCLASS()
class DESECRATION_API UT3ConfirmPanel : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
public:
	//패널 띄우기
	//확인 버튼 동작을 등록하려면 호출마다 OnClickConfirmButtonAction에 바인딩 필요
	UFUNCTION(BlueprintCallable)
	void ShowConfirmPanel(UPARAM() const FString& Content, const bool bNeedCancelButton = true);
	
private:
	//확인 버튼
	UFUNCTION()
	void OnClickConfirmButton();
	
	//취소 버튼
	UFUNCTION()
	void OnClickCancelButton();
	
public:
	//확인 버튼을 누를 때 실행할 내용
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnClickConfirmButtonAction OnClickConfirmButtonAction;

private:
	//패널 내용
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UTextBlock> ContentText;
	
	//확인 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> ConfirmButton;
	
	//취소 버튼
	UPROPERTY(meta = (AllowPrivateAccess = true, BindWidget))
	TObjectPtr<UButton> CancelButton;
};
