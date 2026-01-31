
#if WITH_EDITOR

#include "AccessorGenerator.h"
#include "PinHelper.h"

#include "Data/ItemData.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"

#include "Settings/DFInventorySettings.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/BlueprintInstancedStructLibrary.h"

namespace
{
	const FName GeneratedMetaKey(TEXT("DFInventoryGenerated"));
}

uint32 ComputeStructHash(const UScriptStruct *Struct)
{
	if (!Struct) return 0;
	
	FString HashString = Struct->GetName();
	HashString += TEXT("_v6");
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			HashString += It->GetName();
			HashString += It->GetCPPType();
			HashString += FString::FromInt(It->ArrayDim);
		}
	return GetTypeHash(HashString);
}

FString FAccessorGenerator::SanitizeIdentifier(FString Input)
{
	FString Out;
	Out.Reserve(Input.Len());

	for (TCHAR Char : Input)
	{
		if (FChar::IsAlnum(Char) || Char == TEXT('_'))
		{ Out.AppendChar(Char); }
	}

	if (Out.IsEmpty() || FChar::IsDigit(Out[0]))
	{ Out.InsertAt(0, TEXT('_')); }

	return Out;
}

FString FAccessorGenerator::MakeFunctionName(const FString &Prefix, const FString &PropertyName)
{ return Prefix + SanitizeIdentifier(PropertyName); }

FString FAccessorGenerator::GetLibraryPackagePath()
{ return TEXT("/DFInventory/ExtraInfoLibrary"); }

UBlueprint *FAccessorGenerator::LoadOrCreateLibrary()
{
	const FString PackagePath = GetLibraryPackagePath();
	if (PackagePath.IsEmpty()) return nullptr;

	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	const FString ObjectPath =
	FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);

	if (UBlueprint *Existing = LoadObject<UBlueprint>(nullptr, *ObjectPath))
	{ return Existing; }

	UPackage *Package = CreatePackage(*PackagePath);
	if (!Package) return nullptr;

	Package->FullyLoad();

	if (UObject *ExistingObject = FindObject<UObject>(Package, *AssetName))
	{
		if (UBlueprint *ExistingBP = Cast<UBlueprint>(ExistingObject))
		{ return ExistingBP; }
		return nullptr;
	}

	UBlueprint *NewBP = FKismetEditorUtilities::CreateBlueprint(
		UBlueprintFunctionLibrary::StaticClass(),
		Package,
		*AssetName,
		BPTYPE_FunctionLibrary,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass()
	);

	if (NewBP) FAssetRegistryModule::AssetCreated(NewBP);
	return NewBP;
}

bool FAccessorGenerator::ConvertPropertyToPinType(const FProperty *Property, FEdGraphPinType &OutPinType)
{
	OutPinType = FEdGraphPinType{};
	const UEdGraphSchema_K2 *Schema = GetDefault<UEdGraphSchema_K2>();
	return Property && Schema && Schema->ConvertPropertyToPinType(Property, OutPinType);
}

void FAccessorGenerator::ResetFunctionGraph(UEdGraph *Graph, UK2Node_FunctionEntry *&OutEntry, UK2Node_FunctionResult *&OutResult)
{
	OutEntry = nullptr;
	OutResult = nullptr;
	if (!Graph) return;

	Graph->Modify();
	for (UEdGraphNode *Node : TArray<UEdGraphNode *>(Graph->Nodes))
	{ Graph->RemoveNode(Node); }

	const UEdGraphSchema_K2 *Schema = GetDefault<UEdGraphSchema_K2>();
	Schema->CreateDefaultNodesForGraph(*Graph);

	for (UEdGraphNode *Node : Graph->Nodes)
	{
		if (UK2Node_FunctionEntry *Entry = Cast<UK2Node_FunctionEntry>(Node))
		{ OutEntry = Entry; }
		else if (UK2Node_FunctionResult *Result = Cast<UK2Node_FunctionResult>(Node))
		{ OutResult = Result; }
	}

	FPinHelper Helper(Graph);

	if (!OutEntry)
	{
		OutEntry = Helper.SpawnNode<UK2Node_FunctionEntry>(-200.0f, 0.0f);
		OutEntry->AllocateDefaultPins();
	}

	if (!OutResult)
	{
		OutResult = Helper.SpawnNode<UK2Node_FunctionResult>(200.0f, 0.0f);
		OutResult->AllocateDefaultPins();
	}
}

