// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#include "Character/ALSPlayerCameraManagerTopDown.h"


#include "Character/ALSBaseCharacter.h"
#include "Character/Animation/ALSPlayerCameraBehavior.h"
#include "Kismet/KismetMathLibrary.h"

void AALSPlayerCameraManagerTopDown::OnPossess(AALSBaseCharacter* NewCharacter)
{
    // Partially taken from base class

	// Set "Controlled Pawn" when Player Controller Possesses new character. (called from Player Controller)
	check(NewCharacter);
	ControlledCharacter = NewCharacter;

    TopDownCharacter = Cast<AALSCharacterTopDown>(NewCharacter);

	// Update references in the Camera Behavior AnimBP.
	UALSPlayerCameraBehavior* CastedBehv = Cast<UALSPlayerCameraBehavior>(CameraBehavior->GetAnimInstance());
	if (CastedBehv)
	{
		CastedBehv->PlayerController = GetOwningPlayerController();
		CastedBehv->ControlledPawn = ControlledCharacter;

		// Initial position
		const FVector& TPSLoc = ControlledCharacter->GetThirdPersonPivotTarget().GetLocation();
		SetActorLocation(TPSLoc);
		SmoothedPivotTarget.SetLocation(TPSLoc);
	}
}

void AALSPlayerCameraManagerTopDown::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
    // Partially taken from base class

    if (OutVT.Target)
    {
        FVector OutLocation;
        FRotator OutRotation;
        float OutFOV;

        if (CustomCameraBehaviorTopDown(DeltaTime, OutLocation, OutRotation, OutFOV))
        {
            OutVT.POV.Location = OutLocation;
            OutVT.POV.Rotation = OutRotation;
            OutVT.POV.FOV = OutFOV;
        }
    }
}

float AALSPlayerCameraManagerTopDown::CalculateTargetLocationInterpAxis(
    float LimitRadius,
    float Alpha,
    float Exp
    )
{
    return FMath::Lerp(
        -LimitRadius,
        LimitRadius,
        (Alpha < 0.5f)
            ? FMath::InterpEaseOut(0.f, 1.f, Alpha * 2.f, Exp) * 0.5f
            : FMath::InterpEaseIn(0.f, 1.f, Alpha * 2.f - 1.f, Exp) * 0.5f + 0.5f
            );
}

FVector AALSPlayerCameraManagerTopDown::CalculateAxisIndependentLagTopDown(
    FVector CurrentLocation,
    FVector TargetLocation,
    FRotator CameraRotation,
    FVector AnchorLocation,
    FVector LagSpeeds,
    float LimitRadius,
    float LimitExp,
    float DeltaTime
    )
{
    CameraRotation.Roll = 0.0f;
    CameraRotation.Pitch = 0.0f;
    const FVector UnrotatedCurLoc = CameraRotation.UnrotateVector(CurrentLocation);
    const FVector UnrotatedDstLoc = CameraRotation.UnrotateVector(TargetLocation);
    const FVector UnrotatedAncLoc = CameraRotation.UnrotateVector(AnchorLocation);

    const FVector CameraLimitMin(LimitRadius);
    const FVector CameraLimitMax(LimitRadius);
    const FVector AncToDst = UnrotatedDstLoc-UnrotatedAncLoc;

    FVector AncToDstDir;
    float AncToDstDist;

    AncToDst.ToDirectionAndLength(AncToDstDir, AncToDstDist);

    const float DstAlpha = FMath::Clamp(
        FMath::GetRangePct(-LimitRadius, LimitRadius, AncToDstDist),
        0.f,
        1.f
        );

    const float DstDist = FMath::Lerp(
        -LimitRadius,
        LimitRadius,
        (DstAlpha < 0.5f)
            ? FMath::InterpEaseOut(0.f, 1.f, DstAlpha * 2.f, LimitExp) * 0.5f
            : FMath::InterpEaseIn(0.f, 1.f, DstAlpha * 2.f - 1.f, LimitExp) * 0.5f + 0.5f
            );

    FVector InterpDst = UnrotatedAncLoc+AncToDstDir*DstDist;

    const FVector ResultVector(
        FMath::FInterpTo(UnrotatedCurLoc.X, InterpDst.X, DeltaTime, LagSpeeds.X),
        FMath::FInterpTo(UnrotatedCurLoc.Y, InterpDst.Y, DeltaTime, LagSpeeds.Y),
        FMath::FInterpTo(UnrotatedCurLoc.Z, InterpDst.Z, DeltaTime, LagSpeeds.Z)
        );

    return CameraRotation.RotateVector(ResultVector);
}

