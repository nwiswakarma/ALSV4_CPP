// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#pragma once

#include "CoreMinimal.h"
#include "Character/ALSCharacter.h"
#include "ALSCharacterTopDown.generated.h"

/**
 * Specialized character class, with additional features like held object etc.
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API AALSCharacterTopDown : public AALSCharacter
{
	GENERATED_BODY()

public:
	AALSCharacterTopDown(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	//virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FTransform GetThirdPersonPivotTarget() override;

	UFUNCTION(BlueprintCallable, Category = "ALS|Camera System")
	virtual FVector GetThirdPersonPivotAnchorLocation();

	UFUNCTION(BlueprintCallable)
	FVector GetAimingLocation() const { return AimingLocation; }

	UFUNCTION(BlueprintCallable)
	FVector GetControlLocation() const { return ControlLocation; }

public:
	//UPROPERTY(BlueprintReadOnly, Replicated, Category = "ALS|Essential Information")
	//FVector ReplicatedControlLocation = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float AimMaxRadius = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FRotator MovementFixedRotation;

protected:
	virtual void PlayerForwardMovementInput(float Value) override;

	virtual void PlayerRightMovementInput(float Value) override;

	virtual void PlayerCameraUpInput(float Value) override;

	virtual void PlayerCameraRightInput(float Value) override;

	virtual void SetEssentialValues(float DeltaTime) override;

    virtual void UpdateAimInput(float Value);

protected:
	FVector AimingLocation = FVector::ZeroVector;

	FVector ControlLocation = FVector::ZeroVector;
};
