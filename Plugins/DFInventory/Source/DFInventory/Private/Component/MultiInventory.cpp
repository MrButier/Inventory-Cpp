#include "Component/MultiInventory.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Data/ItemData.h"

UMPInventoryComponent::UMPInventoryComponent()
{ SetIsReplicatedByDefault(true); }

bool UMPInventoryComponent::ShouldAutoSave() const
{ return GetOwner() && GetOwner()->HasAuthority(); }

FString UMPInventoryComponent::GetSaveID() const
{
	if (!SaveID.IsEmpty()) return SaveID;
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		APlayerState* PS = OwnerPawn->GetPlayerState();
		if (PS && PS->GetUniqueId().IsValid())
		{
			return "PlayerInv_" + PS->GetUniqueId().ToString();
		}
	}
	return TEXT("");
}

void UMPInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, InventoryItems);
}

bool UMPInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (TObjectPtr<UItemData> Item : InventoryItems)
	{
		if (!Item) continue;
		bWroteSomething |= Channel->ReplicateSubobject(Item, *Bunch, *RepFlags);
	}
	return bWroteSomething;
}
