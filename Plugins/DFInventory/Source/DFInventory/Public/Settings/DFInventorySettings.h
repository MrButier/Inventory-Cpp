// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/SoftObjectPtr.h"
#include "DFInventorySettings.generated.h"

UENUM(BlueprintType)
enum class EGenerationOptions : uint8
{
	/** Always regenerate accessors when settings change or assets are loaded. */
	Always,
	/** Only regenerate accessors when the editor restarts (on module startup). */
	WhenRestarting,
	/** Never automatically regenerate accessors. */
	Never
};

UCLASS(config=Plugins, defaultconfig, meta=(DisplayName="Demon Forge"))
class DFINVENTORY_API UDFInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
#if WITH_EDITOR
	virtual FText GetSectionText() const override { return NSLOCTEXT("DemonForge", "DFInventorySettingsSection", "Demon Forge"); }
#endif

	/** Soft reference to the ExtraInfo struct type to use for all items. */
	UPROPERTY(EditAnywhere, Config, Category="General", meta=(AllowedTypes="ScriptStruct"))
	TSoftObjectPtr<UScriptStruct> ExtraInfo;

	/** Instanced struct holding the configured ExtraInfo type. */
	UPROPERTY(VisibleAnywhere, Config, Category="General", meta=(EditCondition=false, EditConditionHides))
	FInstancedStruct InstancedExtraInfo;

	/** Controls when the Blueprint accessors are regenerated. */
	UPROPERTY(EditAnywhere, Config, Category="Generation")
	EGenerationOptions GenerationOptions = EGenerationOptions::Always;

	/** If true, the generator will log detailed information to the output log. */
	UPROPERTY(EditAnywhere, Config, Category="Generation")
	bool bVerboseLogging = false;

	/** If true, items can be stacked. If false, MaxStackSize is forced to 1. */
	UPROPERTY(EditAnywhere, Config, Category="General")
	bool bCanItemsStack = true;

	/** If true, the inventory will automatically try to save/load from the DFInventorySubsystem during map transitions. */
	UPROPERTY(EditAnywhere, Config, Category="Persistence")
	bool bEnableAutoSaveOnMapTransition = false;

	/** Ensures DefaultExtraInfo is initialized to the struct pointed to by ExtraInfoStruct. */
	void EnsureExtraInfoValid();
};

