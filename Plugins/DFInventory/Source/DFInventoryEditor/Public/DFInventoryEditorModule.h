#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Templates/SharedPointer.h"

struct FAssetData;
struct FPropertyChangedEvent;

class FDFInventoryEditorModule : public IModuleInterface
{
	
public:
	
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	FDelegateHandle AssetAddedHandle;
	FDelegateHandle AssetUpdatedHandle;
	FDelegateHandle AssetRemovedHandle;
	FDelegateHandle OnFilesLoadedHandle;
	FDelegateHandle SettingsChangedHandle;
	
	void RegisterStructChangeWatch();
	void RegisterSettingsChangeWatch();
	void UnregisterStructChangeWatch();
	void UnregisterSettingsChangeWatch();
	
	void HandleAssetChanged(const FAssetData& AssetData);
	void HandleSettingsChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
};