bool AALSPlayerCameraManagerTopDown::CustomCameraBehaviorTopDown(float DeltaTime, FVector& Location, FRotator& Rotation, float& FOV)
{
    // Partially taken from base class

    if (! IsValid(TopDownCharacter))
    {
        return false;
    }

    // Step 1: Get Camera Parameters from CharacterBP via the Camera Interface
    const FTransform PivotTarget(TopDownCharacter->GetThirdPersonPivotTarget());
    const FVector& FPTarget = TopDownCharacter->GetFirstPersonCameraTarget();
    float TPFOV = 90.0f;
    float FPFOV = 90.0f;
    bool bRightShoulder = false;
    TopDownCharacter->GetCameraParameters(TPFOV, FPFOV, bRightShoulder);

    // Step 2: Set fixed top down camera rotation
    TargetCameraRotation = FRotator(FixedCameraPitch, FixedCameraYaw, 0.f);

    // Step 3: Calculate the Smoothed Pivot Target (Orange Sphere).
    // Get the 3P Pivot Target (Green Sphere) and interpolate using axis independent lag for maximum control.
    const FVector LagSpd(GetCameraBehaviorParam(FName(TEXT("PivotLagSpeed_X"))),
                         GetCameraBehaviorParam(FName(TEXT("PivotLagSpeed_Y"))),
                         GetCameraBehaviorParam(FName(TEXT("PivotLagSpeed_Z"))));

    const FVector AxisIndpLag(
        CalculateAxisIndependentLagTopDown(
            SmoothedPivotTarget.GetLocation(),
            PivotTarget.GetLocation(),
            TargetCameraRotation,
            TopDownCharacter->GetThirdPersonPivotAnchorLocation(),
            LagSpd,
            CameraPivotRadius,
            CameraPivotExp,
            DeltaTime
            ) );

    SmoothedPivotTarget.SetRotation(PivotTarget.GetRotation());
    SmoothedPivotTarget.SetLocation(AxisIndpLag);
    SmoothedPivotTarget.SetScale3D(FVector::OneVector);

    // Step 4: Calculate Pivot Location (BlueSphere). Get the Smoothed
    // Pivot Target and apply local offsets for further camera control.
    PivotLocation =
        SmoothedPivotTarget.GetLocation() +
        UKismetMathLibrary::GetForwardVector(SmoothedPivotTarget.Rotator()) * GetCameraBehaviorParam(
            FName(TEXT("PivotOffset_X"))) +
        UKismetMathLibrary::GetRightVector(SmoothedPivotTarget.Rotator()) * GetCameraBehaviorParam(
            FName(TEXT("PivotOffset_Y"))) +
        UKismetMathLibrary::GetUpVector(SmoothedPivotTarget.Rotator()) * GetCameraBehaviorParam(
            FName(TEXT("PivotOffset_Z")));

    // Step 5: Calculate Target Camera Location. Get the Pivot location and apply camera relative offsets.
    TargetCameraLocation = UKismetMathLibrary::VLerp(
        PivotLocation +
        UKismetMathLibrary::GetForwardVector(TargetCameraRotation) * GetCameraBehaviorParam(
            FName(TEXT("CameraOffset_X"))) +
        UKismetMathLibrary::GetRightVector(TargetCameraRotation) * GetCameraBehaviorParam(FName(TEXT("CameraOffset_Y")))
        +
        UKismetMathLibrary::GetUpVector(TargetCameraRotation) * GetCameraBehaviorParam(FName(TEXT("CameraOffset_Z"))),
        PivotTarget.GetLocation() + DebugViewOffset,
        GetCameraBehaviorParam(FName(TEXT("Override_Debug"))));

    // Step 6: Trace for an object between the camera and character to apply a corrective offset.
    // Trace origins are set within the Character BP via the Camera Interface.
    // Functions like the normal spring arm, but can allow for different trace origins regardless of the pivot
    //FVector TraceOrigin;
    //float TraceRadius;
    //ECollisionChannel TraceChannel = TopDownCharacter->GetThirdPersonTraceParams(TraceOrigin, TraceRadius);

    //UWorld* World = GetWorld();
    //check(World);

    //FCollisionQueryParams Params;
    //Params.AddIgnoredActor(this);
    //Params.AddIgnoredActor(TopDownCharacter);

    //FHitResult HitResult;
    //World->SweepSingleByChannel(HitResult, TraceOrigin, TargetCameraLocation, FQuat::Identity,
    //                            TraceChannel, FCollisionShape::MakeSphere(TraceRadius), Params);

    //if (HitResult.IsValidBlockingHit())
    //{
    //    TargetCameraLocation += (HitResult.Location - HitResult.TraceEnd);
    //}

    // Step 7: Draw Debug Shapes.
    DrawDebugTargets(PivotTarget.GetLocation());

    // Step 8: Lerp First Person Override and return target camera parameters.
    FTransform TargetCameraTransform(TargetCameraRotation, TargetCameraLocation, FVector::OneVector);
    FTransform FPTargetCameraTransform(TargetCameraRotation, FPTarget, FVector::OneVector);

    const FTransform& MixedTransform = UKismetMathLibrary::TLerp(TargetCameraTransform, FPTargetCameraTransform,
                                                                 GetCameraBehaviorParam(
                                                                     FName(TEXT("Weight_FirstPerson"))));

    const FTransform& TargetTransform = UKismetMathLibrary::TLerp(MixedTransform,
                                                                  FTransform(DebugViewRotation, TargetCameraLocation,
                                                                             FVector::OneVector),
                                                                  GetCameraBehaviorParam(
                                                                      FName(TEXT("Override_Debug"))));

    Location = TargetTransform.GetLocation();
    Rotation = TargetTransform.Rotator();
    FOV = FMath::Lerp(TPFOV, FPFOV, GetCameraBehaviorParam(FName(TEXT("Weight_FirstPerson"))));

    return true;
}
