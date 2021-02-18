// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2020 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    Nuraga Wiswakarma


#include "Character/ALSMCharacter.h"
#include "Engine/StaticMesh.h"
#include "Character/AI/ALSAIController.h"

AALSMCharacter::AALSMCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	HeldObjectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HeldObjectRoot"));
	HeldObjectRoot->SetupAttachment(GetMesh());

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(HeldObjectRoot);

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(HeldObjectRoot);

	//AIControllerClass = AALSAIController::StaticClass();
}

void AALSMCharacter::ClearHeldObject()
{
    StaticMesh->SetStaticMesh(nullptr);
    SkeletalMesh->SetSkeletalMesh(nullptr);
    SkeletalMesh->SetAnimInstanceClass(nullptr);
}

void AALSMCharacter::AttachToHand(
    UStaticMesh* NewStaticMesh,
    USkeletalMesh* NewSkeletalMesh,
    class UClass* NewAnimClass,
    bool bLeftHand,
    FVector Offset
    )
{
    ClearHeldObject();
    
    if (IsValid(NewStaticMesh))
    {
        StaticMesh->SetStaticMesh(NewStaticMesh);
    }
    else
    if (IsValid(NewSkeletalMesh))
    {
        SkeletalMesh->SetSkeletalMesh(NewSkeletalMesh);

        if (IsValid(NewAnimClass))
        {
            SkeletalMesh->SetAnimInstanceClass(NewAnimClass);
        }
    }
    
    FName AttachBone;

    if (bLeftHand)
    {
        AttachBone = TEXT("VB LHS_ik_hand_gun");
    }
    else
    {
        AttachBone = TEXT("VB RHS_ik_hand_gun");
    }
    
    HeldObjectRoot->AttachToComponent(
        GetMesh(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachBone
        );

    HeldObjectRoot->SetRelativeLocation(Offset);
}

//void AALSMCharacter::RagdollStart()
//{
//    ClearHeldObject();
//    Super::RagdollStart();
//}

//void AALSMCharacter::RagdollEnd()
//{
//    Super::RagdollEnd();
//    UpdateHeldObject();
//}

ECollisionChannel AALSMCharacter::GetThirdPersonTraceParams(
    FVector& TraceOrigin,
    float& TraceRadius
    )
{
	const FName CameraSocketName = bRightShoulder ? TEXT("TP_CameraTrace_R") : TEXT("TP_CameraTrace_L");
	TraceOrigin = GetMesh()->GetSocketLocation(CameraSocketName);
	TraceRadius = 15.0f;
	return ECC_Camera;
}

FTransform AALSMCharacter::GetThirdPersonPivotTarget()
{
    return FTransform(
        GetActorRotation(),
        (GetMesh()->GetSocketLocation(TEXT("Head")) + GetMesh()->GetSocketLocation(TEXT("root"))) / 2.0f,
        FVector::OneVector
        );
}

FVector AALSMCharacter::GetFirstPersonCameraTarget()
{
	return GetMesh()->GetSocketLocation(TEXT("FP_Camera"));
}

void AALSMCharacter::OnOverlayStateChanged(EALSOverlayState PreviousState)
{
    Super::OnOverlayStateChanged(PreviousState);
    UpdateHeldObject();
}

void AALSMCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateHeldObjectAnimations();
}

void AALSMCharacter::BeginPlay()
{
	Super::BeginPlay();
	UpdateHeldObject();
}

//void AALSMCharacter::MantleStart(
//    float MantleHeight,
//    const FALSComponentAndTransform& MantleLedgeWS,
//    EALSMantleType MantleType
//    )
//{
//    Super::MantleStart(MantleHeight, MantleLedgeWS, MantleType);
//    if (MantleType != EALSMantleType::LowMantle)
//    {
//        // If we're not doing low mantle, clear held object
//        ClearHeldObject();
//    }
//}

//void AALSMCharacter::MantleEnd()
//{
//    Super::MantleEnd();
//    UpdateHeldObject();
//}
