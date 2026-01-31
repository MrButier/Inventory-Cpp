#pragma once

#include "CoreMinimal.h"
#include "ItemInterface.generated.h"

class UItemData;

UINTERFACE(Blueprintable)
class DFINVENTORY_API UItemInterface : public UInterface
{
	GENERATED_BODY()
};

class IItemInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item")
	UItemData* GetItem();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Item")
	void SetItem(UItemData* NewItem);
};