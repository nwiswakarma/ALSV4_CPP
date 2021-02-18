// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2020 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Haziq Fadhil, Nuraga Wiswakarma


#include "Character/ALSMBaseCharacter.h"

#include "Character/ALSPlayerController.h"
#include "Character/Animation/ALSMCharacterAnimInstance.h"
#include "Library/ALSMathLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveFloat.h"
#include "Character/ALSCharacterMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

/** Constructor */

AALSMBaseCharacter::AALSMBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UALSCharacterMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;
	//MantleTimeline = CreateDefaultSubobject<UTimelineComponent>(FName(TEXT("MantleTimeline")));
	bUseControllerRotationYaw = 0;
	bReplicates = true;
	SetReplicatingMovement(true);
}

/** Base Overrides */

void AALSMBaseCharacter::Restart()
{
	Super::Restart();

	AALSPlayerController* NewController = Cast<AALSPlayerController>(GetController());
	if (NewController)
	{
		NewController->OnRestartPawn(this);
	}
}

void AALSMBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward/Backwards", this, &AALSMBaseCharacter::PlayerForwardMovementInput);
	PlayerInputComponent->BindAxis("MoveRight/Left", this, &AALSMBaseCharacter::PlayerRightMovementInput);
	PlayerInputComponent->BindAxis("LookUp/Down", this, &AALSMBaseCharacter::PlayerCameraUpInput);
	PlayerInputComponent->BindAxis("LookLeft/Right", this, &AALSMBaseCharacter::PlayerCameraRightInput);
	//PlayerInputComponent->BindAction("JumpAction", IE_Pressed, this, &AALSMBaseCharacter::JumpPressedAction);
	//PlayerInputComponent->BindAction("JumpAction", IE_Released, this, &AALSMBaseCharacter::JumpReleasedAction);
	//PlayerInputComponent->BindAction("StanceAction", IE_Pressed, this, &AALSMBaseCharacter::StancePressedAction);
	//PlayerInputComponent->BindAction("WalkAction", IE_Pressed, this, &AALSMBaseCharacter::WalkPressedAction);
	//PlayerInputComponent->BindAction("RagdollAction", IE_Pressed, this, &AALSMBaseCharacter::RagdollPressedAction);
	//PlayerInputComponent->BindAction("SelectRotationMode_1", IE_Pressed, this,
	//                                 &AALSMBaseCharacter::VelocityDirectionPressedAction);
	//PlayerInputComponent->BindAction("SelectRotationMode_2", IE_Pressed, this,
	//                                 &AALSMBaseCharacter::LookingDirectionPressedAction);
	//PlayerInputComponent->BindAction("SprintAction", IE_Pressed, this, &AALSMBaseCharacter::SprintPressedAction);
	//PlayerInputComponent->BindAction("SprintAction", IE_Released, this, &AALSMBaseCharacter::SprintReleasedAction);
	//PlayerInputComponent->BindAction("AimAction", IE_Pressed, this, &AALSMBaseCharacter::AimPressedAction);
	//PlayerInputComponent->BindAction("AimAction", IE_Released, this, &AALSMBaseCharacter::AimReleasedAction);
	//PlayerInputComponent->BindAction("CameraAction", IE_Pressed, this, &AALSMBaseCharacter::CameraPressedAction);
	//PlayerInputComponent->BindAction("CameraAction", IE_Released, this, &AALSMBaseCharacter::CameraReleasedAction);
}

void AALSMBaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	MyCharacterMovementComponent = Cast<UALSCharacterMovementComponent>(Super::GetMovementComponent());
}

void AALSMBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AALSMBaseCharacter, TargetRagdollLocation);
	DOREPLIFETIME_CONDITION(AALSMBaseCharacter, ReplicatedCurrentAcceleration, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AALSMBaseCharacter, ReplicatedControlRotation, COND_SkipOwner);

	DOREPLIFETIME(AALSMBaseCharacter, DesiredGait);
	//DOREPLIFETIME_CONDITION(AALSMBaseCharacter, DesiredStance, COND_SkipOwner);
	//DOREPLIFETIME_CONDITION(AALSMBaseCharacter, DesiredRotationMode, COND_SkipOwner);

	//DOREPLIFETIME_CONDITION(AALSMBaseCharacter, RotationMode, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AALSMBaseCharacter, OverlayState, COND_SkipOwner);
	//DOREPLIFETIME_CONDITION(AALSMBaseCharacter, ViewMode, COND_SkipOwner);
}

void AALSMBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	// If we're in networked game, disable curved movement
	bDisableCurvedMovement = !IsNetMode(ENetMode::NM_Standalone);

	//FOnTimelineFloat TimelineUpdated;
	//FOnTimelineEvent TimelineFinished;
	//TimelineUpdated.BindUFunction(this, FName(TEXT("MantleUpdate")));
	//TimelineFinished.BindUFunction(this, FName(TEXT("MantleEnd")));
	//MantleTimeline->SetTimelineFinishedFunc(TimelineFinished);
	//MantleTimeline->SetLooping(false);
	//MantleTimeline->SetTimelineLengthMode(TL_TimelineLength);
	//MantleTimeline->AddInterpFloat(MantleTimelineCurve, TimelineUpdated);

	// Make sure the mesh and animbp update after the CharacterBP to ensure it gets the most recent values.
	GetMesh()->AddTickPrerequisiteActor(this);

	// Set the Movement Model
	SetMovementModel();

	// Once, force set variables in anim bp.
    //
    // This ensures anim instance & character starts synchronized
	FALSAnimCharacterInformation& AnimData = MainAnimInstance->GetCharacterInformationMutable();
	MainAnimInstance->Gait = DesiredGait;
	//MainAnimInstance->Stance = DesiredStance;
	//MainAnimInstance->RotationMode = DesiredRotationMode;
	//AnimData.ViewMode = ViewMode;
	MainAnimInstance->OverlayState = OverlayState;
	AnimData.PrevMovementState = PrevMovementState;
	MainAnimInstance->MovementState = MovementState;

	// Update states to use the initial desired values.
	SetGait(DesiredGait);
	//SetStance(DesiredStance);
	//SetRotationMode(DesiredRotationMode);
	//SetViewMode(ViewMode);
	SetOverlayState(OverlayState);

    //if (Stance == EALSStance::Standing)
    //{
    //    UnCrouch();
    //}
    //else
    //if (Stance == EALSStance::Crouching)
    //{
    //    Crouch();
    //}

	// Set default rotation values.
	TargetRotation = GetActorRotation();
	LastVelocityRotation = TargetRotation;
	LastMovementInputRotation = TargetRotation;

    if (GetLocalRole() == ROLE_SimulatedProxy)
    {
        MainAnimInstance->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
    }
}

void AALSMBaseCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	MainAnimInstance = Cast<UALSMCharacterAnimInstance>(GetMesh()->GetAnimInstance());
	if (! MainAnimInstance)
	{
		// Animation instance should be assigned if we're not in editor preview
		checkf(GetWorld()->WorldType == EWorldType::EditorPreview,
		       TEXT("%s doesn't have a valid animation instance assigned. That's not allowed"),
		       *GetName());
	}
}

void AALSMBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Set required values
	SetEssentialValues(DeltaTime);

    if (MovementState == EALSMovementState::Grounded)
    {
        UpdateCharacterMovement();
        UpdateGroundedRotation(DeltaTime);
    }
    else
    if (MovementState == EALSMovementState::InAir)
    {
        //UpdateInAirRotation(DeltaTime);

        // Perform a mantle check if falling while movement input is pressed.
        //if (bHasMovementInput)
        //{
        //    MantleCheck(FallingTraceSettings);
        //}
    }
    else
    if (MovementState == EALSMovementState::Ragdoll)
    {
        //RagdollUpdate(DeltaTime);
    }

	// Cache values
	PreviousVelocity = GetVelocity();
	PreviousAimYaw = AimingRotation.Yaw;

	DrawDebugSpheres();
}

/** Character States */

