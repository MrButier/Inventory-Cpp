#include "DFInventoryEditorModule.h"

#include "AccessorGenerator.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Settings/DFInventorySettings.h"

void FDFInventoryEditorModule::StartupModule()
{
	RegisterStructChangeWatch();
	RegisterSettingsChangeWatch();
}

void FDFInventoryEditorModule::ShutdownModule()
{
	UnregisterSettingsChangeWatch();
	UnregisterStructChangeWatch();
}

void FDFInventoryEditorModule::RegisterStructChangeWatch()
{
	FAssetRegistryModule &AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	OnFilesLoadedHandle = AssetRegistryModule.Get().OnFilesLoaded().AddLambda([this, &AssetRegistryModule]
	{
		const UDFInventorySettings* Settings = GetDefault<UDFInventorySettings>();
		if (Settings && Settings->GenerationOptions != EGenerationOptions::Never)
		{
			FAccessorGenerator::Generate();
		}

		auto Callback = [this](const FAssetData &AssetData)
		{ HandleAssetChanged(AssetData); };
		AssetAddedHandle = AssetRegistryModule.Get().OnAssetAdded().AddLambda(Callback);
		AssetUpdatedHandle = AssetRegistryModule.Get().OnAssetUpdated().AddLambda(Callback);
		AssetRemovedHandle = AssetRegistryModule.Get().OnAssetRemoved().AddLambda(Callback);
	});
}

void FDFInventoryEditorModule::RegisterSettingsChangeWatch()
{ SettingsChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddRaw(this, &FDFInventoryEditorModule::HandleSettingsChanged); }

void FDFInventoryEditorModule::UnregisterStructChangeWatch()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		FAssetRegistryModule &AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
		if (AssetAddedHandle.IsValid())
		{ AssetRegistryModule.Get().OnAssetAdded().Remove(AssetAddedHandle); }
		if (AssetUpdatedHandle.IsValid())
		{ AssetRegistryModule.Get().OnAssetUpdated().Remove(AssetUpdatedHandle); }
		if (AssetRemovedHandle.IsValid())
		{ AssetRegistryModule.Get().OnAssetRemoved().Remove(AssetRemovedHandle); }
		if (OnFilesLoadedHandle.IsValid())
		{ AssetRegistryModule.Get().OnFilesLoaded().Remove(OnFilesLoadedHandle); }
	}
	AssetAddedHandle.Reset();
	AssetUpdatedHandle.Reset();
	AssetRemovedHandle.Reset();
	OnFilesLoadedHandle.Reset();
}

void FDFInventoryEditorModule::UnregisterSettingsChangeWatch()
{
	if (SettingsChangedHandle.IsValid())
	{ FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(SettingsChangedHandle); }

	SettingsChangedHandle.Reset();
}

void FDFInventoryEditorModule::HandleAssetChanged(const FAssetData &AssetData)
{
	const UDFInventorySettings *Settings = GetDefault<UDFInventorySettings>();
	if (!Settings || Settings->ExtraInfo.IsNull()) return;
	
	if (Settings->GenerationOptions != EGenerationOptions::Always) return;
	
	const FSoftObjectPath TargetPath = Settings->ExtraInfo.ToSoftObjectPath();
	if (AssetData.GetSoftObjectPath() != TargetPath) return;

	FAccessorGenerator::Generate();
}

void FDFInventoryEditorModule::HandleSettingsChanged(UObject *Object, FPropertyChangedEvent &PropertyChangedEvent)
{
	if (!Object || Object->GetClass() != UDFInventorySettings::StaticClass()) return;

	const FName ChangedName = PropertyChangedEvent.GetPropertyName();
	const bool bIsExtraInfo = ChangedName == GET_MEMBER_NAME_CHECKED(UDFInventorySettings, ExtraInfo);
	const bool bIsInstancedExtraInfo = ChangedName == GET_MEMBER_NAME_CHECKED(UDFInventorySettings, InstancedExtraInfo);

	if (!bIsExtraInfo && !bIsInstancedExtraInfo) return;
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive) return;

	const UDFInventorySettings* Settings = GetDefault<UDFInventorySettings>();
	if (Settings && Settings->GenerationOptions == EGenerationOptions::Always)
	{
		FAccessorGenerator::Generate();
	}
}

IMPLEMENT_MODULE(FDFInventoryEditorModule, DFInventoryEditor)
