// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    


#include "Character/ALSCharacter.h"

#include "Engine/StaticMesh.h"
#include "AI/ALSAIController.h"
#include "Kismet/GameplayStatics.h"

#include "Weapon/ALSWeapon.h"

FOnALSCharacterEquipWeapon AALSCharacter::NotifyEquipWeapon;
FOnALSCharacterUnEquipWeapon AALSCharacter::NotifyUnEquipWeapon;

AALSCharacter::AALSCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    HeldObjectRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HeldObjectRoot"));
    HeldObjectRoot->SetupAttachment(GetMesh());

    SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
    SkeletalMesh->SetupAttachment(HeldObjectRoot);

    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    StaticMesh->SetupAttachment(HeldObjectRoot);

    AIControllerClass = AALSAIController::StaticClass();

    bWantsToFire = false;
}

void AALSCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (GetLocalRole() == ROLE_Authority)
    {
        // Needs to happen after character is added to repgraph
        GetWorldTimerManager().SetTimerForNextTick(this, &AALSCharacter::SpawnDefaultInventory);
    }
}

void AALSCharacter::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // only to local owner: weapon change requests are locally instigated,
    // other clients don't need it
    DOREPLIFETIME_CONDITION(AALSCharacter, Inventory, COND_OwnerOnly);

    // Everyone
    DOREPLIFETIME(AALSCharacter, CurrentWeapon);
}

void AALSCharacter::ClearHeldObject()
{
    StaticMesh->SetStaticMesh(nullptr);
    SkeletalMesh->SetSkeletalMesh(nullptr);
    SkeletalMesh->SetAnimInstanceClass(nullptr);
}

void AALSCharacter::AttachToHand(
    UStaticMesh* NewStaticMesh,
    USkeletalMesh* NewSkeletalMesh,
    UClass* NewAnimClass,
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
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        AttachBone
        );

    HeldObjectRoot->SetRelativeLocation(Offset);
}

void AALSCharacter::AttachComponentToHand(USceneComponent* NewComponent, bool bLeftHand)
{
    ClearHeldObject();

    // Invalid actor to attach, abort
    if (! IsValid(NewComponent))
    {
        return;
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

    // Attach actor to held object root
    NewComponent->AttachToComponent(
        HeldObjectRoot,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale
        );

    // Attach held object root to skeletal mesh attach bone
    HeldObjectRoot->AttachToComponent(
        GetMesh(),
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        AttachBone
        );
}

void AALSCharacter::RagdollStart()
{
    ClearHeldObject();
    Super::RagdollStart();
}

void AALSCharacter::RagdollEnd()
{
    Super::RagdollEnd();
    UpdateHeldObject();
}

ECollisionChannel AALSCharacter::GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius)
{
    const FName CameraSocketName = bRightShoulder ? TEXT("TP_CameraTrace_R") : TEXT("TP_CameraTrace_L");
    TraceOrigin = GetMesh()->GetSocketLocation(CameraSocketName);
    TraceRadius = 15.0f;
    return ECC_Camera;
}

FTransform AALSCharacter::GetThirdPersonPivotTarget()
{
    return FTransform(GetActorRotation(),
                      (GetMesh()->GetSocketLocation(TEXT("Head")) + GetMesh()->GetSocketLocation(TEXT("root"))) / 2.0f,
                      FVector::OneVector);
}

FVector AALSCharacter::GetFirstPersonCameraTarget()
{
    return GetMesh()->GetSocketLocation(TEXT("FP_Camera"));
}

void AALSCharacter::OnOverlayStateChanged(EALSOverlayState PreviousState)
{
    Super::OnOverlayStateChanged(PreviousState);
    UpdateHeldObject();

    switch (OverlayState)
    {
        case EALSOverlayState::Rifle:
            if (Inventory.IsValidIndex(0))
            {
                EquipWeapon(Inventory[0]);
            }
            break;

        default:
            EquipWeapon(nullptr);
            break;
    }
}

void AALSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHeldObjectAnimations();
}

void AALSCharacter::BeginPlay()
{
    Super::BeginPlay();
    UpdateHeldObject();
}

void AALSCharacter::Destroyed()
{
    Super::Destroyed();
    DestroyInventory();
}

void AALSCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();

    // reattach weapon if needed
    SetCurrentWeapon(CurrentWeapon);
}

void AALSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AALSCharacter::OnStartFire);
    PlayerInputComponent->BindAction("Fire", IE_Released, this, &AALSCharacter::OnStopFire);

    PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &AALSCharacter::OnReload);
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void AALSCharacter::SpawnDefaultInventory()
{
    if (GetLocalRole() < ROLE_Authority)
    {
        return;
    }

    int32 NumWeaponClasses = DefaultInventoryClasses.Num();
    for (int32 i = 0; i < NumWeaponClasses; i++)
    {
        if (DefaultInventoryClasses[i])
        {
            FActorSpawnParameters SpawnInfo;
            SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AALSWeapon* NewWeapon = GetWorld()->SpawnActor<AALSWeapon>(DefaultInventoryClasses[i], SpawnInfo);
            AddWeapon(NewWeapon);
        }
    }

    // equip first weapon in inventory
    //if (Inventory.Num() > 0)
    //{
    //    EquipWeapon(Inventory[0]);
    //}
}