void AALSMBaseCharacter::SetMovementState(const EALSMovementState NewState)
{
	if (MovementState != NewState)
	{
		PrevMovementState = MovementState;
		MovementState = NewState;

		FALSAnimCharacterInformation& AnimData(MainAnimInstance->GetCharacterInformationMutable());
		AnimData.PrevMovementState = PrevMovementState;
		MainAnimInstance->MovementState = MovementState;

		OnMovementStateChanged(PrevMovementState);
	}
}

void AALSMBaseCharacter::SetGait(const EALSGait NewGait)
{
	if (Gait != NewGait)
	{
		Gait = NewGait;
		MainAnimInstance->Gait = Gait;
	}
}

/** Movement System */

void AALSMBaseCharacter::SetHasMovementInput(bool bNewHasMovementInput)
{
	bHasMovementInput = bNewHasMovementInput;
	MainAnimInstance->GetCharacterInformationMutable().bHasMovementInput = bHasMovementInput;
}

FALSMovementSettings AALSMBaseCharacter::GetTargetMovementSettings() const
{
    //if (RotationMode == EALSRotationMode::VelocityDirection)
    //{
    //    if (Stance == EALSStance::Standing)
    //    {
    //        return MovementData.VelocityDirection.Standing;
    //    }
    //    if (Stance == EALSStance::Crouching)
    //    {
    //        return MovementData.VelocityDirection.Crouching;
    //    }
    //}
    //else
    //if (RotationMode == EALSRotationMode::LookingDirection)
    //{
    //    if (Stance == EALSStance::Standing)
    //    {
    //        return MovementData.LookingDirection.Standing;
    //    }
    //    if (Stance == EALSStance::Crouching)
    //    {
    //        return MovementData.LookingDirection.Crouching;
    //    }
    //}
    //else
    //if (RotationMode == EALSRotationMode::Aiming)
    //{
    //    if (Stance == EALSStance::Standing)
    //    {
    //        return MovementData.Aiming.Standing;
    //    }
    //    if (Stance == EALSStance::Crouching)
    //    {
    //        return MovementData.Aiming.Crouching;
    //    }
    //}

	// Default to velocity direction standing
	return MovementData.VelocityDirection.Standing;
}

EALSGait AALSMBaseCharacter::GetAllowedGait() const
{
	// Calculate the Allowed Gait.
    //
    // This represents the maximum Gait the character is
    // currently allowed to be in, and can be determined by the desired gait,
    // the rotation mode, the stance, etc.
    //
    // For example, if you wanted to force the character
    // into a walking state while indoors, this could be done here.

    /*

	if (Stance == EALSStance::Standing)
	{
		if (RotationMode != EALSRotationMode::Aiming)
		{
            if (DesiredGait == EALSGait::Sprinting)
            {
                return CanSprint() ? EALSGait::Sprinting : EALSGait::Running;
            }

            return DesiredGait;
		}
	}

	// Crouching stance & Aiming rot mode has same behaviour

	if (DesiredGait == EALSGait::Sprinting)
	{
		return EALSGait::Running;
	}

	return DesiredGait;

    */

    return EALSGait::Running;
}

EALSGait AALSMBaseCharacter::GetActualGait(EALSGait AllowedGait) const
{
	// Get the Actual Gait.
    //
    // This is calculated by the actual movement of the character,
    // and so it can be different from the desired gait or allowed gait.
    //
    // For instance, if the Allowed Gait becomes walking,
	// the Actual gait will still be running until the character
    // decelerates to the walking speed.

	const float LocWalkSpeed = CurrentMovementSettings.WalkSpeed;
	const float LocRunSpeed = CurrentMovementSettings.RunSpeed;

	if (Speed > LocRunSpeed + 10.0f)
	{
		//if (AllowedGait == EALSGait::Sprinting)
		//{
		//    return EALSGait::Sprinting;
		//}
		return EALSGait::Running;
	}

	if (Speed >= LocWalkSpeed + 10.0f)
	{
		return EALSGait::Running;
	}

	return EALSGait::Walking;
}

/** State Changes */

void AALSMBaseCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// Use the Character Movement Mode changes
    // to set the Movement States to the right values.
    //
    // This allows you to have a custom set of movement states
    // but still use the functionality of
    // the default character movement component.

	if (GetCharacterMovement()->MovementMode == MOVE_Walking ||
		GetCharacterMovement()->MovementMode == MOVE_NavWalking)
	{
		SetMovementState(EALSMovementState::Grounded);
	}
	else if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		SetMovementState(EALSMovementState::InAir);
	}
}

void AALSMBaseCharacter::OnMovementStateChanged(const EALSMovementState PreviousState)
{
	//if (MovementState == EALSMovementState::InAir)
	//{
	//	if (MovementAction == EALSMovementAction::None)
	//	{
	//		// If the character enters the air,
    //        // set the In Air Rotation and uncrouch if crouched.
	//		InAirRotation = GetActorRotation();

	//		if (Stance == EALSStance::Crouching)
	//		{
	//			UnCrouch();
	//		}
	//	}
	//	else
    //    if (MovementAction == EALSMovementAction::Rolling)
	//	{
    //        // If the character is currently rolling, enable the ragdoll.
    //        ReplicatedRagdollStart();
	//	}
	//}
	//else
    //if (MovementState == EALSMovementState::Ragdoll && PreviousState == EALSMovementState::Mantling)
	//{
	//	// Stop the Mantle Timeline if transitioning
    //    // to the ragdoll state while mantling.
	//	MantleTimeline->Stop();
	//}
}

//void AALSMBaseCharacter::OnRotationModeChanged(EALSRotationMode PreviousRotationMode)
//{
//    MainAnimInstance->RotationMode = RotationMode;
//
//    if (RotationMode == EALSRotationMode::VelocityDirection && ViewMode == EALSViewMode::FirstPerson)
//    {
//        // If the new rotation mode is Velocity Direction and the character is in First Person,
//        // set the viewmode to Third Person.
//        SetViewMode(EALSViewMode::ThirdPerson);
//    }
//}

void AALSMBaseCharacter::OnOverlayStateChanged(const EALSOverlayState PreviousState)
{
	MainAnimInstance->OverlayState = OverlayState;
}

void AALSMBaseCharacter::SetEssentialValues(float DeltaTime)
{
	if (GetLocalRole() != ROLE_SimulatedProxy)
	{
		ReplicatedCurrentAcceleration = GetCharacterMovement()->GetCurrentAcceleration();
		ReplicatedControlRotation = GetControlRotation();
		EasedMaxAcceleration = GetCharacterMovement()->GetMaxAcceleration();
	}

	else
	{
		EasedMaxAcceleration = GetCharacterMovement()->GetMaxAcceleration() != 0
           ? GetCharacterMovement()->GetMaxAcceleration()
           : EasedMaxAcceleration / 2;
	}

	// Interp AimingRotation to current control rotation
    // for smooth character rotation movement.
    //
    // Decrease InterpSpeed for slower but smoother movement.
	AimingRotation = FMath::RInterpTo(
        AimingRotation,
        ReplicatedControlRotation,
        DeltaTime,
        30
        );

	// These values represent how the capsule
    // is moving as well as how it wants to move,
    // and therefore are essential for any data driven animation system.
    //
    // They are also used throughout the system for various functions,
	// so I found it is easiest to manage them all in one place.

	const FVector CurrentVel = GetVelocity();

	// Set the amount of Acceleration.
	SetAcceleration((CurrentVel - PreviousVelocity) / DeltaTime);

	// Determine if the character is moving by getting it's speed.
    // The Speed equals the length of the horizontal (x y) velocity,
    // so it does not take vertical movement into account.
    //
    // If the character is moving, update the last velocity rotation.
    // This value is saved because it might be useful
    // to know the last orientation of movement even after
    // the character has stopped.
	SetSpeed(CurrentVel.Size2D());
	SetIsMoving(Speed > 1.0f);
	if (bIsMoving)
	{
		LastVelocityRotation = CurrentVel.ToOrientationRotator();
	}

	// Determine if the character has movement input
    // by getting its movement input amount.
    //
	// The Movement Input Amount is equal to
    // the current acceleration divided by the max acceleration so that
	// it has a range of 0-1, 1 being the maximum possible amount of input,
    // and 0 being none.
    //
	// If the character has movement input,
    // update the Last Movement Input Rotation.
	SetMovementInputAmount(ReplicatedCurrentAcceleration.Size() / EasedMaxAcceleration);
	SetHasMovementInput(MovementInputAmount > 0.0f);
	if (bHasMovementInput)
	{
		LastMovementInputRotation = ReplicatedCurrentAcceleration.ToOrientationRotator();
	}

	// Set the Aim Yaw rate by comparing the current and previous Aim Yaw value,
    // divided by Delta Seconds.
    //
	// This represents the speed the camera is rotating left to right.
	SetAimYawRate(FMath::Abs((AimingRotation.Yaw - PreviousAimYaw) / DeltaTime));
}

