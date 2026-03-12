#include "GameSystem/T3SaveLostMoney.h"

void UT3SaveLostMoney::ResetGameData()
{
	LostMoneyList.Empty();
}

void UT3SaveLostMoney::AddLostMoney(FLostMoney NewLostMoney)
{
	//LostMoneyList내 객체 개수를 키로 잡은 다음 중복이 있다면 1씩 더해서 확인
	int32 Key = LostMoneyList.Num();
	while (true)
	{
		if (!LostMoneyList.Contains(Key))
		{
			break;
		}
		++Key;
	}

	LostMoneyList.Emplace(Key, NewLostMoney);
}

void UT3SaveLostMoney::RegainLostMoney(const int32 LostMoneyID)
{
	LostMoneyList.Remove(LostMoneyID);
}
