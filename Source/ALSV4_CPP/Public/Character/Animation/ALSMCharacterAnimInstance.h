// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2020 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Library/ALSAnimationStructLibrary.h"
#include "Library/ALSStructEnumLibrary.h"

#include "ALSMCharacterAnimInstance.generated.h"

class AALSMBaseCharacter;
class UCurveFloat;
class UAnimSequence;
class UCurveVector;

/**
 * Main anim instance class for character
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API UALSMCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable)
	void PlayTransition(const FALSDynamicMontageParams& Parameters);

	UFUNCTION(BlueprintCallable)
	void PlayDynamicTransition(
        float ReTriggerDelay,
        FALSDynamicMontageParams Parameters
        );

	UFUNCTION(BlueprintCallable)
	void OnPivot();

	UFUNCTION(BlueprintCallable, Category = "Grounded")
	void SetTrackedHipsDirection(EALSHipsDirection HipsDirection)
	{
		Grounded.TrackedHipsDirection = HipsDirection;
	}

	// Enable Movement Animations if IsMoving and HasMovementInput,
    // or if the Speed is greater than 150.
	UFUNCTION(BlueprintCallable, Category = "Grounded")
	bool ShouldMoveCheck() const;

	/**
	 * Only perform a Turn In Place check if the character
     * is looking toward the camera in Third Person,
	 * and if the "Enable Transition" curve is fully weighted.
     *
     * The Enable_Transition curve is modified within certain
	 * states of the AnimBP so that the character can only turn
     * while in those states.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grounded")
	bool CanTurnInPlace() const;

	/**
	 * Only perform a Dynamic Transition check
     * if the "Enable Transition" curve is fully weighted.
     *
	 * The Enable_Transition curve is modified within
     * certain states of the AnimBP so that the character
     * can only transition while in those states.
	 */
	UFUNCTION(BlueprintCallable, Category = "Grounded")
	bool CanDynamicTransition() const;

	// Return mutable reference of character information
    // to edit them easily inside character class
	FORCEINLINE FALSAnimCharacterInformation& GetCharacterInformationMutable()
	{
		return CharacterInformation;
	}

public:
    bool bHasValidBlendCurves = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Character Information")
	FALSMovementState MovementState = EALSMovementState::None;

	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Character Information")
	//FALSMovementAction MovementAction = EALSMovementAction::None;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Character Information")
	FALSRotationMode RotationMode = EALSRotationMode::LookingDirection;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Character Information")
	FALSGait Gait = EALSGait::Walking;

	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Character Information")
	//FALSStance Stance = EALSStance::Standing;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Character Information")
	FALSOverlayState OverlayState = EALSOverlayState::Default;

    /** Anim Graph - Grounded */

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Grounded")
	FALSVelocityBlend VelocityBlend;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Grounded")
	FALSLeanAmount LeanAmount;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Grounded")
	FVector RelativeAccelerationAmount = FVector::ZeroVector;

	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Grounded")
	//FALSGroundedEntryState GroundedEntryState = EALSGroundedEntryState::None;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Grounded")
	FALSMovementDirection MovementDirection = EALSMovementDirection::Forward;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Aiming Values")
	FVector2D SmoothedAimingAngle = FVector2D::ZeroVector;