void AALSMBaseCharacter::UpdateCharacterMovement()
{
	// Set the Allowed Gait
	const EALSGait AllowedGait = GetAllowedGait();

	// Determine the Actual Gait.
    //
    // If it is different from the current Gait, Set the new Gait Event.
	const EALSGait ActualGait = GetActualGait(AllowedGait);

	if (ActualGait != Gait)
	{
		SetGait(ActualGait);
	}

	// Use the allowed gait to update the movement settings.
	if (bDisableCurvedMovement)
	{
		// Don't use curves for movement
		UpdateDynamicMovementSettingsNetworked(AllowedGait);
	}
	else
	{
		// Use curves for movement
		UpdateDynamicMovementSettingsStandalone(AllowedGait);
	}
}

void AALSMBaseCharacter::UpdateDynamicMovementSettingsNetworked(EALSGait AllowedGait)
{
	// Get the Current Movement Settings.
	CurrentMovementSettings = GetTargetMovementSettings();
	const float NewMaxSpeed = CurrentMovementSettings.GetSpeedForGait(AllowedGait);

	// Update the Character Max Walk Speed to the configured speeds
    // based on the currently Allowed Gait.
	if (IsLocallyControlled() || HasAuthority())
	{
		if (GetCharacterMovement()->MaxWalkSpeed != NewMaxSpeed)
		{
			MyCharacterMovementComponent->SetMaxWalkingSpeed(NewMaxSpeed);
		}
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = NewMaxSpeed;
	}
}

void AALSMBaseCharacter::UpdateDynamicMovementSettingsStandalone(EALSGait AllowedGait)
{
	// Get the Current Movement Settings.
	CurrentMovementSettings = GetTargetMovementSettings();
	const float NewMaxSpeed = CurrentMovementSettings.GetSpeedForGait(AllowedGait);

	// Update the Acceleration, Deceleration, and Ground Friction
    // using the Movement Curve.
    //
	// This allows for fine control over movement behavior
    // at each speed (May not be suitable for replication).
	const float MappedSpeed = GetMappedSpeed();
	const FVector CurveVec = CurrentMovementSettings.MovementCurve->GetVectorValue(MappedSpeed);

	// Update the Character Max Walk Speed to the configured speeds based on the currently Allowed Gait.
	MyCharacterMovementComponent->SetMaxWalkingSpeed(NewMaxSpeed);
	GetCharacterMovement()->MaxAcceleration = CurveVec.X;
	GetCharacterMovement()->BrakingDecelerationWalking = CurveVec.Y;
	GetCharacterMovement()->GroundFriction = CurveVec.Z;
}

