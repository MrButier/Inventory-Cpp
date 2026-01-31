#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "InventoryMultiplayerTest.generated.h"

class UMPInventoryComponent;

/**
 * Functional Test to verify Inventory Replication.
 * Requires a map with functional testing enabled or running via Session Frontend.
 */
UCLASS()
class DFINVENTORY_API AInventoryMultiplayerTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AInventoryMultiplayerTest();

protected:
	virtual void PrepareTest() override;
	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UMPInventoryComponent> SourceInventory;

	UPROPERTY()
	TObjectPtr<UMPInventoryComponent> TargetInventory;

	UFUNCTION()
	void Server_AddItem();

	UFUNCTION()
	void Server_TransferItem();
	
	void Client_VerifyItem();
	
	bool bItemAdded = false;
	bool bTransferTriggered = false;
	float TimeWaited = 0.0f;
};