bool FAccessorGenerator::BuildGetterGraph(UBlueprint *Library, UEdGraph *Graph, const UScriptStruct *ExtraStruct, const FProperty *Property, const FString &FunctionName)
{
	if (!Library || !Graph || !ExtraStruct || !Property) return false;

	UK2Node_FunctionEntry *Entry = nullptr;
	UK2Node_FunctionResult *Result = nullptr;
	ResetFunctionGraph(Graph, Entry, Result);
	if (!Entry || !Result) return false;

	FPinHelper Helper(Graph);
	const UEdGraphSchema_K2 *Schema = GetDefault<UEdGraphSchema_K2>();
	
	Entry->CustomGeneratedFunctionName = *FunctionName;
	Entry->AddExtraFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure | FUNC_Static | FUNC_Public);
	Entry->NodePosX = 0;
	Entry->NodePosY = 0;

	while (Entry->UserDefinedPins.Num() > 0)
	{ Entry->RemoveUserDefinedPin(Entry->UserDefinedPins[0]); }
	while (Result->UserDefinedPins.Num() > 0)
	{ Result->RemoveUserDefinedPin(Result->UserDefinedPins[0]); }

	FEdGraphPinType ItemPinType;
	ItemPinType.PinCategory = Schema->PC_Object;
	ItemPinType.PinSubCategoryObject = UItemData::StaticClass();
	UEdGraphPin *EntryItemPin = Entry->CreateUserDefinedPin(TEXT("Item"), ItemPinType, EGPD_Output);
	
	FEdGraphPinType ValuePinType;
	if (!ConvertPropertyToPinType(Property, ValuePinType)) return false;

	UEdGraphPin *ResultValuePin = Result->CreateUserDefinedPin(TEXT("Value"), ValuePinType, EGPD_Input);
	Result->NodePosX = 900; // V6
	Result->NodePosY = 0;

	UK2Node_CallFunction *CallGetInfo = Helper.SpawnNode<UK2Node_CallFunction>(200.0f, 150.0f);
	CallGetInfo->SetFromFunction(UItemData::StaticClass()->FindFunctionByName(TEXT("GetExtraInfo")));
	CallGetInfo->AllocateDefaultPins();

	if (EntryItemPin)
	{
		UEdGraphPin *Target = Helper.FindPin(CallGetInfo, UEdGraphSchema_K2::PN_Self);
		if (!Target) Target = Helper.FindPin(CallGetInfo, FName(TEXT("Target")));
		if (Target) Helper.Connect(EntryItemPin, Target);
	}

	UK2Node_CallFunction *CallGetValue = Helper.SpawnNode<UK2Node_CallFunction>(350.0f, 0.0f);
	CallGetValue->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(TEXT("GetInstancedStructValue")));
	CallGetValue->AllocateDefaultPins();
	
	Helper.Connect(CallGetInfo->GetReturnValuePin(), Helper.FindPin(CallGetValue, FName(TEXT("InstancedStruct"))));
	Helper.ConnectExec(Entry, CallGetValue);
	
	UK2Node_BreakStruct *BreakNode = Helper.SpawnNode<UK2Node_BreakStruct>(350.0f, 250.0f);
	BreakNode->StructType = const_cast<UScriptStruct *>(ExtraStruct);
	BreakNode->AllocateDefaultPins();

	if (UEdGraphPin *ValOut = Helper.FindPin(CallGetValue, FName(TEXT("Value"))))
	{
		ValOut->PinType.PinCategory = Schema->PC_Struct;
		ValOut->PinType.PinSubCategoryObject = const_cast<UScriptStruct *>(ExtraStruct);
		
		Helper.Connect(ValOut, Helper.FindPinByCategory(BreakNode, Schema->PC_Struct, EGPD_Input));
	}

	UEdGraphPin *PropPin = Helper.FindPin(BreakNode, Property->GetFName());
	if (!PropPin) PropPin = Helper.FindPin(BreakNode, FName(*Property->GetAuthoredName()));

	Helper.Connect(PropPin, ResultValuePin);
	Helper.Connect(Helper.FindPin(CallGetValue, FName(TEXT("Valid"))), Helper.GetToExecPin(Result));

	return true;
}