void AALSMBaseCharacter::UpdateGroundedRotation(float DeltaTime)
{
    /*
	if (MovementAction == EALSMovementAction::None)
	{
		const bool bCanUpdateMovingRot = ((bIsMoving && bHasMovementInput) || Speed > 150.0f) && !HasAnyRootMotion();

		if (bCanUpdateMovingRot)
		{
			const float GroundedRotationRate = CalculateGroundedRotationRate();
			if (RotationMode == EALSRotationMode::VelocityDirection)
			{
				// Velocity Direction Rotation
				SmoothCharacterRotation({0.0f, LastVelocityRotation.Yaw, 0.0f}, 800.0f, GroundedRotationRate,
				                        DeltaTime);
			}
			else if (RotationMode == EALSRotationMode::LookingDirection)
			{
				// Looking Direction Rotation
				float YawValue;
				if (Gait == EALSGait::Sprinting)
				{
					YawValue = LastVelocityRotation.Yaw;
				}
				else
				{
					// Walking or Running..
					const float YawOffsetCurveVal = MainAnimInstance->GetCurveValue(FName(TEXT("YawOffset")));
					YawValue = AimingRotation.Yaw + YawOffsetCurveVal;
				}
				SmoothCharacterRotation({0.0f, YawValue, 0.0f}, 500.0f, GroundedRotationRate, DeltaTime);
			}
			else if (RotationMode == EALSRotationMode::Aiming)
			{
				const float ControlYaw = AimingRotation.Yaw;
				SmoothCharacterRotation({0.0f, ControlYaw, 0.0f}, 1000.0f, 20.0f, DeltaTime);
			}
		}
		else
		{
			// Not Moving

			if ((ViewMode == EALSViewMode::ThirdPerson && RotationMode == EALSRotationMode::Aiming) ||
				ViewMode == EALSViewMode::FirstPerson)
			{
				LimitRotation(-100.0f, 100.0f, 20.0f, DeltaTime);
			}

			// Apply the RotationAmount curve from Turn In Place Animations.
			// The Rotation Amount curve defines how much rotation should be applied each frame,
			// and is calculated for animations that are animated at 30fps.

			const float RotAmountCurve = MainAnimInstance->GetCurveValue(FName(TEXT("RotationAmount")));

			if (FMath::Abs(RotAmountCurve) > 0.001f)
			{
				if (GetLocalRole() == ROLE_AutonomousProxy)
				{
					TargetRotation.Yaw = UKismetMathLibrary::NormalizeAxis(
						TargetRotation.Yaw + (RotAmountCurve * (DeltaTime / (1.0f / 30.0f))));
					SetActorRotation(TargetRotation);
				}
				else
				{
					AddActorWorldRotation({0, RotAmountCurve * (DeltaTime / (1.0f / 30.0f)), 0});
				}
				TargetRotation = GetActorRotation();
			}
		}
	}
	else
    if (MovementAction == EALSMovementAction::Rolling)
	{
        // Rolling Rotation

        if (bHasMovementInput)
        {
            SmoothCharacterRotation({0.0f, LastMovementInputRotation.Yaw, 0.0f}, 0.0f, 2.0f, DeltaTime);
        }
	}

	// Other actions are ignored...
    */

    const bool bCanUpdateMovingRot = ((bIsMoving && bHasMovementInput) || Speed > 150.0f) && !HasAnyRootMotion();

    if (bCanUpdateMovingRot)
    {
        const float GroundedRotationRate = CalculateGroundedRotationRate();

        const float YawOffsetCurveVal = MainAnimInstance->GetCurveValue(FName(TEXT("YawOffset")));
        float YawValue = AimingRotation.Yaw + YawOffsetCurveVal;

        SmoothCharacterRotation(
            FRotator(0.0f, YawValue, 0.0f),
            500.0f,
            GroundedRotationRate,
            DeltaTime
            );
    }
    // Not Moving
    else
    {
        // Apply the RotationAmount curve from Turn In Place Animations.
        //
        // The Rotation Amount curve defines how much rotation
        // should be applied each frame, and is calculated for animations
        // that are animated at 30fps.

        const float RotAmountCurve = MainAnimInstance->GetCurveValue(FName(TEXT("RotationAmount")));

        if (FMath::Abs(RotAmountCurve) > 0.001f)
        {
            if (GetLocalRole() == ROLE_AutonomousProxy)
            {
                TargetRotation.Yaw = UKismetMathLibrary::NormalizeAxis(
                    TargetRotation.Yaw + (RotAmountCurve * (DeltaTime / (1.0f / 30.0f))));
                SetActorRotation(TargetRotation);
            }
            else
            {
                AddActorWorldRotation({0, RotAmountCurve * (DeltaTime / (1.0f / 30.0f)), 0});
            }
            TargetRotation = GetActorRotation();
        }
    }
}

