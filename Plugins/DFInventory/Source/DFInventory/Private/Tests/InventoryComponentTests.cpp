#include "Component/InventoryComponent.h"
#include "Data/ItemData.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

// Expose protected members for testing
class UTestInventory : public UInventoryComponent
{
public:
	using UInventoryComponent::InventoryItems;
	using UInventoryComponent::CreateNewInventory;
	using UInventoryComponent::FindEmptySlot;
	using UInventoryComponent::FindStackableItem;
	using UInventoryComponent::IsInventoryFull;
};

// --- Helper Functions ---
UItemData* CreateTestItem(UObject* Outer, int32 Amount = 1, int32 MaxAmount = 10)
{
	UItemData* Item = NewObject<UItemData>(Outer);
	FItemStruct Info;
	Info.ParentItem = Item; // Self-reference for type equality
	Info.MaxAmount = MaxAmount;
	Info.Amount = Amount;
	Item->SetInfo(Info);
	return Item;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryCoreTest, "DFInventory.Core.Basics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryCoreTest::RunTest(const FString& Parameters)
{
	UTestInventory* Inventory = NewObject<UTestInventory>(GetTransientPackage());
	Inventory->CreateNewInventory();

	// Test 1: Add Item
	{
		UItemData* NewItem = CreateTestItem(GetTransientPackage(), 5, 10);
		Inventory->AddItemToInventory(NewItem);
		
		TestTrue("Item Added to Slot 0", Inventory->InventoryItems[0] != nullptr);
		if (Inventory->InventoryItems[0])
		{
			TestEqual("Item Amount is 5", Inventory->InventoryItems[0]->GetItemAmount(), 5);
		}
	}
	
	// Test 2: Stack Item
	{
		UItemData* StackItem = CreateTestItem(GetTransientPackage(), 5, 10);
		// Need to ensure ParentItem matches Slot 0
		FItemStruct StackInfo = StackItem->GetItemInfo();
		StackInfo.ParentItem = Inventory->InventoryItems[0]->GetParentItem();
		StackItem->SetInfo(StackInfo);
		
		Inventory->AddItemToInventory(StackItem);
		
		if (Inventory->InventoryItems[0])
		{
			TestEqual("Item Stacked in Slot 0", Inventory->InventoryItems[0]->GetItemAmount(), 10);
		}
	}
	
	// Test 3: Overflow / New Slot
	{
		UItemData* OverflowItem = CreateTestItem(GetTransientPackage(), 5, 10);
		// Set type to match slot 0
		FItemStruct OverInfo = OverflowItem->GetItemInfo();
		OverInfo.ParentItem = Inventory->InventoryItems[0]->GetParentItem();
		OverflowItem->SetInfo(OverInfo);
		
		// Slot 0 is full (10). Should go to Slot 1.
		Inventory->AddItemToInventory(OverflowItem);
		
		TestTrue("Item Overflow to Slot 1", Inventory->InventoryItems[1] != nullptr);
		if (Inventory->InventoryItems[1])
		{
			TestEqual("Slot 1 Amount is 5", Inventory->InventoryItems[1]->GetItemAmount(), 5);
		}
	}
	
	// Test 4: Swap Logic
	{
		// Slot 0: TypeA (10), Slot 1: TypeA (5).
		// Create different TypeB for Slot 2.
		UItemData* ItemB = CreateTestItem(GetTransientPackage(), 1, 1);
		Inventory->AddItemAtIndex(ItemB, 2);
		
		bool bSwap = Inventory->SwapItemSlots(0, 2);
		TestTrue("Swap Successful", bSwap);
		
		UItemData* Slot0 = Inventory->InventoryItems[0];
		UItemData* Slot2 = Inventory->InventoryItems[2];
		
		TestTrue("Slot 0 is TypeB", Slot0 && Slot0->GetParentItem() == ItemB);
		// Check Slot 2 parent item logic if we had references, but checking pointer equality of parent is enough if we kept track.
		// Actually, GetParentItem() returns the DataAsset. in CreateTestItem, ParentItem = Self.
		// So Slot2->ParentItem should act as "TypeA".
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryHelperTest, "DFInventory.Core.Helpers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryHelperTest::RunTest(const FString& Parameters)
{
	UTestInventory* Inventory = NewObject<UTestInventory>(GetTransientPackage());
	Inventory->CreateNewInventory();
	
	// FindEmptySlot
	TestEqual("Empty Slot is 0", Inventory->FindEmptySlot(), 0);
	
	UItemData* Item = CreateTestItem(GetTransientPackage());
	Inventory->AddItemToInventory(Item);
	
	TestEqual("Empty Slot is 1", Inventory->FindEmptySlot(), 1);
	
	// FindItemIndex
	int32 Index = Inventory->FindItemIndex(Inventory->InventoryItems[0]);
	TestEqual("Found Item At 0", Index, 0);
	
	// IsInventoryFull
	Inventory->SetMaxItemSlots(1);
	TestTrue("Inventory Is Full (1/1)", Inventory->IsInventoryFull());
	
	Inventory->SetMaxItemSlots(5);
	TestFalse("Inventory Not Full (1/5)", Inventory->IsInventoryFull());
	
	// CanStackAnyItem
	UItemData* FoundRef = nullptr;
	UItemData* Stackable = CreateTestItem(GetTransientPackage(), 1, 10);
	// Set parent to match Item 0
	FItemStruct Info = Stackable->GetItemInfo();
	Info.ParentItem = Inventory->InventoryItems[0]->GetParentItem(); 
	Stackable->SetInfo(Info);
	
	bool bCanStack = Inventory->FindStackableItem(Stackable, FoundRef);
	TestTrue("Can Stack Any Item", bCanStack);
	TestNotNull("Found Ref Valid", FoundRef);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryRemovalTest, "DFInventory.Core.Removal", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryRemovalTest::RunTest(const FString& Parameters)
{
	UTestInventory* Inventory = NewObject<UTestInventory>(GetTransientPackage());
	Inventory->CreateNewInventory();
	
	UItemData* ItemA = CreateTestItem(GetTransientPackage());
	UItemData* ItemB = CreateTestItem(GetTransientPackage());
	
	Inventory->AddItemAtIndex(ItemA, 0);
	Inventory->AddItemAtIndex(ItemB, 1);
	
	// Remove Item From Inventory (Single)
	Inventory->RemoveItemFromInventory(0);
	TestNull("Slot 0 Removed", Inventory->InventoryItems[0].Get());
	TestNotNull("Slot 1 Remains", Inventory->InventoryItems[1].Get());
	
	// Refill 0
	Inventory->AddItemAtIndex(ItemA, 0);
	
	// Remove Items By Index
	TArray<int32> Indexes = {0, 1};
	Inventory->RemoveItemsByIndex(Indexes);
	TestNull("Slot 0 Removed", Inventory->InventoryItems[0].Get());
	TestNull("Slot 1 Removed", Inventory->InventoryItems[1].Get());
	
	// Refill
	Inventory->AddItemAtIndex(ItemA, 0);
	Inventory->AddItemAtIndex(ItemB, 1);
	
	// Remove Items By Object
	// Note: FindItemIndex searches by pointer equality of the wrapper item in the slot. 
	// The InventoryItems array holds new copies/instances if added via AddItemToInventory? 
	// AddItemAtIndex sets the pointer directly. So ItemA pointer should match.
	TArray<UItemData*> Objs = {Inventory->InventoryItems[0], Inventory->InventoryItems[1]};
	Inventory->RemoveItemsByObject(Objs);
	
	TestNull("Slot 0 Removed (Obj)", Inventory->InventoryItems[0].Get());
	TestNull("Slot 1 Removed (Obj)", Inventory->InventoryItems[1].Get());
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryAdvancedTest, "DFInventory.Core.Advanced", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryAdvancedTest::RunTest(const FString& Parameters)
{
	UTestInventory* Inventory = NewObject<UTestInventory>(GetTransientPackage());
	Inventory->CreateNewInventory();
	
	// AddItemAtIndex
	UItemData* Item = CreateTestItem(GetTransientPackage());
	bool bSuccess = Inventory->AddItemAtIndex(Item, 2); // Explicitly slot 2
	TestTrue("Add At Index 2", bSuccess);
	TestNotNull("Slot 2 Set", Inventory->InventoryItems[2].Get());
	TestNull("Slot 0 Empty", Inventory->InventoryItems[0].Get());
	
	// AddItemAtIndex (Occupied)
	UItemData* Item2 = CreateTestItem(GetTransientPackage());
	bool bFail = Inventory->AddItemAtIndex(Item2, 2); // Should fail/stack? 
	// Logic: If FoundItem, Checks CanItemsStack.
	// Item and Item2 are different "Types" (diff ParentItem structs). So stack should fail.
	TestFalse("Add At Index 2 (Occupied, No Stack)", bFail);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventoryTransferTest, "DFInventory.Core.Transfer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventoryTransferTest::RunTest(const FString& Parameters)
{
	UTestInventory* SourceInv = NewObject<UTestInventory>(GetTransientPackage());
	SourceInv->CreateNewInventory();
	
	UTestInventory* TargetInv = NewObject<UTestInventory>(GetTransientPackage());
	TargetInv->CreateNewInventory();
	TargetInv->SetMaxItemSlots(1); // Restrict to 1 slot to force partial transfer failure on overflow
	
	// Test 1: Full Transfer
	{
		UItemData* Item = CreateTestItem(SourceInv, 10, 10);
		SourceInv->AddItemAtIndex(Item, 0);
		
		TArray<int32> Indexes = {0};
		SourceInv->TransferItemsByIndex(TargetInv, Indexes);
		
		TestNull("Source Empty", SourceInv->InventoryItems[0].Get());
		TestNotNull("Target Filled", TargetInv->InventoryItems[0].Get());
	}
	
	// Reset
	SourceInv->RemoveItemFromInventory(0);
	TargetInv->RemoveItemFromInventory(0);
	
	// Test 2: Partial Transfer
	{
		UItemData* SItem = CreateTestItem(SourceInv, 10, 10);
		SourceInv->AddItemAtIndex(SItem, 0);
		
		UItemData* TItem = CreateTestItem(TargetInv, 5, 10);
		// Match Types
		FItemStruct Info = TItem->GetItemInfo();
		Info.ParentItem = SItem->GetParentItem();
		TItem->SetInfo(Info);
		TargetInv->AddItemAtIndex(TItem, 0);
		
		TArray<int32> Indexes = {0};
		TArray<int32> Failed = SourceInv->TransferItemsByIndex(TargetInv, Indexes);
		
		TestEqual("Failed Count (Partial)", Failed.Num(), 1);
		
		if (SourceInv->InventoryItems[0])
			TestEqual("Source reduced", SourceInv->InventoryItems[0]->GetItemAmount(), 5);
		if (TargetInv->InventoryItems[0])
			TestEqual("Target filled", TargetInv->InventoryItems[0]->GetItemAmount(), 10);
	}
	
	// Test 3: Transfer By Object
	{
		// Cleanup
		SourceInv->RemoveItemFromInventory(0);
		TargetInv->RemoveItemFromInventory(0);
		
		UItemData* Item = CreateTestItem(SourceInv, 5, 10);
		SourceInv->AddItemAtIndex(Item, 0);
		
		TArray<UItemData*> Refs = {Item};
		SourceInv->TransferItemsByObject(TargetInv, Refs);
		
		TestNull("Source Empty (Obj)", SourceInv->InventoryItems[0].Get());
		TestNotNull("Target Filled (Obj)", TargetInv->InventoryItems[0].Get());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventorySplitTest, "DFInventory.Core.Split", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInventorySplitTest::RunTest(const FString& Parameters)
{
	UTestInventory* Inventory = NewObject<UTestInventory>(GetTransientPackage());
	Inventory->CreateNewInventory();
	
	UItemData* Item = CreateTestItem(GetTransientPackage(), 10, 20);
	Inventory->AddItemAtIndex(Item, 0);
	
	bool bSplit = Inventory->SplitStack(0, 1, 5);
	
	TestTrue("Split Success", bSplit);
	if (Inventory->InventoryItems[0]) TestEqual("Source", Inventory->InventoryItems[0]->GetItemAmount(), 5);
	if (Inventory->InventoryItems[1]) TestEqual("Target", Inventory->InventoryItems[1]->GetItemAmount(), 5);
	
	// Test 2: Split Into Existing Stack
	{
		// Slot 0 has 5 remaining from previous test. Slot 1 has 5. Max is 20.
		// Split 5 from Slot 1 back to Slot 0? Or split 3 from Slot 0 to Slot 1.
		
		// Let's split 3 from Slot 0 (Amount 5) to Slot 1 (Amount 5).
		// Expect: Slot 0 = 2, Slot 1 = 8.
		
		bool bSplitMerge = Inventory->SplitStack(0, 1, 3);
		TestTrue("Split Into Existing Success", bSplitMerge);
		
		if (Inventory->InventoryItems[0]) 
			TestEqual("Source Remaining", Inventory->InventoryItems[0]->GetItemAmount(), 2);
			
		if (Inventory->InventoryItems[1])
			TestEqual("Target Merged", Inventory->InventoryItems[1]->GetItemAmount(), 8);
	}
	
	return true;
}
