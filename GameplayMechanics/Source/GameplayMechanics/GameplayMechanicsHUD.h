// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "GameplayMechanicsHUD.generated.h"

UCLASS()
class AGameplayMechanicsHUD : public AHUD
{
	GENERATED_BODY()

public:
	AGameplayMechanicsHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