/** Utils */

float AALSMBaseCharacter::GetMappedSpeed() const
{
	// Map the character's current speed to the configured
    // movement speeds with a range of 0-3, with:
    // 0 = stopped,
    // 1 = the Walk Speed,
    // 2 = the Run Speed,
    // 3 = the Sprint Speed.
    //
	// This allows us to vary the movement speeds
    // but still use the mapped range in calculations for consistent results

	const float LocWalkSpeed = CurrentMovementSettings.WalkSpeed;
	const float LocRunSpeed = CurrentMovementSettings.RunSpeed;
	const float LocSprintSpeed = CurrentMovementSettings.SprintSpeed;

	if (Speed > LocRunSpeed)
	{
		return FMath::GetMappedRangeValueClamped({LocRunSpeed, LocSprintSpeed}, {2.0f, 3.0f}, Speed);
	}

	if (Speed > LocWalkSpeed)
	{
		return FMath::GetMappedRangeValueClamped({LocWalkSpeed, LocRunSpeed}, {1.0f, 2.0f}, Speed);
	}

	return FMath::GetMappedRangeValueClamped({0.0f, LocWalkSpeed}, {0.0f, 1.0f}, Speed);
}

void AALSMBaseCharacter::SmoothCharacterRotation(
    FRotator Target,
    float TargetInterpSpeed,
    float ActorInterpSpeed,
    float DeltaTime
    )
{
	// Interpolate the Target Rotation for extra smooth rotation behavior
	TargetRotation = FMath::RInterpConstantTo(
        TargetRotation,
        Target,
        DeltaTime,
        TargetInterpSpeed
        );

	SetActorRotation(
		FMath::RInterpTo(
            GetActorRotation(),
            TargetRotation,
            DeltaTime,
            ActorInterpSpeed
            ) );
}

float AALSMBaseCharacter::CalculateGroundedRotationRate() const
{
	// Calculate the rotation rate by using the current Rotation Rate Curve
    // in the Movement Settings.
    //
	// Using the curve in conjunction with the mapped speed
    // gives you a high level of control over the rotation rates for each speed.
    // Increase the speed if the camera is rotating quickly
    // for more responsive rotation.

	const float MappedSpeedVal = GetMappedSpeed();
	const float CurveVal = CurrentMovementSettings.RotationRateCurve->GetFloatValue(MappedSpeedVal);
	const float ClampedAimYawRate = FMath::GetMappedRangeValueClamped({0.0f, 300.0f}, {1.0f, 3.0f}, AimYawRate);
	return CurveVal * ClampedAimYawRate;
}

void AALSMBaseCharacter::SetOverlayState(const EALSOverlayState NewState)
{
	if (OverlayState != NewState)
	{
		const EALSOverlayState Prev = OverlayState;
		OverlayState = NewState;
		OnOverlayStateChanged(Prev);

		if (GetLocalRole() == ROLE_AutonomousProxy)
		{
			Server_SetOverlayState(NewState);
		}
	}
}

void AALSMBaseCharacter::Server_SetOverlayState_Implementation(EALSOverlayState NewState)
{
	SetOverlayState(NewState);
}

void AALSMBaseCharacter::SetMovementModel()
{
    if (! MovementModel.DataTable)
    {
		// No movement model, warn and abort
		//warnf(! MovementModel.DataTable,
		//       TEXT("%s doesn't have a valid movement model"),
		//       *GetName());
        return;
    }

	const FString ContextString = GetFullName();
	FALSMovementStateSettings* OutRow =
		MovementModel.DataTable->FindRow<FALSMovementStateSettings>(
            MovementModel.RowName,
            ContextString
            );
	check(OutRow);
	MovementData = *OutRow;
}

