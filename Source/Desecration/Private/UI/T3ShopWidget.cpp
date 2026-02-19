#include "UI/T3ShopWidget.h"

void UT3ShopWidget::Init(UT3InventoryComponent* InventoryComp, UT3ShopComponent* ShopComp, AT3CharacterBase* Character)
{
	InventoryComponent = InventoryComp;
	ShopComponent = ShopComp;
	PlayerCharacter = Character;
}
