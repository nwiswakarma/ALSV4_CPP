// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    


#include "Character/ALSCharacterTopDown.h"
#include "Blueprint/WidgetLayoutLibrary.h"

AALSCharacterTopDown::AALSCharacterTopDown(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MovementFixedRotation = FRotator(0.f, -45.0f, 0.f);
}

void AALSCharacterTopDown::BeginPlay()
{
	Super::BeginPlay();

    ControlLocation = GetActorLocation();
}

FTransform AALSCharacterTopDown::GetThirdPersonPivotTarget()
{
    const FVector BasePivotPos = GetThirdPersonPivotAnchorLocation();
    const FVector AimPosDelta = AimingLocation - BasePivotPos;

    FVector AdjustedAimPos(AimingLocation);

    if (AimPosDelta.SizeSquared() > (AimMaxRadius*AimMaxRadius))
    {
        AdjustedAimPos = BasePivotPos + AimPosDelta.GetClampedToMaxSize(AimMaxRadius);
    }

    return FTransform(
        GetActorRotation(),
        AdjustedAimPos,
        FVector::OneVector
        );
}

FVector AALSCharacterTopDown::GetThirdPersonPivotAnchorLocation()
{
    //return (GetMesh()->GetSocketLocation(TEXT("Head")) + GetMesh()->GetSocketLocation(TEXT("root"))) / 2.0f;
    return GetMesh()->GetSocketLocation(TEXT("root"));
}

void AALSCharacterTopDown::PlayerCameraUpInput(float Value)
{
    //if (! FMath::IsNearlyZero(Value) && Controller && Controller->IsLocalPlayerController())
    if (Controller && Controller->IsLocalPlayerController())
    {
        UpdateAimInput(Value);
    }
}

void AALSCharacterTopDown::PlayerCameraRightInput(float Value)
{
    //if (! FMath::IsNearlyZero(Value) && Controller && Controller->IsLocalPlayerController())
    if (Controller && Controller->IsLocalPlayerController())
    {
        UpdateAimInput(Value);
    }
}

void AALSCharacterTopDown::PlayerForwardMovementInput(float Value)
{
    if (MovementState == EALSMovementState::Grounded || MovementState == EALSMovementState::InAir)
    {
        // Default camera relative movement behavior
        const float Scale = UALSMathLibrary::FixDiagonalGamepadValues(Value, GetInputAxisValue("MoveRight/Left")).Key;
        AddMovementInput(UKismetMathLibrary::GetForwardVector(MovementFixedRotation), Scale);
    }
}

void AALSCharacterTopDown::PlayerRightMovementInput(float Value)
{
    if (MovementState == EALSMovementState::Grounded || MovementState == EALSMovementState::InAir)
    {
        // Default camera relative movement behavior
        const float Scale = UALSMathLibrary::FixDiagonalGamepadValues(GetInputAxisValue("MoveForward/Backwards"), Value).Value;
        AddMovementInput(UKismetMathLibrary::GetRightVector(MovementFixedRotation), Scale);
    }
}

void AALSCharacterTopDown::SetEssentialValues(float DeltaTime)
{
    Super::SetEssentialValues(DeltaTime);

    // Interp AimingLocation to current control rotation for smooth character rotation movement. Decrease InterpSpeed
    // for slower but smoother movement.
    AimingLocation = FMath::VInterpTo(AimingLocation, GetControlLocation(), DeltaTime, 30);
}

void AALSCharacterTopDown::UpdateAimInput(float Value)
{
    check(IsValid(Controller));
    check(Controller->IsLocalPlayerController());

    APlayerController* const PC = CastChecked<APlayerController>(Controller);

    UWorld* World = GetWorld();
    check(World);

    FVector PawnPos(GetActorLocation());

    FVector2D MouseViewportPos(UWidgetLayoutLibrary::GetMousePositionOnViewport(World));
    FVector MouseWorldPos;
    FVector MouseWorldDir;

    UGameplayStatics::DeprojectScreenToWorld(
        PC,
        MouseViewportPos,
        MouseWorldPos,
        MouseWorldDir
        );

    FVector MousePawnProjected = FMath::RayPlaneIntersection(
        MouseWorldPos,
        MouseWorldDir,
        FPlane(PawnPos, FVector(0.f, 0.f, 1.f))
        );

    FVector MousePawnDelta(MousePawnProjected-PawnPos);

    if (MousePawnDelta.SizeSquared() > 0.f)
    {
        FRotator ControlRot(MousePawnDelta.GetUnsafeNormal().ToOrientationRotator());
        PC->SetControlRotation(FRotator(0,ControlRot.Yaw,0));
    }

    ControlLocation = MousePawnProjected;
}
