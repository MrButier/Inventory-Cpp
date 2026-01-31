#include "Rules/InventorySaveRules.h"
#include "Component/InventoryComponent.h"
#include "Engine/EngineTypes.h"

// =================================================================================================
// BASE RULES (Save ID Logic)
// =================================================================================================

bool UInventorySaveRules::HandleBeginPlay(UInventoryComponent* Inventory)
{
	return false; // Default does nothing
}

void UInventorySaveRules::HandleEndPlay(UInventoryComponent* Inventory, const ELinkTo* EndPlayReason_Obsolete)
{
	// Legacy signature
}

void UInventorySaveRules::HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason)
{
	// Override in child
}

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

FString UInventorySaveRules::GetSaveID_Implementation(UInventoryComponent* Inventory)
{
	// 1. If explicit ID set in Rules, use it
	if (!SaveID.IsEmpty()) return SaveID;

	// 2. Fallbacks
	if (!Inventory) return FString();

	AActor* Owner = Inventory->GetOwner();
	if (!Owner) return FString();

	FString ResolvedID;

	if (APawn* Pawn = Cast<APawn>(Owner))
	{
		// Try finding the controller directly
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			if (PC->IsLocalController())
			{
				ResolvedID = FString::Printf(TEXT("PlayerInv_Local_%d"), UGameplayStatics::GetPlayerControllerID(PC));
			}
		}
		// Fallback: If no controller yet (common in transition), check if we are likely Player 0
		else if (APlayerController* FirstPC = UGameplayStatics::GetPlayerController(Inventory, 0))
		{
			if (FirstPC->GetPawn() == Pawn || FirstPC->GetPawn() == nullptr)
			{
				// Heuristic: If Player 0 has no pawn or is this pawn, assume this is Player 0's inventory
				ResolvedID = FString::Printf(TEXT("PlayerInv_Local_0"));
			}
		}
	}

	// Final Fallback: Actor Name
	if (ResolvedID.IsEmpty())
	{
		ResolvedID = FString::Printf(TEXT("Inv_%s"), *Owner->GetName());
	}
	
	// Cache it? Ideally yes, so it doesn't drift.
	SaveID = ResolvedID;

	return ResolvedID;
}

	return ResolvedID;
}
// =================================================================================================
// HELPERS (Moved from Component)
// =================================================================================================
#include "Settings/InventorySaveGame.h"
#include "Subsystem/DFInventorySubsystem.h"
#include "Settings/DFInventorySettings.h" 

FItemSaveData UInventorySaveRules::CreateSaveData(UInventoryComponent* Inventory)
{
	FItemSaveData Data;
	if (!Inventory) return Data;

	Data.MaxSlots = Inventory->MaxItemSlots;
	Data.Items = Inventory->InventoryItems; // Friend Access
	return Data;
}

void UInventorySaveRules::ApplySaveData(UInventoryComponent* Inventory, const FItemSaveData& Data)
{
	if (!Inventory) return;
	
	Inventory->InventoryItems = Data.Items; // Friend Access
	Inventory->MaxItemSlots = Data.MaxSlots;

	// Ensure size match
	if (Inventory->InventoryItems.Num() != Inventory->MaxItemSlots)
	{
		Inventory->InventoryItems.SetNum(Inventory->MaxItemSlots);
	}
	
	Inventory->OnInventoryRefresh.Broadcast();
}

bool UInventorySaveRules::SaveToDisk(UInventoryComponent* Inventory)
{
	if (!Inventory) return false;
	
	// Global Settings Check
	const UDFInventorySettings* Settings = GetDefault<UDFInventorySettings>();
	if (Settings && !Settings->bEnableAutoSaveOnMapTransition) return false;

	FString ID = GetSaveID(Inventory);
	if (ID.IsEmpty()) return false;

	// Use Configured Class or Default
	TSubclassOf<USaveGame> ClassToUse = SaveGameClass ? SaveGameClass : UInventorySaveGame::StaticClass();
	USaveGame* SaveObj = UGameplayStatics::CreateSaveGameObject(ClassToUse);
	
	if (UInventorySaveGame* TypedSave = Cast<UInventorySaveGame>(SaveObj))
	{
		TypedSave->SaveData = CreateSaveData(Inventory);
		return UGameplayStatics::SaveGameToSlot(TypedSave, ID, 0); // UserIndex 0 for now
	}
	return false;
}

