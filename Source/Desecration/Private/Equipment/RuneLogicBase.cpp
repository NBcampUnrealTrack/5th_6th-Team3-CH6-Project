// Fill out your copyright notice in the Description page of Project Settings.


#include "Public/Equipment/RuneLogicBase.h"

void URuneLogicBase::Init(ERuneStatType InType, float InValue)
{
	// 테이블에서 읽은 값을 내 메모리에 저장!
	ConfigStatType = InType;
	ConfigValue = InValue;
}

float URuneLogicBase::GetStatBonus_Implementation(ERuneStatType CheckType) const
{
	// 물어본 스탯 타입(CheckType)이 내가 담당하는 타입(ConfigStatType)과 같으면 값 리턴
	if (ConfigStatType == CheckType)
	{
		return ConfigValue;
	}
	return 0.0f;
}