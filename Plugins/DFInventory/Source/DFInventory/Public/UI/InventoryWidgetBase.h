#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryWidgetBase.generated.h"

class UInventoryComponent;
class UDFInventorySubsystem;

/**
 * Base class for all inventory-related widgets.
 * Provides helper functions for accessing the inventory system.
 */
UCLASS(Abstract)
class DFINVENTORY_API UInventoryWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Helper to get the Inventory Subsystem from the Game Instance. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI", meta = (WorldContext = "WorldContextObject"))
	UDFInventorySubsystem* GetInventorySubsystem(const UObject* WorldContextObject) const;

	/** Helper to cast an actor component to an Inventory Component. */
	UFUNCTION(BlueprintPure, Category = "Inventory|UI")
	UInventoryComponent* GetInventoryComponent(UActorComponent* Component) const;
};
