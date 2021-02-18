// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2020 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#include "Character/Animation/ALSMPlayerCameraBehavior.h"

#include "Character/ALSMBaseCharacter.h"

void UALSMPlayerCameraBehavior::NativeUpdateAnimation(float DeltaSeconds)
{
	if (ControlledPawn)
	{
		MovementState = ControlledPawn->GetMovementState();
		MovementAction = ControlledPawn->GetMovementAction();
		//RotationMode = ControlledPawn->GetRotationMode();
		Gait = ControlledPawn->GetGait();
		//Stance = ControlledPawn->GetStance();
		//ViewMode = ControlledPawn->GetViewMode();
		bRightShoulder = ControlledPawn->IsRightShoulder();
	}
}