bool FAccessorGenerator::BuildSetterGraph(UBlueprint *Library, UEdGraph *Graph, const UScriptStruct *ExtraStruct, const FProperty *Property, const FString &FunctionName)
{
	if (!Library || !Graph || !ExtraStruct || !Property) return false;

	UK2Node_FunctionEntry *Entry = nullptr;
	UK2Node_FunctionResult *Result = nullptr;
	ResetFunctionGraph(Graph, Entry, Result);
	if (!Entry || !Result) return false;

	FPinHelper Helper(Graph);
	const UEdGraphSchema_K2 *Schema = GetDefault<UEdGraphSchema_K2>();
	
	Entry->CustomGeneratedFunctionName = *FunctionName;
	Entry->AddExtraFlags(FUNC_BlueprintCallable | FUNC_Static | FUNC_Public);
	Entry->NodePosX = 0;
	Entry->NodePosY = 0;

	while (Entry->UserDefinedPins.Num() > 0)
	{ Entry->RemoveUserDefinedPin(Entry->UserDefinedPins[0]); }
	while (Result->UserDefinedPins.Num() > 0)
	{ Result->RemoveUserDefinedPin(Result->UserDefinedPins[0]); }

	FEdGraphPinType ItemPinType;
	ItemPinType.PinCategory = Schema->PC_Object;
	ItemPinType.PinSubCategoryObject = UItemData::StaticClass();
	UEdGraphPin *EntryItemPin = Entry->CreateUserDefinedPin(TEXT("Item"), ItemPinType, EGPD_Output);

	FEdGraphPinType ValuePinType;
	if (!ConvertPropertyToPinType(Property, ValuePinType)) return false;
	UEdGraphPin *EntryNewValuePin = Entry->CreateUserDefinedPin(TEXT("NewValue"), ValuePinType, EGPD_Output);

	Result->NodePosX = 1450; // V6
	Result->NodePosY = 0;

	UK2Node_CallFunction *CallGetInfo = Helper.SpawnNode<UK2Node_CallFunction>(200.0f, 150.0f);
	CallGetInfo->SetFromFunction(UItemData::StaticClass()->FindFunctionByName(TEXT("GetExtraInfo")));
	CallGetInfo->AllocateDefaultPins();

	if (EntryItemPin)
	{
		UEdGraphPin *Target = Helper.FindPin(CallGetInfo, UEdGraphSchema_K2::PN_Self);
		if (!Target) Target = Helper.FindPin(CallGetInfo, FName(TEXT("Target")));
		if (Target) Helper.Connect(EntryItemPin, Target);
	}

	UK2Node_CallFunction *CallGetValue = Helper.SpawnNode<UK2Node_CallFunction>(350.0f, 0.0f);
	CallGetValue->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(TEXT("GetInstancedStructValue")));
	CallGetValue->AllocateDefaultPins();

	Helper.Connect(CallGetInfo->GetReturnValuePin(), Helper.FindPin(CallGetValue, FName(TEXT("InstancedStruct"))));
	Helper.ConnectExec(Entry, CallGetValue);

	UK2Node_BreakStruct *BreakNode = Helper.SpawnNode<UK2Node_BreakStruct>(350.0f, 250.0f);
	BreakNode->StructType = const_cast<UScriptStruct *>(ExtraStruct);
	BreakNode->AllocateDefaultPins();

	UK2Node_MakeStruct *MakeNode = Helper.SpawnNode<UK2Node_MakeStruct>(700.0f, 250.0f); 
	MakeNode->StructType = const_cast<UScriptStruct *>(ExtraStruct);
	MakeNode->AllocateDefaultPins();

	if (UEdGraphPin *ValOut = Helper.FindPin(CallGetValue, FName(TEXT("Value"))))
	{
		ValOut->PinType.PinCategory = Schema->PC_Struct;
		ValOut->PinType.PinSubCategoryObject = const_cast<UScriptStruct *>(ExtraStruct);

		UEdGraphPin *StructIn = Helper.FindPin(BreakNode, ExtraStruct->GetFName());
		if (!StructIn) StructIn = Helper.FindPin(BreakNode, FName(TEXT("Struct")));
		if (!StructIn) StructIn = Helper.FindPinByCategory(BreakNode, Schema->PC_Struct, EGPD_Input);
		
		Helper.Connect(ValOut, StructIn);
	}

	for (UEdGraphPin *MakePin : MakeNode->Pins)
	{
		if (MakePin->Direction != EGPD_Input) continue;

		bool bIsTarget = MakePin->PinName == Property->GetFName() || MakePin->PinName == *Property->GetAuthoredName();
		if (bIsTarget && EntryNewValuePin)
		{ Helper.Connect(EntryNewValuePin, MakePin); }
		else
		{ Helper.Connect(Helper.FindPin(BreakNode, MakePin->PinName), MakePin); }
	}

	UK2Node_CallFunction *CallMakeInstanced = Helper.SpawnNode<UK2Node_CallFunction>(850.0f, 0.0f);
	CallMakeInstanced->SetFromFunction(UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(TEXT("MakeInstancedStruct")));
	CallMakeInstanced->AllocateDefaultPins();
	
	UEdGraphPin *MakeOut = nullptr;
	for (UEdGraphPin *Pin : MakeNode->Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{ MakeOut = Pin; break; }
	}

	if (MakeOut)
	{
		UEdGraphPin *ValueIn = Helper.FindPin(CallMakeInstanced, FName(TEXT("Value")));
		if (!ValueIn) ValueIn = Helper.FindPinByCategory(CallMakeInstanced, Schema->PC_Wildcard, EGPD_Input);
		
		if (ValueIn)
		{
			ValueIn->PinType.PinCategory = Schema->PC_Struct;
			ValueIn->PinType.PinSubCategoryObject = const_cast<UScriptStruct *>(ExtraStruct);
			Helper.Connect(MakeOut, ValueIn);
		}
	}

	UK2Node_CallFunction *CallSetInfo = Helper.SpawnNode<UK2Node_CallFunction>(1100.0f, 0.0f);
	CallSetInfo->SetFromFunction(UItemData::StaticClass()->FindFunctionByName(TEXT("SetExtraInfo")));
	CallSetInfo->AllocateDefaultPins();
	
	if (EntryItemPin)
	{
		UEdGraphPin *Target = Helper.FindPin(CallSetInfo, UEdGraphSchema_K2::PN_Self);
		if (!Target) Target = Helper.FindPin(CallSetInfo, FName(TEXT("Target")));
		if (Target) Helper.Connect(EntryItemPin, Target);
	}

	Helper.Connect(CallMakeInstanced->GetReturnValuePin(), Helper.FindPin(CallSetInfo, FName(TEXT("NewExtraInfo"))));

	if (UEdGraphPin *ValidPin = Helper.FindPin(CallGetValue, FName(TEXT("Valid"))))
	{
		if (UEdGraphPin *MakeExec = CallMakeInstanced->GetExecPin())
		{ Helper.Connect(ValidPin, MakeExec); }
		else
		{ Helper.Connect(ValidPin, CallSetInfo->GetExecPin()); }
	}

	Helper.Connect(CallMakeInstanced->GetThenPin(), CallSetInfo->GetExecPin());
	Helper.Connect(CallSetInfo->GetThenPin(), Helper.GetToExecPin(Result));

	return true;
}

