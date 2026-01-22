#include "Public/Item/Component/InventoryComponent.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UInventoryComponent::AddItem(FName ItemName)
{
}

void UInventoryComponent::UseItem(int32 SlotIndex)
{
}

void UInventoryComponent::DropItem(int32 SlotIndex)
{
}