bool UInventorySaveRules::LoadFromDisk(UInventoryComponent* Inventory)
{
	if (!Inventory) return false;

	// Global Settings Check
	const UDFInventorySettings* Settings = GetDefault<UDFInventorySettings>();
	if (Settings && !Settings->bEnableAutoSaveOnMapTransition) return false;

	FString ID = GetSaveID(Inventory);
	
	if (UGameplayStatics::DoesSaveGameExist(ID, 0))
	{
		USaveGame* LoadedObj = UGameplayStatics::LoadGameFromSlot(ID, 0);
		if (UInventorySaveGame* TypedSave = Cast<UInventorySaveGame>(LoadedObj))
		{
			ApplySaveData(Inventory, TypedSave->SaveData);
			return true;
		}
	}
	return false;
}

bool UInventorySaveRules::SaveToMemory(UInventoryComponent* Inventory)
{
	if (!Inventory) return false;

	// Global Settings Check
	const UDFInventorySettings* Settings = GetDefault<UDFInventorySettings>();
	if (Settings && !Settings->bEnableAutoSaveOnMapTransition) return false;

	UWorld* World = Inventory->GetWorld();
	if (!World) return false;
	
	if (UDFInventorySubsystem* Subsystem = World->GetGameInstance()->GetSubsystem<UDFInventorySubsystem>())
	{
		Subsystem->StoreInventoryData(FName(*GetSaveID(Inventory)), CreateSaveData(Inventory));
		return true;
	}
	return false;
}

bool UInventorySaveRules::LoadFromMemory(UInventoryComponent* Inventory)
{
	if (!Inventory) return false;

	// Global Settings Check
	const UDFInventorySettings* Settings = GetDefault<UDFInventorySettings>();
	if (Settings && !Settings->bEnableAutoSaveOnMapTransition) return false;

	UWorld* World = Inventory->GetWorld();
	if (!World) return false;

	if (UDFInventorySubsystem* Subsystem = World->GetGameInstance()->GetSubsystem<UDFInventorySubsystem>())
	{
		FItemSaveData Data;
		if (Subsystem->RetrieveInventoryData(FName(*GetSaveID(Inventory)), Data))
		{
			ApplySaveData(Inventory, Data);
			return true;
		}
	}
	return false;
}

// =================================================================================================
// HYBRID
// =================================================================================================

bool UInventorySaveRules_Hybrid::HandleBeginPlay(UInventoryComponent* Inventory)
{
	if (!Inventory) return false;

	// Priority 1: Memory (Map Transition)
	if (LoadFromMemory(Inventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rules] Hybrid: Loaded from MEMORY."));
		return true;
	}
	// Priority 2: Disk (New Session)
	if (LoadFromDisk(Inventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rules] Hybrid: Loaded from DISK."));
		return true;
	}
	return false;
}

void UInventorySaveRules_Hybrid::HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason)
{
	if (!Inventory) return;

	if (Reason == EEndPlayReason::LevelTransition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rules] Hybrid: Map Change -> Save to MEMORY."));
		SaveToMemory(Inventory);
	}
	else if (Reason == EEndPlayReason::Quit || Reason == EEndPlayReason::EndPlayInEditor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rules] Hybrid: Quit/Stop -> Save to DISK."));
		SaveToDisk(Inventory);
	}
	else if (Reason == EEndPlayReason::Destroyed || Reason == EEndPlayReason::RemovedFromWorld)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rules] Hybrid: Destroyed -> Save to MEMORY (Transition Backup)."));
		SaveToMemory(Inventory);
	}
}

// =================================================================================================
// DISK ONLY
// =================================================================================================

bool UInventorySaveRules_Disk::HandleBeginPlay(UInventoryComponent* Inventory)
{
	if (LoadFromDisk(Inventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rules] DiskOnly: Loaded."));
		return true;
	}
	return false;
}

void UInventorySaveRules_Disk::HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("[Rules] DiskOnly: Saving..."));
	SaveToDisk(Inventory);
}

// =================================================================================================
// MEMORY ONLY
// =================================================================================================

bool UInventorySaveRules_Memory::HandleBeginPlay(UInventoryComponent* Inventory)
{
	if (LoadFromMemory(Inventory))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Rules] MemoryOnly: Loaded."));
		return true;
	}
	return false;
}

void UInventorySaveRules_Memory::HandleEndPlay_Explicit(UInventoryComponent* Inventory, EEndPlayReason::Type Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("[Rules] MemoryOnly: Saving to Subsystem..."));
	SaveToMemory(Inventory);
}
