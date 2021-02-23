// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#pragma once

#include "CoreMinimal.h"
#include "ALSPlayerCameraManager.h"
#include "ALSPlayerCameraManagerTopDown.generated.h"

class AALSCharacterTopDown;

/**
 * Player camera manager class
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API AALSPlayerCameraManagerTopDown : public AALSPlayerCameraManager
{
	GENERATED_BODY()

public:
	virtual void OnPossess(AALSBaseCharacter* NewCharacter) override;

protected:
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	virtual bool CustomCameraBehaviorTopDown(float DeltaTime, FVector& Location, FRotator& Rotation, float& FOV);

	UFUNCTION(BlueprintCallable)
	static FVector CalculateAxisIndependentLagTopDown(
		FVector CurrentLocation,
        FVector TargetLocation,
        FRotator CameraRotation,
        FVector AnchorLocation,
        FVector LagSpeeds,
        float LimitRadius,
        float LimitExp,
        float DeltaTime
        );

	static float CalculateTargetLocationInterpAxis(
        float LimitRadius,
        float Alpha,
        float Exp
        );

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	AALSCharacterTopDown* TopDownCharacter = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CameraPivotRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CameraPivotExp = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FixedCameraPitch = -60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float FixedCameraYaw = -45.f;
};
