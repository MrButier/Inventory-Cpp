#pragma once

#include "CoreMinimal.h"
#include "Settings/InventorySaveGame.h"
#include "InventoryComponent.generated.h"

class UItemData;
struct FItemStruct;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUpdated, int32, Index, UItemData*, Item);

UENUM(BlueprintType)
enum class ESaveType : uint8
{
	Disk	UMETA(DisplayName = "Disk (SaveGame)"),
	Memory	UMETA(DisplayName = "Memory (Map Transfer)")
};

UCLASS(Blueprintable, ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class DFINVENTORY_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UInventorySaveRules;

protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin=1))
	int32 MaxItemSlots = 5;

	// defines how this inventory handles saving/loading (Disk, Memory, etc).
	// If Null, no auto-saving will occur.
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Save|Persistence")
	TObjectPtr<class UInventorySaveRules> SaveRules;
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_InventoryItems)
	TArray<TObjectPtr<UItemData>> InventoryItems;
	
	UPROPERTY(BlueprintAssignable)
	FOnItemUpdated OnItemUpdated;

	// Fired when the inventory is full.
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEvent OnInventoryFull;
	
	// Fired when the inventory is fully reloaded (e.g. Replication or Load).
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryEvent OnInventoryRefresh;
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryEvent OnInventoryLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnInventoryEvent OnInventorySaved;

public:
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (ReturnDisplayName = "Inventory Items"))
	virtual TArray<UItemData*> GetInventoryItems()
	{ return InventoryItems;}

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetMaxItemSlots() const { return MaxItemSlots; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetMaxItemSlots(int32 NewMaxSlots);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SplitStack(int32 SourceIndex, int32 TargetIndex, int32 Amount);
	
	UFUNCTION(BlueprintPure, Category = "Inventory")
	virtual int InventoryLastIndex() { return InventoryItems.Num() - 1; }
	
	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (ReturnDisplayName = "Can Stack?"))
	virtual bool FindStackableItem(UItemData* NewItem, UItemData*& ItemRef);

	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (ReturnDisplayName = "Can Items Stack"))
	virtual bool CanItemsStack(UItemData* NewItem, UItemData* ExistingItem);

	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (ReturnDisplayName = "Empty Slot Index"))
	virtual int32 FindEmptySlot();

	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (ReturnDisplayName = "Item Index"))
	virtual int32 FindItemIndex(UItemData* Item);
	
	UFUNCTION(BlueprintPure, Category = "Inventory", meta = (ReturnDisplayName = "Is Full?"))
	virtual bool IsInventoryFull();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (DisplayName = "Create New Bag"))
	virtual void CreateNewInventory();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (ReturnDisplayName = "Drop"))
	virtual bool AddItemToInventory(UItemData* NewItem);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	virtual bool TransferItemToSlot(UInventoryComponent* TargetComponent, int32 SourceIndex, int32 TargetIndex)
	{ return GenericTransferItem(TargetComponent, SourceIndex, TargetIndex); }
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta=(DisplayName="Transfer Items (by Index)"))
	TArray<int32> TransferItemsByIndex(UInventoryComponent* TargetComponent, const TArray<int32>& ItemIndexes);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta=(DisplayName="Transfer Items (by Data Asset)"))
	TArray<UItemData*> TransferItemsByObject(UInventoryComponent* TargetComponent, const TArray<UItemData*>& ItemObjects);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (DisplayName = "Drop Item"))
	virtual void RemoveItemFromInventory(int32 ItemIndex);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory (by Index)")
	void RemoveItemsByIndex(const TArray<int32>& ItemIndexes);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta=(DisplayName="Remove Items (by Data Asset)"))
	void RemoveItemsByObject(const TArray<UItemData*>& ItemObjects);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (DisplayName = "Swap Item"))
	virtual bool SwapItemSlots(int32 SourceIndex, int32 TargetIndex);

// Blueprint Implementable events
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory")
	bool ItemStackCondition(UItemData* NewItem, UItemData*& FoundItem);
	bool ItemStackCondition_Implementation(UItemData* NewItem, UItemData*& FoundItem)
	{ return true; }
	
	UFUNCTION(BlueprintCallable, BlueprintCallable, Category="Inventory")
	virtual bool StackItem(UItemData* FoundItem, UItemData* NewItem, int32& ItemIndex);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory")
	bool HandleInventoryFull(UItemData* NewItem);
	virtual bool HandleInventoryFull_Implementation(UItemData* NewItem)
	{ return true; }
	
	void AddItem(UItemData* NewItem, int& ItemIndex);

// Multiplayer
	UFUNCTION()
	virtual void OnRep_InventoryItems()
	{ OnInventoryRefresh.Broadcast(); }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (DisplayName = "Add Item At Index"))
	virtual bool AddItemAtIndex(UItemData* NewItem, int32 Index);

protected:
	
	virtual bool GenericTransferItem(UInventoryComponent* TargetComponent, int32 SourceIndex, int32 TargetIndex = -1);
};
