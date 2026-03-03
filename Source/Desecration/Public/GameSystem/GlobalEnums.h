#pragma once

#include "CoreMinimal.h"

//레벨(맵) 이름
UENUM(BlueprintType)
enum class ELevelName : uint8
{
	Title = 0 UMETA(DisplayName = "T3TitleLevel"),
	SelectClass UMETA(DisplayName = "T3SelectClassLevel"),
	Tutorial UMETA(DisplayName = "T3_TutrorialMaps"),
	Town UMETA(DisplayName = "T3Town"),
	Castle_1 UMETA(DisplayName = "T3Castle_1"),
};

//플레이어의 클래스
UENUM()
enum class EPlayerClass
{
	None,
	Warrior,
	Class1,//TODO : 모든 클래스의 영어명 정하기
	Class2
};
