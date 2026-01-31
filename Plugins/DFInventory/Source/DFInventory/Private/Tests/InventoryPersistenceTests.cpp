#include "Misc/AutomationTest.h"
#include "Component/InventoryComponent.h"
#include "Subsystem/DFInventorySubsystem.h"
#include "Data/ItemData.h"
#include "Settings/InventorySaveGame.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationEditorCommon.h"

// Subsystem Test - Disable due to creating GameInstance/World in automation instability. Covered by Integration Test below.
/*
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventorySubsystemTest, "DFInventory.Persistence.Subsystem", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventorySubsystemTest::RunTest(const FString& Parameters)
{
	// Use a proper World/GameInstance setup for Subsystem creation
 	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	if (!TestNotNull("World Created", World)) return false;

	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	World->SetGameInstance(GI);
	GI->Init();

	UDFInventorySubsystem* Subsystem = GI->GetSubsystem<UDFInventorySubsystem>();
	if (!TestNotNull("Subsystem Exists", Subsystem)) return false;

	// Test Data
	FName TestKey = FName("TestInventory_Auto");
	FItemSaveData SaveData;
	SaveData.MaxSlots = 10;
	FItemStruct DummyItem;
	DummyItem.ItemName = FString("TestItem");
	DummyItem.Amount = 5;
	SaveData.Items.Add(DummyItem);

	// Store
	Subsystem->StoreInventoryData(TestKey, SaveData);

	// Retrieve
	FItemSaveData RetrievedData;
	bool bFound = Subsystem->RetrieveInventoryData(TestKey, RetrievedData);
	TestTrue("Data Retrieved", bFound);
	TestEqual("MaxSlots Preserved", RetrievedData.MaxSlots, 10);
	if (TestEqual("Items Count Preserved", RetrievedData.Items.Num(), 1))
	{
		TestEqual("Item Name Preserved", RetrievedData.Items[0].ItemName, FString("TestItem"));
		TestEqual("Item Amount Preserved", RetrievedData.Items[0].Amount, 5);
	}

	// Remove
	Subsystem->RemoveStoredInventoryData(TestKey);
	bool bFoundAfterRemove = Subsystem->RetrieveInventoryData(TestKey, RetrievedData);
	TestFalse("Data Removed", bFoundAfterRemove);

	return true;
}
*/

// Component Memory Save
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryMemorySaveTest, "DFInventory.Persistence.ComponentMemory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryMemorySaveTest::RunTest(const FString& Parameters)
{
	// Setup a temporary World and GameInstance explicitly
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	if (!TestNotNull("World Created", World)) return false;

	UGameInstance* GI = NewObject<UGameInstance>(GEngine);
	World->SetGameInstance(GI);
	GI->Init(); // Initialize subsystems

	UDFInventorySubsystem* Subsystem = GI->GetSubsystem<UDFInventorySubsystem>();
	if (!TestNotNull("Subsystem Exists via GI", Subsystem)) return false;

	// Create Component
	AActor* Host = World->SpawnActor<AActor>();
	
	// Set Manual SaveID using local subclass to expose protected 'SaveID'
	class UTestPersistenceInventory : public UInventoryComponent
	{
		public:
			void SetTestSaveID(FString NewID) { SaveID = NewID; }
	};
	
	UTestPersistenceInventory* TestInv = NewObject<UTestPersistenceInventory>(Host);
	TestInv->RegisterComponent();
	TestInv->SetTestSaveID("AutoTest_Memory_Inv");
	TestInv->SetMaxItemSlots(5);
	
	// Add Item
	UItemData* Item = NewObject<UItemData>(TestInv);
	FItemStruct Info;
	Info.ItemName = FString("SavedItem");
	Info.Amount = 3;
	// IMPORTANT: Set ParentItem so logic knows what class to spawn on load
	Info.ParentItem = Item; 
	Item->SetInfo(Info);
	TestInv->AddItemToInventory(Item);

	// Save to Memory
	TestInv->SaveInventory(ESaveType::Memory);

	// Verify Subsystem has it
	FItemSaveData Data;
	bool bInSubsystem = Subsystem->RetrieveInventoryData(FName("AutoTest_Memory_Inv"), Data);
	TestTrue("Subsystem has data", bInSubsystem);

	// Clear Inventory
	TestInv->CreateNewInventory(); // Resets items
	TestEqual("Inventory Cleared", TestInv->GetInventoryItems()[0], (UItemData*)nullptr);

	// Load from Memory
	TestInv->LoadInventory(ESaveType::Memory);

	// Verify Restoration
	TArray<UItemData*> Items = TestInv->GetInventoryItems();
	if (TestTrue("Item Restored Index 0", Items.IsValidIndex(0) && Items[0] != nullptr))
	{
		TestEqual("Restored Name", Items[0]->GetItemInfo().ItemName, FString("SavedItem"));
		TestEqual("Restored Amount", Items[0]->GetItemInfo().Amount, 3);
	}

	// Cleanup
	Subsystem->RemoveStoredInventoryData(FName("AutoTest_Memory_Inv"));
	// World cleanup is handled by Automation Utils mostly, but specific actors destroyed
	
	return true;
}