void AALSCharacter::DestroyInventory()
{
    if (GetLocalRole() < ROLE_Authority)
    {
        return;
    }

    // remove all weapons from inventory and destroy them
    for (int32 i = Inventory.Num() - 1; i >= 0; i--)
    {
        AALSWeapon* Weapon = Inventory[i];
        if (Weapon)
        {
            RemoveWeapon(Weapon);
            Weapon->Destroy();
        }
    }
}

void AALSCharacter::AddWeapon(AALSWeapon* Weapon)
{
    if (Weapon && GetLocalRole() == ROLE_Authority)
    {
        Weapon->OnEnterInventory(this);
        Inventory.AddUnique(Weapon);
    }
}

void AALSCharacter::RemoveWeapon(AALSWeapon* Weapon)
{
    if (Weapon && GetLocalRole() == ROLE_Authority)
    {
        Weapon->OnLeaveInventory();
        Inventory.RemoveSingle(Weapon);
    }
}

void AALSCharacter::EquipWeapon(AALSWeapon* Weapon)
{
    //if (Weapon)
    //{
    //    if (GetLocalRole() == ROLE_Authority)
    //    {
    //        SetCurrentWeapon(Weapon, CurrentWeapon);
    //    }
    //    else
    //    {
    //        ServerEquipWeapon(Weapon);
    //    }
    //}

    if (Weapon != CurrentWeapon)
    {
        if (GetLocalRole() == ROLE_Authority)
        {
            SetCurrentWeapon(Weapon, CurrentWeapon);
        }
        else
        {
            ServerEquipWeapon(Weapon);
        }
    }
}

bool AALSCharacter::ServerEquipWeapon_Validate(AALSWeapon* Weapon)
{
    return true;
}

void AALSCharacter::ServerEquipWeapon_Implementation(AALSWeapon* Weapon)
{
    EquipWeapon(Weapon);
}

void AALSCharacter::OnRep_CurrentWeapon(AALSWeapon* LastWeapon)
{
    SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AALSCharacter::SetCurrentWeapon(AALSWeapon* NewWeapon, AALSWeapon* LastWeapon)
{
    AALSWeapon* LocalLastWeapon = nullptr;

    if (LastWeapon)
    {
        LocalLastWeapon = LastWeapon;
    }
    else
    if (NewWeapon != CurrentWeapon)
    {
        LocalLastWeapon = CurrentWeapon;
    }

    // unequip previous
    if (LocalLastWeapon)
    {
        LocalLastWeapon->OnUnEquip();
    }

    CurrentWeapon = NewWeapon;

    // equip new one
    if (NewWeapon)
    {
        // Make sure weapon's MyPawn is pointing back to us.
        // During replication, we can't guarantee APawn::CurrentWeapon
        // will rep after AWeapon::MyPawn!
        NewWeapon->SetOwningPawn(this);

        NewWeapon->OnEquip(LastWeapon);
    }
}

//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AALSCharacter::StartWeaponFire()
{
    if (! bWantsToFire)
    {
        bWantsToFire = true;
        if (CurrentWeapon)
        {
            CurrentWeapon->StartFire();
        }
    }
}

void AALSCharacter::StopWeaponFire()
{
    if (bWantsToFire)
    {
        bWantsToFire = false;
        if (CurrentWeapon)
        {
            CurrentWeapon->StopFire();
        }
    }
}

bool AALSCharacter::CanFire() const
{
    return true; //IsAlive();
}

bool AALSCharacter::CanReload() const
{
    return true;
}

void AALSCharacter::OnStartFire()
{
    //AALSPlayerController* MyPC = Cast<AALSPlayerController>(Controller);
    //if (MyPC && MyPC->IsGameInputAllowed())
    //{
    //    if (IsRunning())
    //    {
    //        SetRunning(false, false);
    //    }
    //    StartWeaponFire();
    //}
    if (GetRotationMode() == EALSRotationMode::Aiming)
    {
        StartWeaponFire();
    }
}

void AALSCharacter::OnStopFire()
{
    StopWeaponFire();
}

void AALSCharacter::OnReload()
{
    //AALSPlayerController* MyPC = Cast<AALSPlayerController>(Controller);
    //if (MyPC && MyPC->IsGameInputAllowed())
    //{
    //    if (CurrentWeapon)
    //    {
    //        CurrentWeapon->StartReload();
    //    }
    //}
    if (CurrentWeapon)
    {
        CurrentWeapon->StartReload();
    }
}
