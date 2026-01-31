#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node.h"

struct FPinHelper
{
	UEdGraph *Graph;
	const UEdGraphSchema_K2 *Schema;

	FPinHelper(UEdGraph *InGraph) : Graph(InGraph), Schema(GetDefault<UEdGraphSchema_K2>()) {}

	template <typename TNode> TNode *SpawnNode(float X = 0.0f, float Y = 0.0f)
	{
		if (!Graph) return nullptr;
		Graph->Modify();
		TNode *Node = NewObject<TNode>(Graph);
		Node->CreateNewGuid();
		Graph->AddNode(Node, false, false);
		Node->PostPlacedNewNode();
		Node->NodePosX = X;
		Node->NodePosY = Y;
		return Node;
	}

	UEdGraphPin *FindPin(UEdGraphNode *Node, const FName &PinName, EEdGraphPinDirection Direction = EGPD_MAX) const
	{
		if (!Node) return nullptr;

		for (UEdGraphPin *Pin : Node->Pins)
		{
			if (Pin->PinName == PinName)
			{
				if (Direction == EGPD_MAX || Pin->Direction == Direction)
				{ return Pin; }
			}
		}
		return nullptr;
	}

	UEdGraphPin *FindPin(UEdGraphNode *Node, const FString &PinName, EEdGraphPinDirection Direction = EGPD_MAX) const
	{ return FindPin(Node, FName(*PinName), Direction); }

	UEdGraphPin *FindPinByCategory(UEdGraphNode *Node, const FName &Category, EEdGraphPinDirection Direction) const
	{
		if (!Node) return nullptr;
		for (UEdGraphPin *Pin : Node->Pins)
		{
			if (Pin->Direction == Direction && Pin->PinType.PinCategory == Category)
				{ return Pin; }
		}
		return nullptr;
	}

	bool Connect(UEdGraphPin *PinA, UEdGraphPin *PinB)
	{
		if (!PinA || !PinB || !Schema) return false;
		return Schema->TryCreateConnection(PinA, PinB);
	}

	bool FindAndConnect(UEdGraphNode *NodeA, const FName &PinNameA, UEdGraphNode *NodeB, const FName &PinNameB)
	{ return Connect(FindPin(NodeA, PinNameA), FindPin(NodeB, PinNameB)); }

	UEdGraphPin *GetToExecPin(UEdGraphNode *Node) const
	{
		if (!Schema) return nullptr;
		return Schema->FindExecutionPin(*Node, EGPD_Input);
	}

	UEdGraphPin *GetFromExecPin(UEdGraphNode *Node) const
	{
		if (!Schema) return nullptr;
		return Schema->FindExecutionPin(*Node, EGPD_Output);
	}

	bool ConnectExec(UEdGraphNode *NodeA, UEdGraphNode *NodeB)
	{ return Connect(GetFromExecPin(NodeA), GetToExecPin(NodeB)); }
};
