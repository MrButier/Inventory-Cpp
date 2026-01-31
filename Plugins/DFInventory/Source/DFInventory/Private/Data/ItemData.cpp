#include "Data/ItemData.h"
#include "Net/UnrealNetwork.h"
#include "Settings/DFInventorySettings.h"

UItemData::UItemData()
{
	UDFInventorySettings* Settings = GetMutableDefault<UDFInventorySettings>();
	if (Settings)
	{
		Settings->EnsureExtraInfoValid();
		if (Settings->InstancedExtraInfo.IsValid())
		{ Info.ExtraInfo = Settings->InstancedExtraInfo; }
	}
}

void UItemData::PostLoad()
{
	Super::PostLoad();
	
	UDFInventorySettings* Settings = GetMutableDefault<UDFInventorySettings>();
	if (Settings && Info.ExtraInfo.IsValid())
	{
		Settings->EnsureExtraInfoValid();
		if (Settings->InstancedExtraInfo.IsValid())
		{ Info.ExtraInfo = Settings->InstancedExtraInfo; }
	}
}

void UItemData::SetInfo(FItemStruct NewInfo)
{
	Info = NewInfo;
	OnDataChanged.Broadcast();
}

void UItemData::SetExtraInfo(const FInstancedStruct& NewExtraInfo)
{
	Info.ExtraInfo = NewExtraInfo;
	OnDataChanged.Broadcast();
}

int UItemData::SetItemAmount(int NewValue)
{
	Info.Amount = FMath::Clamp(NewValue, 0, Info.MaxAmount);
	OnDataChanged.Broadcast();
	return Info.Amount;
}

int UItemData::AddItemAmount(int NewValue)
{
	int32 AvailableSpace = Info.MaxAmount - Info.Amount;
	int32 AmountToAdd = FMath::Min(NewValue, AvailableSpace);
	SetItemAmount(Info.Amount + AmountToAdd);
	return NewValue - AmountToAdd;
}

void UItemData::SetIcon(UTexture2D* NewIcon)
{
	if (Info.Icon != NewIcon)
	{
		Info.Icon = NewIcon;
		OnDataChanged.Broadcast();
	}
}

void UItemData::SetItemName(const FString& NewName)
{
	if (!Info.ItemName.Equals(NewName))
	{
		Info.ItemName = NewName;
		OnDataChanged.Broadcast();
	}
}

void UItemData::SetDescription(const FText& NewDescription)
{
	if (!Info.Description.EqualTo(NewDescription))
	{
		Info.Description = NewDescription;
		OnDataChanged.Broadcast();
	}
}

void UItemData::SetMaxAmount(int32 NewMaxAmount)
{
	if (Info.MaxAmount != NewMaxAmount)
	{
		Info.MaxAmount = NewMaxAmount;
		// Re-clamp amount if max is lowered?
		if (Info.Amount > Info.MaxAmount)
		{
			Info.Amount = Info.MaxAmount;
		}
		OnDataChanged.Broadcast();
	}
}

void UItemData::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemData, Info);
}

#if WITH_EDITOR

void UItemData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
    
	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		if (Info.ParentItem != this)
		{ Info.ParentItem = this; }
	}
	
	const UDFInventorySettings* Settings = GetMutableDefault<UDFInventorySettings>();
	if (Settings && Settings->InstancedExtraInfo.IsValid())
	{
		const UScriptStruct* DesiredStruct = Settings->InstancedExtraInfo.GetScriptStruct();
		const UScriptStruct* CurrentStruct = Info.ExtraInfo.GetScriptStruct();
		
		if (!DesiredStruct || CurrentStruct == DesiredStruct) return;
		
		Info.ExtraInfo = Settings->InstancedExtraInfo;
		if (CurrentStruct == nullptr) return;
		
		FMessageLog("Blueprint")
			.Warning(FText::Format(
				NSLOCTEXT("DFInventory", "ExtraInfoLocked", "ExtraInfo can only be changed in project settings and was reset back to '{0}.'"),
				FText::FromName(DesiredStruct->GetFName())));

		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(
				NSLOCTEXT("DFInventory", "ExtraInfoLockedDialog", "ExtraInfo type is locked to Project Settings and has been reset to '{0}'."),
				FText::FromName(DesiredStruct->GetFName())));
	}
}

#endif