protected:
	// References
	UPROPERTY(BlueprintReadOnly)
	AALSMBaseCharacter* Character = nullptr;

	// Character Information
	UPROPERTY(
        VisibleDefaultsOnly,
        BlueprintReadOnly,
        Category = "Read Only Data|Character Information",
        Meta = (ShowOnlyInnerProperties))
	FALSAnimCharacterInformation CharacterInformation;

	// Main Animation Configuration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Main Configuration", Meta = (ShowOnlyInnerProperties))
	FALSAnimConfiguration Config;

    /** Anim Graph */

	// Anim Graph - Grounded
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Grounded", Meta = (ShowOnlyInnerProperties))
	FALSAnimGraphGrounded Grounded;

	// Anim Graph - In Air
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - In Air", Meta = (ShowOnlyInnerProperties))
	FALSAnimGraphInAir InAir;

	// Anim Graph - Aiming Values
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Aiming Values", Meta = (ShowOnlyInnerProperties))
	FALSAnimGraphAimingValues AimingValues;

	// Anim Graph - Ragdoll
	//UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Ragdoll")
	//float FlailRate = 0.0f;

	// Anim Graph - Layer Blending
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Layer Blending", Meta = (ShowOnlyInnerProperties))
	FALSAnimGraphLayerBlending LayerBlendingValues;

	// Anim Graph - Foot IK
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Read Only Data|Anim Graph - Foot IK", Meta = (ShowOnlyInnerProperties))
	FALSAnimGraphFootIK FootIKValues;

    /** Rotational Animation */

	// Turn In Place
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Turn In Place", Meta = (ShowOnlyInnerProperties))
	FALSAnimTurnInPlace TurnInPlaceValues;

	// Rotate In Place
	//UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Rotate In Place", Meta = ( ShowOnlyInnerProperties))
	//FALSAnimRotateInPlace RotateInPlace;

	/** Blend Curves */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveFloat* DiagonalScaleAmountCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveFloat* StrideBlend_N_Walk = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveFloat* StrideBlend_N_Run = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveFloat* StrideBlend_C_Walk = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveFloat* LandPredictionCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveFloat* LeanInAirCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveVector* YawOffset_FB = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Blend Curves")
	UCurveVector* YawOffset_LR = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Dynamic Transition")
	UAnimSequenceBase* TransitionAnim_R = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Configuration|Dynamic Transition")
	UAnimSequenceBase* TransitionAnim_L = nullptr;

private:
	void PlayDynamicTransitionDelay();

	//void OnJumpedDelay();

	void OnPivotDelay();

	/** Update Values */

	void UpdateAimingValues(float DeltaSeconds);

	void UpdateLayerValues();

	void UpdateFootIK(float DeltaSeconds);

	void UpdateMovementValues(float DeltaSeconds);

	void UpdateRotationValues();

	void UpdateInAirValues(float DeltaSeconds);

	//void UpdateRagdollValues();

	/** Foot IK */

	void SetFootLocking(
        float DeltaSeconds,
        FName EnableFootIKCurve,
        FName FootLockCurve,
        FName IKFootBone,
        float& CurFootLockAlpha,
        bool& UseFootLockCurve,
        FVector& CurFootLockLoc,
        FRotator& CurFootLockRot
        );

	void SetFootLockOffsets(
        float DeltaSeconds,
        FVector& LocalLoc,
        FRotator& LocalRot
        );

	void SetPelvisIKOffset(
        float DeltaSeconds,
        FVector FootOffsetLTarget,
        FVector FootOffsetRTarget
        );

	void ResetIKOffsets(float DeltaSeconds);

	void SetFootOffsets(
        float DeltaSeconds,
        FName EnableFootIKCurve,
        FName IKFootBone,
        FName RootBone,
        FVector& CurLocationTarget,
        FVector& CurLocationOffset,
        FRotator& CurRotationOffset
        );

	/** Grounded */

	//void RotateInPlaceCheck();

	void TurnInPlaceCheck(float DeltaSeconds);

	void DynamicTransitionCheck();

	FALSVelocityBlend CalculateVelocityBlend() const;

	void TurnInPlace(
        FRotator TargetRotation,
        float PlayRateScale,
        float StartTime,
        bool OverrideCurrent
        );

	/** Movement */

	FVector CalculateRelativeAccelerationAmount() const;

	float CalculateStrideBlend() const;

	float CalculateWalkRunBlend() const;

	float CalculateStandingPlayRate() const;

	float CalculateDiagonalScaleAmount() const;

	float CalculateCrouchingPlayRate() const;

	float CalculateLandPrediction() const;

	FALSLeanAmount CalculateAirLeanAmount() const;

	EALSMovementDirection CalculateMovementDirection() const;

	/** Util */

	float GetAnimCurveClamped(
        const FName& Name,
        float Bias,
        float ClampMin,
        float ClampMax
        ) const;

private:
	FTimerHandle OnPivotTimer;

	FTimerHandle PlayDynamicTransitionTimer;

	bool bCanPlayDynamicTransition = true;
};
