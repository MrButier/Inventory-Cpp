// Copyright Epic Games, Inc. All Rights Reserved.


#include "MakePluginPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"

void AMakePluginPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Add Input Mapping Contexts
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}
	}
}
