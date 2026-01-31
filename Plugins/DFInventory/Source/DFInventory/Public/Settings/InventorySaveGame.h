// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InventorySaveGame.generated.h"

struct FItemStruct;

USTRUCT(BlueprintType)
struct DFINVENTORY_API FItemSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FItemStruct> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxSlots = 0;
};

UCLASS()
class DFINVENTORY_API UInventorySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	
	UPROPERTY(SaveGame)
	FItemSaveData SaveData;
};