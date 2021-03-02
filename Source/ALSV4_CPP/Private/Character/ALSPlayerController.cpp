// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Drakynfly


#include "Character/ALSPlayerController.h"
#include "Character/ALSCharacter.h"
#include "Character/ALSPlayerCameraManager.h"

#include "Kismet/KismetSystemLibrary.h"

void AALSPlayerController::OnPossess(APawn* NewPawn)
{
	PossessedCharacter = Cast<AALSBaseCharacter>(NewPawn);
	Super::OnPossess(NewPawn);

	// Servers want to setup camera only in listen servers.
	if (!IsRunningDedicatedServer())
	{
    UKismetSystemLibrary::PrintString(this, FString(TEXT("CHECK OnPossess() ")) + UKismetSystemLibrary::GetDisplayName(Cast<AALSBaseCharacter>(GetPawn())));
		SetupCamera();
	}
}

void AALSPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
    UKismetSystemLibrary::PrintString(this, FString(TEXT("CHECK OnRep_Pawn() ")) + UKismetSystemLibrary::GetDisplayName(Cast<AALSBaseCharacter>(GetPawn())));
	SetupCamera();
}

void AALSPlayerController::SetupCamera()
{
	PossessedCharacter = Cast<AALSBaseCharacter>(GetPawn());

    if (! IsValid(PossessedCharacter))
    {
        return;
    }

	check(PossessedCharacter);

	// Call "OnPossess" in Player Camera Manager when possessing a pawn
	AALSPlayerCameraManager* CastedMgr = Cast<AALSPlayerCameraManager>(PlayerCameraManager);
	if (CastedMgr)
	{
		CastedMgr->OnPossess(PossessedCharacter);
	}
}
