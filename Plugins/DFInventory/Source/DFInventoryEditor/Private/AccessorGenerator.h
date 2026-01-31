#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "AccessorGenerator.generated.h"

class UK2Node_FunctionResult;
class UK2Node_FunctionEntry;

USTRUCT()
struct FAccessorGenerator
{
	GENERATED_BODY()

public:
	
	static bool Generate();

private:
	
	static FString GetLibraryPackagePath();
	static UBlueprint* LoadOrCreateLibrary();
	static FString SanitizeIdentifier(FString Input);
	static bool RebuildLibrary(UBlueprint* Library, const UScriptStruct* ExtraStruct);
	static FString MakeFunctionName(const FString& Prefix, const FString& PropertyName);
	static bool ConvertPropertyToPinType(const FProperty* Property, FEdGraphPinType& OutPinType);
	static void ResetFunctionGraph(UEdGraph* Graph, UK2Node_FunctionEntry*& OutEntry, UK2Node_FunctionResult*& OutResult);
	static bool BuildGetterGraph(UBlueprint* Library, UEdGraph* Graph, const UScriptStruct* ExtraStruct, const FProperty* Property, const FString& FunctionName);
	static bool BuildSetterGraph(UBlueprint* Library, UEdGraph* Graph, const UScriptStruct* ExtraStruct, const FProperty* Property, const FString& FunctionName);
	static bool BuildStructGetterGraph(UBlueprint* Library, UEdGraph* Graph, const UScriptStruct* ExtraStruct, const FString& FunctionName);
	static bool BuildStructSetterGraph(UBlueprint* Library, UEdGraph* Graph, const UScriptStruct* ExtraStruct, const FString& FunctionName);
};

#endif
