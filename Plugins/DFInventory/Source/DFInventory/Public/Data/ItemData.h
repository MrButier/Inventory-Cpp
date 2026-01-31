// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

// BEGIN INCLUDES
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Struct/ItemInfo.h"
// END INCLUDES

#include "ItemData.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemDataChanged);

UCLASS(BlueprintType)
class DFINVENTORY_API UItemData : public UDataAsset
{
	GENERATED_BODY()

protected:
	
	UItemData();
	virtual void PostLoad() override;
	
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Info, BlueprintSetter=SetInfo, Category="Info", meta=(ShowOnlyInnerProperties))
	FItemStruct Info {};
	
	UFUNCTION()
	void OnRep_Info()
	{ OnDataChanged.Broadcast(); }
	
public:

	#if WITH_EDITOR
		virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	UPROPERTY(BlueprintAssignable, Category = "Item Data")
	FOnItemDataChanged OnDataChanged;
	
	UFUNCTION(BlueprintCallable)
	int AddItemAmount(int NewValue);
	
	UFUNCTION(BlueprintCallable)
	virtual FItemStruct GetItemInfo() { return Info; }
	
	UFUNCTION(BlueprintCallable)
	FInstancedStruct GetExtraInfo() const { return Info.ExtraInfo; }
	
	UFUNCTION(BlueprintSetter)
	void SetInfo(FItemStruct NewInfo);
	
	UFUNCTION(BlueprintCallable)
	void SetExtraInfo(const FInstancedStruct& NewExtraInfo);
	
	UFUNCTION(BlueprintCallable)
	int SetItemAmount(int NewValue);
	
	UFUNCTION(BlueprintCallable)
	bool ItemSlotsAvailable() { return Info.Amount != Info.MaxAmount; }
	
	UFUNCTION(BlueprintCallable)
	UItemData* GetParentItem() const { return Info.ParentItem; }

	UFUNCTION(BlueprintCallable)
	UTexture2D* GetIcon() const { return Info.Icon; }

	UFUNCTION(BlueprintCallable)
	FString GetItemName() const { return Info.ItemName; }

	UFUNCTION(BlueprintCallable)
	FText GetDescription() const { return Info.Description; }

	UFUNCTION(BlueprintCallable)
	int32 GetItemAmount() { return Info.Amount; }

	UFUNCTION(BlueprintCallable)
	int32 GetItemMaxAmount() { return Info.MaxAmount; }

	UFUNCTION(BlueprintCallable)
	void SetIcon(UTexture2D* NewIcon);

	UFUNCTION(BlueprintCallable)
	void SetItemName(const FString& NewName);

	UFUNCTION(BlueprintCallable)
	void SetDescription(const FText& NewDescription);

	UFUNCTION(BlueprintCallable)
	void SetMaxAmount(int32 NewMaxAmount);
};
