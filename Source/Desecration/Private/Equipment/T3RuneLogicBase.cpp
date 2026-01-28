// Fill out your copyright notice in the Description page of Project Settings.


#include "Equipment/T3RuneLogicBase.h"

void UT3RuneLogicBase::Init(ET3RuneStatType InType, float InValue)
{
	// 테이블에서 읽은 값을 내 메모리에 저장!
	ConfigStatType = InType;
	ConfigValue = InValue;
}

float UT3RuneLogicBase::GetStatBonus_Implementation(ET3RuneStatType CheckType) const
{
	// 물어본 스탯 타입(CheckType)이 내가 담당하는 타입(ConfigStatType)과 같으면 값 리턴
	if (ConfigStatType == CheckType)
	{
		return ConfigValue;
	}
	return 0.0f;
}