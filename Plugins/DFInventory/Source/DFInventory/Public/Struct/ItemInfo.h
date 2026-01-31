#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "ItemInfo.generated.h"

class UItemData;

USTRUCT(Blueprintable)
struct DFINVENTORY_API FItemStruct {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Info")
  UItemData *ParentItem = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, category = "Info")
  UTexture2D *Icon = nullptr;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, category = "Info")
  FString ItemName;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, category = "Info")
  FText Description;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Info")
  int32 Amount = 1;

  UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Info", meta = (ClampMin = 1))
  int32 MaxAmount = 1;

  // Pluggable per-item payload that can be set in editor via project settings.
  UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Info")
  FInstancedStruct ExtraInfo;

  template <typename TExtra> TExtra *GetExtraInfoMutable() {
    return ExtraInfo.GetMutablePtr<TExtra>();
  }

  template <typename TExtra> const TExtra *GetExtraInfo() const {
    return ExtraInfo.GetPtr<TExtra>();
  }

  UItemData *GetParentItem() { return ParentItem; }

  int32 GetItemAmount() { return Amount; }

  int32 GetItemMaxAmount() { return MaxAmount; }

  bool ItemSlotsAvailable() { return Amount != MaxAmount; }

  int32 SetItemAmount(int32 NewAmount) {
    return Amount = FMath::Clamp(NewAmount, 0, MaxAmount);
  }

  int32 AddItemAmount(int32 NewAmount) {
    int32 AvailableSpace = MaxAmount - Amount;
    int32 AmountToAdd = FMath::Min(NewAmount, AvailableSpace);
    SetItemAmount(Amount + AmountToAdd);
    return NewAmount - AmountToAdd;
  }
};

// Base payload for ExtraInfo. Hidden from the picker but not abstract so users
// can create derived structs.
USTRUCT(BlueprintType, meta = (Hidden, BlueprintInternalUseOnly = "true"))
struct DFINVENTORY_API FItemExtraInfo {
  GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FEquipmentStruct : public FItemExtraInfo {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Equipment")
  FGameplayTag ItemType;
};
