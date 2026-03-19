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
	Taoist,
	Valkyrie
};

//현재 열려있는 인벤토리 위젯의 타입
UENUM(BlueprintType)
enum class ET3InventoryContext : uint8
{
	MainInventory    UMETA(DisplayName = "메인 인벤토리"),
	Shop             UMETA(DisplayName = "상점"),
	UpgradeStation   UMETA(DisplayName = "강화소")
};