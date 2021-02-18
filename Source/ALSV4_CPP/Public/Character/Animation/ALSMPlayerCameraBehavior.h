// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2020 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Library/ALSCharacterEnumLibrary.h"

#include "ALSMPlayerCameraBehavior.generated.h"

class AALSMBaseCharacter;
class AALSPlayerController;

/**
 * Main class for player camera movement behavior
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API UALSMPlayerCameraBehavior : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AALSMBaseCharacter* ControlledPawn = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	APlayerController* PlayerController = nullptr;

protected:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EALSMovementState MovementState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EALSMovementAction MovementAction;

	//UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	//EALSRotationMode RotationMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EALSGait Gait;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EALSStance Stance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EALSViewMode ViewMode;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bRightShoulder;
};