bool FAccessorGenerator::BuildStructGetterGraph(UBlueprint* Library, UEdGraph* Graph, const UScriptStruct* ExtraStruct, const FString& FunctionName)
{
	if (!Library || !Graph || !ExtraStruct) return false;

	UK2Node_FunctionEntry* Entry = nullptr;
	UK2Node_FunctionResult* Result = nullptr;
	ResetFunctionGraph(Graph, Entry, Result);
	if (!Entry || !Result) return false;

	FPinHelper Helper(Graph);
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	
	Entry->CustomGeneratedFunctionName = *FunctionName;
	Entry->AddExtraFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure | FUNC_Static | FUNC_Public);
	Entry->NodePosX = 0;
	Entry->NodePosY = 0;

	while (Entry->UserDefinedPins.Num() > 0)
	{ Entry->RemoveUserDefinedPin(Entry->UserDefinedPins[0]); }
	while (Result->UserDefinedPins.Num() > 0)
	{ Result->RemoveUserDefinedPin(Result->UserDefinedPins[0]); }

	FEdGraphPinType ItemPinType;
	ItemPinType.PinCategory = Schema->PC_Object;
	ItemPinType.PinSubCategoryObject = UItemData::StaticClass();
	UEdGraphPin* EntryItemPin = Entry->CreateUserDefinedPin(TEXT("Item"), ItemPinType, EGPD_Output);
	
	FEdGraphPinType ValuePinType;
	ValuePinType.PinCategory = Schema->PC_Struct;
	ValuePinType.PinSubCategoryObject = const_cast<UScriptStruct*>(ExtraStruct);
	UEdGraphPin* ResultValuePin = Result->CreateUserDefinedPin(TEXT("Value"), ValuePinType, EGPD_Input);
	
	Result->NodePosX = 900; // V6
	Result->NodePosY = 0;

	UFunction* GetExtraInfoFunc = UItemData::StaticClass()->FindFunctionByName(TEXT("GetExtraInfo"));
	if (!GetExtraInfoFunc) return false;

	UK2Node_CallFunction* CallGetInfo = Helper.SpawnNode<UK2Node_CallFunction>(200.0f, 150.0f);
	CallGetInfo->SetFromFunction(GetExtraInfoFunc);
	CallGetInfo->AllocateDefaultPins();

	if (EntryItemPin)
	{
		UEdGraphPin* Target = Helper.FindPin(CallGetInfo, UEdGraphSchema_K2::PN_Self);
		if (!Target) Target = Helper.FindPin(CallGetInfo, FName(TEXT("Target")));
		if (Target) Helper.Connect(EntryItemPin, Target);
	}

	UFunction* GetInstancedValueFunc = UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(TEXT("GetInstancedStructValue"));
	if (!GetInstancedValueFunc) return false;

	UK2Node_CallFunction* CallGetValue = Helper.SpawnNode<UK2Node_CallFunction>(350.0f, 0.0f);
	CallGetValue->SetFromFunction(GetInstancedValueFunc);
	CallGetValue->AllocateDefaultPins();
	
	Helper.Connect(CallGetInfo->GetReturnValuePin(), Helper.FindPin(CallGetValue, FName(TEXT("InstancedStruct"))));
	Helper.ConnectExec(Entry, CallGetValue);

	if (UEdGraphPin* ValOut = Helper.FindPin(CallGetValue, FName(TEXT("Value"))))
	{
		ValOut->PinType = ValuePinType;
		Helper.Connect(ValOut, ResultValuePin);
	}
	
	Helper.Connect(Helper.FindPin(CallGetValue, FName(TEXT("Valid"))), Helper.GetToExecPin(Result));

	return true;
}

