#pragma once

#include "CoreMinimal.h"

//해상도 목록
UENUM()
enum class ET3Resolution
{
	W800H600 = 8000600,
	W1024H768 = 10240768,
	W1280H720 = 12800720,
	W1280H800 = 12800800,
	W1280H960 = 12800960,
	W1366H768 = 13660768,
	W1440H900 = 14400900,
	W1600H900 = 16000900,
	W1600H1200 = 16001200,
	W1680H1050 = 16801050,
	W1920H1080 = 19201080,
	W1920H1200 = 19201200,
};

//화면 모드
UENUM()
enum class ET3ScreenMode
{
	Fullscreen,
	WindowedFullscreen,
	Windowed
};

//그래픽 퀄리티
UENUM()
enum class EGraphicQuality
{
	Low = 0,//제일 낮음
	Medium,
	High,
	Epic,
	Cinematic
};

//레벨(맵) 이름
UENUM()
enum class ELevelName
{
	Title = 0 UMETA(DisplayName = "TitleLevel"),
	SelectClass UMETA(DisplayName = "SelectClassLevel"),
};

enum class T3Castle_1
{
	Title = 0 UMETA(DisplayName = "TitleLevel"),
	SelectClass UMETA(DisplayName = "SelectClassLevel"),
};