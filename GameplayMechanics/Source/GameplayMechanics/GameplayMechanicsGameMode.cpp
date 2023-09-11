// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayMechanicsGameMode.h"
#include "GameplayMechanicsHUD.h"
#include "GameplayMechanicsCharacter.h"
#include "UObject/ConstructorHelpers.h"

AGameplayMechanicsGameMode::AGameplayMechanicsGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AGameplayMechanicsHUD::StaticClass();
}