bool FAccessorGenerator::BuildStructSetterGraph(UBlueprint* Library, UEdGraph* Graph, const UScriptStruct* ExtraStruct, const FString& FunctionName)
{
	if (!Library || !Graph || !ExtraStruct) return false;

	UK2Node_FunctionEntry* Entry = nullptr;
	UK2Node_FunctionResult* Result = nullptr;
	ResetFunctionGraph(Graph, Entry, Result);
	if (!Entry || !Result) return false;

	FPinHelper Helper(Graph);
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	
	Entry->CustomGeneratedFunctionName = *FunctionName;
	Entry->AddExtraFlags(FUNC_BlueprintCallable | FUNC_Static | FUNC_Public);
	Entry->NodePosX = 0;
	Entry->NodePosY = 0;

	while (Entry->UserDefinedPins.Num() > 0)
	{ Entry->RemoveUserDefinedPin(Entry->UserDefinedPins[0]); }
	while (Result->UserDefinedPins.Num() > 0)
	{ Result->RemoveUserDefinedPin(Result->UserDefinedPins[0]); }

	FEdGraphPinType ItemPinType;
	ItemPinType.PinCategory = Schema->PC_Object;
	ItemPinType.PinSubCategoryObject = UItemData::StaticClass();
	UEdGraphPin* EntryItemPin = Entry->CreateUserDefinedPin(TEXT("Item"), ItemPinType, EGPD_Output);

	FEdGraphPinType ValuePinType;
	ValuePinType.PinCategory = Schema->PC_Struct;
	ValuePinType.PinSubCategoryObject = const_cast<UScriptStruct*>(ExtraStruct);
	UEdGraphPin* EntryNewValuePin = Entry->CreateUserDefinedPin(TEXT("NewValue"), ValuePinType, EGPD_Output);

	Result->NodePosX = 1450; // V6
	Result->NodePosY = 0;

	UFunction* MakeInstancedFunc = UBlueprintInstancedStructLibrary::StaticClass()->FindFunctionByName(TEXT("MakeInstancedStruct"));
	if (!MakeInstancedFunc) return false;

	UK2Node_CallFunction* CallMakeInstanced = Helper.SpawnNode<UK2Node_CallFunction>(850.0f, 0.0f);
	CallMakeInstanced->SetFromFunction(MakeInstancedFunc);
	CallMakeInstanced->AllocateDefaultPins();
	
	if (EntryNewValuePin)
	{
		UEdGraphPin* ValueIn = Helper.FindPin(CallMakeInstanced, FName(TEXT("Value")));
		if (!ValueIn) ValueIn = Helper.FindPinByCategory(CallMakeInstanced, Schema->PC_Wildcard, EGPD_Input);
		
		if (ValueIn)
		{
			ValueIn->PinType = ValuePinType;
			Helper.Connect(EntryNewValuePin, ValueIn);
		}
	}

	UFunction* SetExtraInfoFunc = UItemData::StaticClass()->FindFunctionByName(TEXT("SetExtraInfo"));
	if (!SetExtraInfoFunc) return false;

	UK2Node_CallFunction* CallSetInfo = Helper.SpawnNode<UK2Node_CallFunction>(1100.0f, 0.0f);
	CallSetInfo->SetFromFunction(SetExtraInfoFunc);
	CallSetInfo->AllocateDefaultPins();
	
	if (EntryItemPin)
	{
		UEdGraphPin* Target = Helper.FindPin(CallSetInfo, UEdGraphSchema_K2::PN_Self);
		if (!Target) Target = Helper.FindPin(CallSetInfo, FName(TEXT("Target")));
		if (Target) Helper.Connect(EntryItemPin, Target);
	}

	Helper.Connect(CallMakeInstanced->GetReturnValuePin(), Helper.FindPin(CallSetInfo, FName(TEXT("NewExtraInfo"))));
	
	Helper.ConnectExec(Entry, CallMakeInstanced);
	Helper.Connect(CallMakeInstanced->GetThenPin(), CallSetInfo->GetExecPin());
	Helper.Connect(CallSetInfo->GetThenPin(), Helper.GetToExecPin(Result));

	return true;
}

