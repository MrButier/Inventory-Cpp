#include "UI/InventoryWidgetBase.h"
#include "Subsystem/DFInventorySubsystem.h"
#include "Component/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"

UDFInventorySubsystem* UInventoryWidgetBase::GetInventorySubsystem(const UObject* WorldContextObject) const
{
	const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<UDFInventorySubsystem>() : nullptr;
}

UInventoryComponent* UInventoryWidgetBase::GetInventoryComponent(UActorComponent* Component) const
{ return Cast<UInventoryComponent>(Component); }
