// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2020 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#include "Character/ALSPlayerController.h"
#include "Character/ALSPlayerCameraManager.h"
#include "Character/ALSBaseCharacter.h"
#include "Character/ALSMBaseCharacter.h"

void AALSPlayerController::OnRestartPawn(APawn* NewPawn)
{
    /*
	PossessedCharacter = Cast<AALSBaseCharacter>(NewPawn);
	check(PossessedCharacter);

	// Call "OnPossess" in Player Camera Manager when possessing a pawn
	AALSPlayerCameraManager* CastedMgr = Cast<AALSPlayerCameraManager>(PlayerCameraManager);
	if (CastedMgr)
	{
		CastedMgr->OnPossess(PossessedCharacter);
	}
    */

    if (NewPawn->IsA(AALSBaseCharacter::StaticClass()))
    {
        PossessedCharacter = Cast<AALSBaseCharacter>(NewPawn);
        check(PossessedCharacter);

        // Call "OnPossess" in Player Camera Manager when possessing a pawn
        AALSPlayerCameraManager* CastedMgr = Cast<AALSPlayerCameraManager>(PlayerCameraManager);
        if (CastedMgr)
        {
            CastedMgr->OnPossess(PossessedCharacter);
        }
    }
    else
    if (NewPawn->IsA(AALSMBaseCharacter::StaticClass()))
    {
        PossessedCharacterM = Cast<AALSMBaseCharacter>(NewPawn);
        check(PossessedCharacterM);

        // Call "OnPossess" in Player Camera Manager when possessing a pawn
        AALSMPlayerCameraManager* CastedMgr = Cast<AALSMPlayerCameraManager>(PlayerCameraManager);
        if (CastedMgr)
        {
            CastedMgr->OnPossess(PossessedCharacterM);
        }
    }
}