bool FAccessorGenerator::RebuildLibrary(UBlueprint *Library, const UScriptStruct *ExtraStruct)
{
	if (!Library || !ExtraStruct) return false;

	const UDFInventorySettings* Settings = GetDefault<UDFInventorySettings>();
	const bool bLog = Settings && Settings->bVerboseLogging;

	if (bLog)
	{
		UE_LOG(LogTemp, Log, TEXT("DFInventory: Rebuilding library for struct %s"), *ExtraStruct->GetName());
	}

	TArray<const FProperty *> Properties;
	for (TFieldIterator<FProperty> It(ExtraStruct); It; ++It)
	{
		if (It->HasAllPropertyFlags(CPF_ReturnParm)) continue;
		Properties.Add(*It);
	}

	TSet<FName> NeededGraphs;
	bool bChanged = false;

	// Build Whole Struct Accessors
	{
		const FString GetterName = TEXT("GetExtraInfo");
		const FString SetterName = TEXT("SetExtraInfo");

		NeededGraphs.Add(*GetterName);
		NeededGraphs.Add(*SetterName);

		auto EnsureAndBuildStructGraph = [&](const FString& Name, bool bGetter)
		{
			UEdGraph* Graph = nullptr;
			for (UEdGraph* TestGraph : Library->FunctionGraphs)
			{
				if (TestGraph->GetFName() == *Name)
				{ Graph = TestGraph; break; }
			}

			if (!Graph)
			{
				Graph = FBlueprintEditorUtils::CreateNewGraph(Library, *Name, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
				FBlueprintEditorUtils::AddFunctionGraph(Library, Graph, true, static_cast<UFunction*>(nullptr));
			}
			const bool bBuilt = bGetter ? BuildStructGetterGraph(Library, Graph, ExtraStruct, Name) : BuildStructSetterGraph(Library, Graph, ExtraStruct, Name);

			if (bBuilt)
			{
				if (FKismetUserDeclaredFunctionMetadata* Meta = FBlueprintEditorUtils::GetGraphFunctionMetaData(Graph))
				{ Meta->SetMetaData(GeneratedMetaKey, FString(TEXT("true"))); }
				bChanged = true;
			}
		};
		
		EnsureAndBuildStructGraph(GetterName, true);
		EnsureAndBuildStructGraph(SetterName, false);
	}

	for (const FProperty *Property : Properties)
	{
		FString PropName = Property->GetAuthoredName();
		if (PropName.IsEmpty()) PropName = Property->GetName();

		PropName = SanitizeIdentifier(PropName);

		const FString GetterName = MakeFunctionName(TEXT("Get"), PropName);
		const FString SetterName = MakeFunctionName(TEXT("Set"), PropName);

		NeededGraphs.Add(*GetterName);
		NeededGraphs.Add(*SetterName);

		auto EnsureAndBuildGraph = [&](const FString &Name, bool bGetter)
		{
			UEdGraph *Graph = nullptr;
			for (UEdGraph *TestGraph : Library->FunctionGraphs)
			{
				if (TestGraph->GetFName() == *Name)
				{ Graph = TestGraph; break; }
			}

			if (!Graph)
			{
				Graph = FBlueprintEditorUtils::CreateNewGraph(Library, *Name, UEdGraph::StaticClass(),UEdGraphSchema_K2::StaticClass());
				FBlueprintEditorUtils::AddFunctionGraph(Library, Graph, true,static_cast<UFunction*>(nullptr));
			}
			const bool bBuilt = bGetter ? BuildGetterGraph(Library, Graph, ExtraStruct, Property, Name) : BuildSetterGraph(Library, Graph, ExtraStruct, Property, Name);

			if (bBuilt)
			{
				if (FKismetUserDeclaredFunctionMetadata *Meta = FBlueprintEditorUtils::GetGraphFunctionMetaData(Graph))
				{ Meta->SetMetaData(GeneratedMetaKey, FString(TEXT("true"))); }
				bChanged = true;
			}
		};
		EnsureAndBuildGraph(GetterName, true);
		EnsureAndBuildGraph(SetterName, false);
	}

	for (int32 i = Library->FunctionGraphs.Num() - 1; i >= 0; i--)
	{
		UEdGraph *Graph = Library->FunctionGraphs[i];
		if (!Graph) continue;

		bool bIsGenerated = false;
		if (FKismetUserDeclaredFunctionMetadata *MetaData = FBlueprintEditorUtils::GetGraphFunctionMetaData(Graph))
		{
			if (MetaData->GetMetaData(GeneratedMetaKey) == TEXT("true"))
			{ bIsGenerated = true; }
		}

		if (!NeededGraphs.Contains(Graph->GetFName()) && bIsGenerated)
		{ FBlueprintEditorUtils::RemoveGraph(Library, Graph); bChanged = true; }
	}

	if (bChanged)
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Library);
		FKismetEditorUtilities::CompileBlueprint(Library);
		if (UPackage *Package = Library->GetOutermost())
		{ return Package->MarkPackageDirty(); }
	}
	return true;
}

