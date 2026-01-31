#pragma once

#include "CoreMinimal.h"
#include "Component/InventoryComponent.h"
#include "MultiInventory.generated.h"

/**
 * Multiplayer-ready Inventory Component.
 * - Auto-Saves only on Authoritative Server.
 * - Uses Player UniqueNetId for persistence across maps.
 */
UCLASS(Blueprintable, ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent, DisplayName="Multiplayer Inventory Component"))
class DFINVENTORY_API UMPInventoryComponent : public UInventoryComponent
{
	GENERATED_BODY()

public:
	UMPInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

protected:

	virtual bool ShouldAutoSave() const override;
	virtual FString GetSaveID() const override;
};