/** Input */

void AALSMBaseCharacter::PlayerForwardMovementInput(float Value)
{
	if (MovementState == EALSMovementState::Grounded || MovementState == EALSMovementState::InAir)
	{
		// Default camera relative movement behavior
		const float Scale = UALSMathLibrary::FixDiagonalGamepadValues(Value, GetInputAxisValue("MoveRight/Left")).Key;
		const FRotator DirRotator(0.0f, AimingRotation.Yaw, 0.0f);
		AddMovementInput(UKismetMathLibrary::GetForwardVector(DirRotator), Scale);
	}
}

void AALSMBaseCharacter::PlayerRightMovementInput(float Value)
{
	if (MovementState == EALSMovementState::Grounded || MovementState == EALSMovementState::InAir)
	{
		// Default camera relative movement behavior
		const float Scale = UALSMathLibrary::FixDiagonalGamepadValues(GetInputAxisValue("MoveForward/Backwards"), Value)
			.Value;
		const FRotator DirRotator(0.0f, AimingRotation.Yaw, 0.0f);
		AddMovementInput(UKismetMathLibrary::GetRightVector(DirRotator), Scale);
	}
}

void AALSMBaseCharacter::PlayerCameraUpInput(float Value)
{
	AddControllerPitchInput(LookUpDownRate * Value);
}

void AALSMBaseCharacter::PlayerCameraRightInput(float Value)
{
	AddControllerYawInput(LookLeftRightRate * Value);
}

/** Camera System */

ECollisionChannel AALSMBaseCharacter::GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius)
{
	TraceOrigin = GetActorLocation();
	TraceRadius = 10.0f;
	return ECC_Visibility;
}

FTransform AALSMBaseCharacter::GetThirdPersonPivotTarget()
{
	return GetActorTransform();
}

FVector AALSMBaseCharacter::GetFirstPersonCameraTarget()
{
	return GetMesh()->GetSocketLocation(FName(TEXT("FP_Camera")));
}

void AALSMBaseCharacter::GetCameraParameters(float& TPFOVOut, float& FPFOVOut, bool& bRightShoulderOut) const
{
	TPFOVOut = ThirdPersonFOV;
	FPFOVOut = FirstPersonFOV;
	bRightShoulderOut = bRightShoulder;
}

void AALSMBaseCharacter::SetAcceleration(const FVector& NewAcceleration)
{
	Acceleration = (NewAcceleration != FVector::ZeroVector || IsLocallyControlled())
       ? NewAcceleration
       : Acceleration / 2;

	MainAnimInstance->GetCharacterInformationMutable().Acceleration = Acceleration;
}

void AALSMBaseCharacter::SetIsMoving(bool bNewIsMoving)
{
	bIsMoving = bNewIsMoving;
	MainAnimInstance->GetCharacterInformationMutable().bIsMoving = bIsMoving;
}

FVector AALSMBaseCharacter::GetMovementInput() const
{
	return ReplicatedCurrentAcceleration;
}

void AALSMBaseCharacter::SetMovementInputAmount(float NewMovementInputAmount)
{
	MovementInputAmount = NewMovementInputAmount;
	MainAnimInstance->GetCharacterInformationMutable().MovementInputAmount = MovementInputAmount;
}

void AALSMBaseCharacter::SetSpeed(float NewSpeed)
{
	Speed = NewSpeed;
	MainAnimInstance->GetCharacterInformationMutable().Speed = Speed;
}

void AALSMBaseCharacter::SetAimYawRate(float NewAimYawRate)
{
	AimYawRate = NewAimYawRate;
	MainAnimInstance->GetCharacterInformationMutable().AimYawRate = AimYawRate;
}

/** Replication */

//void AALSMBaseCharacter::OnRep_RotationMode(EALSRotationMode PrevRotMode)
//{
//    OnRotationModeChanged(PrevRotMode);
//}

void AALSMBaseCharacter::OnRep_OverlayState(EALSOverlayState PrevOverlayState)
{
	OnOverlayStateChanged(PrevOverlayState);
}