bool FAccessorGenerator::Generate()
{
	if (IsEngineExitRequested()) return false;

	UDFInventorySettings *Settings = GetMutableDefault<UDFInventorySettings>();
	if (!Settings) return false;
	Settings->EnsureExtraInfoValid();

	if (Settings->bVerboseLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("DFInventory: Checking if generation is needed..."));
	}
	
	const UScriptStruct *ExtraStruct = nullptr;
	if (!Settings->ExtraInfo.IsNull())
	{ ExtraStruct = Settings->ExtraInfo.LoadSynchronous(); }
	if (!ExtraStruct && Settings->InstancedExtraInfo.IsValid())
	{ ExtraStruct = Settings->InstancedExtraInfo.GetScriptStruct(); }
	if (!ExtraStruct) return false;

	UBlueprint *Library = LoadOrCreateLibrary();
	if (!Library) return false;

	const FString HashKey = TEXT("DFInvStructHash");
	const FString CurrentHash =	FString::Printf(TEXT("%u"), ComputeStructHash(ExtraStruct));

	FString OldHash;
	const FString ConfigSection = TEXT("DFInventoryAccessorGenerator");
	GConfig->GetString(*ConfigSection, *HashKey, OldHash, GEditorPerProjectIni);

	if (OldHash == CurrentHash) return true;

	if (RebuildLibrary(Library, ExtraStruct))
	{
		GConfig->SetString(*ConfigSection, *HashKey, *CurrentHash,
		GEditorPerProjectIni);
		GConfig->SetString(*ConfigSection, *HashKey, *CurrentHash,
		GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);

		if (Settings->bVerboseLogging)
		{
			UE_LOG(LogTemp, Log, TEXT("DFInventory: Generation complete."));
		}
		return true;
	}
	
	if (Settings->bVerboseLogging)
	{
		UE_LOG(LogTemp, Log, TEXT("DFInventory: No changes detected, skipping generation."));
	}
	return false;
}

#endif
