#include "Tests/InventoryMultiplayerTest.h"
#include "Component/MultiInventory.h"
#include "Data/ItemData.h"
#include "Net/UnrealNetwork.h"

AInventoryMultiplayerTest::AInventoryMultiplayerTest()
{
	bReplicates = true;
	TimeLimit = 10.0f; 
}

void AInventoryMultiplayerTest::PrepareTest()
{
	Super::PrepareTest();
	
	if (HasAuthority())
	{
		SourceInventory = NewObject<UMPInventoryComponent>(this, TEXT("SourceInv"));
		SourceInventory->SetMaxItemSlots(5);
		SourceInventory->RegisterComponent();
		SourceInventory->SetIsReplicated(true);
		AddInstanceComponent(SourceInventory);

		TargetInventory = NewObject<UMPInventoryComponent>(this, TEXT("TargetInv"));
		TargetInventory->SetMaxItemSlots(5);
		TargetInventory->RegisterComponent();
		TargetInventory->SetIsReplicated(true);
		AddInstanceComponent(TargetInventory);
	}
}

void AInventoryMultiplayerTest::StartTest()
{
	Super::StartTest();
	if (HasAuthority())
	{
		Server_AddItem();
	}
}

void AInventoryMultiplayerTest::Server_AddItem()
{
	if (!SourceInventory)
	{
		FinishTest(EFunctionalTestResult::Error, "Server: Components not created");
		return;
	}

	UItemData* NewItem = NewObject<UItemData>(this);
	FItemStruct Info;
	Info.ItemName = FString("RepItem");
	Info.Amount = 1;
	NewItem->SetInfo(Info);

	SourceInventory->AddItemToInventory(NewItem);
	bItemAdded = true;
	
	LogStep(ELogVerbosity::Log, "Server: Added Item to Source");
}

void AInventoryMultiplayerTest::Server_TransferItem()
{
	if (!SourceInventory || !TargetInventory) return;

	TArray<int32> Indexes = {0};
	SourceInventory->TransferItemsByIndex(TargetInventory, Indexes);
	bTransferTriggered = true;
	
	LogStep(ELogVerbosity::Log, "Server: Transferred Item to Target");
}

void AInventoryMultiplayerTest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	// Server Logic for Transfer
	{
		if (bItemAdded && !bTransferTriggered)
		{
			TimeWaited += DeltaSeconds;
			if (TimeWaited > 2.0f)
			{
				Server_TransferItem();
			}
		}
		
		if (bTransferTriggered)
		{
			if (TimeWaited > 10.0f) 
			{
				FinishTest(EFunctionalTestResult::Failed, "Timed Out waiting for transfer verification");
			}
			TimeWaited += DeltaSeconds; 
		}
	}
	
	// Client Verification Tick
	if (!HasAuthority())
	{
		Client_VerifyItem();
	}
}

void AInventoryMultiplayerTest::Client_VerifyItem()
{
	TArray<UMPInventoryComponent*> Comps;
	GetComponents(Comps);
	
	UMPInventoryComponent* ClientTarget = nullptr;
		
	for (UMPInventoryComponent* Comp : Comps)
	{
		if (Comp->GetName().Contains("TargetInv")) ClientTarget = Comp;
	}

	if (ClientTarget)
	{
		TArray<UItemData*> TItems = ClientTarget->GetInventoryItems();
		if (TItems.IsValidIndex(0) && TItems[0])
		{
			if (TItems[0]->GetItemInfo().ItemName == FString("RepItem"))
			{
				FinishTest(EFunctionalTestResult::Succeeded, "Client: Item Transferred and Replicated!");
			}
		}
	}
}

// Static Config Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMultiplayerConfigTest, "DFInventory.Multiplayer.Config", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMultiplayerConfigTest::RunTest(const FString& Parameters)
{
	UMPInventoryComponent* Comp = NewObject<UMPInventoryComponent>(GetTransientPackage());
	TestTrue("Component is set to Replicate", Comp->GetIsReplicated());
	return true;
}

#if WITH_EDITOR

	#include "Tests/AutomationEditorCommon.h"
	#include "FileHelpers.h"

	// Integration Test: Zero-Conf Functional Test Runner
	// This test automatically creates a new map, spawns the functional test actor, and runs it.
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryZeroConfTest, "DFInventory.Multiplayer.FullIntegration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	bool FInventoryZeroConfTest::RunTest(const FString& Parameters)
	{
		// 1. Create a new empty map (Memory only)
		UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
		if (!World) return false;

		// 2. Spawn the Functional Test Actor
		AInventoryMultiplayerTest* TestActor = World->SpawnActor<AInventoryMultiplayerTest>();
		if (!TestActor)
		{
			AddError("Failed to spawn Test Actor");
			return false;
		}

		// 3. Start PIE Session
		// FEditorLoadMap intentionally removed as the map is transient and already loaded.
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true)); // Start PIE

		return true;
	}
#endif
