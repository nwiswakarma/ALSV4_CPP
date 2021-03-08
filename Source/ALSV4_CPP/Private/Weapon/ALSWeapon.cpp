// 

#include "Weapon/ALSWeapon.h"
#include "ALSDefines.h"
#include "Character/ALSCharacter.h"
#include "Particles/ParticleSystemComponent.h"
//#include "Bots/ShooterAIController.h"
//#include "Online/ShooterPlayerState.h"
//#include "UI/ShooterHUD.h"
//#include "Camera/CameraShake.h"
#include "Kismet/KismetMathLibrary.h"

#include "Kismet/KismetSystemLibrary.h"

AALSWeapon::AALSWeapon(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    RootComponent = WeaponRoot;

    Mesh1P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh1P"));
    Mesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    Mesh1P->bReceivesDecals = false;
    Mesh1P->CastShadow = false;
    Mesh1P->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh1P->SetCollisionResponseToAllChannels(ECR_Ignore);
    //RootComponent = Mesh1P;
    Mesh1P->SetupAttachment(RootComponent);

    Mesh3P = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("WeaponMesh3P"));
    Mesh3P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
    Mesh3P->bReceivesDecals = false;
    Mesh3P->CastShadow = true;
    Mesh3P->SetCollisionObjectType(ECC_WorldDynamic);
    Mesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh3P->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh3P->SetCollisionResponseToChannel(ECC_ALIAS_WEAPON, ECR_Block);
    Mesh3P->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Mesh3P->SetCollisionResponseToChannel(ECC_ALIAS_PROJECTILE, ECR_Block);
    //Mesh3P->SetupAttachment(Mesh1P);
    Mesh3P->SetupAttachment(RootComponent);

    bLoopedMuzzleFX = false;
    bLoopedFireAnim = false;
    bPlayingFireAnim = false;
    bIsEquipped = false;
    bWantsToFire = false;
    bPendingReload = false;
    bPendingEquip = false;
    CurrentState = EWeaponState::Idle;

    CurrentAmmo = 0;
    CurrentAmmoInClip = 0;
    BurstCounter = 0;
    LastFireTime = 0.0f;

    MyPawnPC = nullptr;
    MyPawnAC = nullptr;
    bIsPlayerControlled = false;
    bIsAIControlled = false;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
    bReplicates = true;
    bNetUseOwnerRelevancy = true;
}

void AALSWeapon::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    if (WeaponConfig.InitialClips > 0)
    {
        CurrentAmmoInClip = WeaponConfig.AmmoPerClip;
        CurrentAmmo = WeaponConfig.AmmoPerClip * WeaponConfig.InitialClips;
    }

    DetachMeshFromPawn();
}

void AALSWeapon::Destroyed()
{
    Super::Destroyed();

    StopSimulatingWeaponFire();
}

//////////////////////////////////////////////////////////////////////////
// Inventory

void AALSWeapon::OnEquip(const AALSWeapon* LastWeapon)
{
    AttachMeshToPawn();

    bPendingEquip = true;
    DetermineWeaponState();

    // Only play animation if last weapon is valid
    if (LastWeapon)
    {
        float Duration = PlayWeaponAnimation(EquipAnim);
        if (Duration <= 0.0f)
        {
            // failsafe
            Duration = 0.5f;
        }
        EquipStartedTime = GetWorld()->GetTimeSeconds();
        EquipDuration = Duration;

        GetWorldTimerManager().SetTimer(TimerHandle_OnEquipFinished, this, &AALSWeapon::OnEquipFinished, Duration, false);
    }
    else
    {
        OnEquipFinished();
    }

    if (MyPawn && MyPawn->IsLocallyControlled())
    {
        PlayWeaponSound(EquipSound);
    }

    AALSCharacter::NotifyEquipWeapon.Broadcast(MyPawn, this);
}

void AALSWeapon::OnEquipFinished()
{
    AttachMeshToPawn();

    bIsEquipped = true;
    bPendingEquip = false;

    // Determine the state so that the can reload checks will work
    DetermineWeaponState(); 
    
    if (MyPawn)
    {
        // try to reload empty clip
        if (MyPawn->IsLocallyControlled() &&
            CurrentAmmoInClip <= 0 &&
            CanReload())
        {
            StartReload();
        }
    }

    
}

void AALSWeapon::OnUnEquip()
{
    DetachMeshFromPawn();
    bIsEquipped = false;
    StopFire();

    if (bPendingReload)
    {
        StopWeaponAnimation(ReloadAnim);
        bPendingReload = false;

        GetWorldTimerManager().ClearTimer(TimerHandle_StopReload);
        GetWorldTimerManager().ClearTimer(TimerHandle_ReloadWeapon);
    }

    if (bPendingEquip)
    {
        StopWeaponAnimation(EquipAnim);
        bPendingEquip = false;

        GetWorldTimerManager().ClearTimer(TimerHandle_OnEquipFinished);
    }

    AALSCharacter::NotifyUnEquipWeapon.Broadcast(MyPawn, this);

    DetermineWeaponState();
}

void AALSWeapon::OnEnterInventory(AALSCharacter* NewOwner)
{
    SetOwningPawn(NewOwner);
}

void AALSWeapon::OnLeaveInventory()
{
    if (IsAttachedToPawn())
    {
        OnUnEquip();
    }

    if (GetLocalRole() == ROLE_Authority)
    {
        SetOwningPawn(nullptr);
    }
}

void AALSWeapon::AttachMeshToPawn()
{
    //if (MyPawn)
    //{
    //    // Remove and hide both first and third person meshes
    //    DetachMeshFromPawn();

    //    // For locally controller players we attach both weapons
    //    // and let the bOnlyOwnerSee, bOwnerNoSee flags deal with visibility.
    //    FName AttachPoint = MyPawn->GetWeaponAttachPoint();
    //    if( MyPawn->IsLocallyControlled() == true )
    //    {
    //        USkeletalMeshComponent* PawnMesh1p = MyPawn->GetSpecifcPawnMesh(true);
    //        USkeletalMeshComponent* PawnMesh3p = MyPawn->GetSpecifcPawnMesh(false);
    //        Mesh1P->SetHiddenInGame( false );
    //        Mesh3P->SetHiddenInGame( false );
    //        Mesh1P->AttachToComponent(PawnMesh1p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
    //        Mesh3P->AttachToComponent(PawnMesh3p, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
    //    }
    //    else
    //    {
    //        USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
    //        USkeletalMeshComponent* UsePawnMesh = MyPawn->GetPawnMesh();
    //        UseWeaponMesh->AttachToComponent(UsePawnMesh, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
    //        UseWeaponMesh->SetHiddenInGame( false );
    //    }
    //}

    if (MyPawn)
    {
        // Remove and hide both first and third person meshes
        DetachMeshFromPawn();

        // Attach weapon root to pawn hand bone
        MyPawn->AttachComponentToHand(WeaponRoot, false);

        // Set mesh visibility
        Mesh3P->SetHiddenInGame(false);
    }
}

void AALSWeapon::DetachMeshFromPawn()
{
    // Detach weapon root from parent
    WeaponRoot->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);

    // Hide mesh visibility
    Mesh1P->SetHiddenInGame(true);
    Mesh3P->SetHiddenInGame(true);
}


//////////////////////////////////////////////////////////////////////////
// Input

void AALSWeapon::StartFire()
{
    if (GetLocalRole() < ROLE_Authority)
    {
        ServerStartFire();
    }

    if (!bWantsToFire)
    {
        bWantsToFire = true;
        DetermineWeaponState();
    }
}

void AALSWeapon::StopFire()
{
    if ((GetLocalRole() < ROLE_Authority) && MyPawn && MyPawn->IsLocallyControlled())
    {
        ServerStopFire();
    }

    if (bWantsToFire)
    {
        bWantsToFire = false;
        DetermineWeaponState();
    }
}

void AALSWeapon::StartReload(bool bFromReplication)
{
    if (!bFromReplication && GetLocalRole() < ROLE_Authority)
    {
        ServerStartReload();
    }

    if (bFromReplication || CanReload())
    {
        bPendingReload = true;
        DetermineWeaponState();

        float AnimDuration = PlayWeaponAnimation(ReloadAnim);       
        if (AnimDuration <= 0.0f)
        {
            AnimDuration = WeaponConfig.NoAnimReloadDuration;
        }

        GetWorldTimerManager().SetTimer(TimerHandle_StopReload, this, &AALSWeapon::StopReload, AnimDuration, false);
        if (GetLocalRole() == ROLE_Authority)
        {
            GetWorldTimerManager().SetTimer(TimerHandle_ReloadWeapon, this, &AALSWeapon::ReloadWeapon, FMath::Max(0.1f, AnimDuration - 0.1f), false);
        }
        
        if (MyPawn && MyPawn->IsLocallyControlled())
        {
            PlayWeaponSound(ReloadSound);
        }
    }
}

void AALSWeapon::StopReload()
{
    if (CurrentState == EWeaponState::Reloading)
    {
        bPendingReload = false;
        DetermineWeaponState();
        StopWeaponAnimation(ReloadAnim);
    }
}

bool AALSWeapon::ServerStartFire_Validate()
{
    return true;
}

void AALSWeapon::ServerStartFire_Implementation()
{
    StartFire();
}

bool AALSWeapon::ServerStopFire_Validate()
{
    return true;
}

void AALSWeapon::ServerStopFire_Implementation()
{
    StopFire();
}

bool AALSWeapon::ServerStartReload_Validate()
{
    return true;
}

void AALSWeapon::ServerStartReload_Implementation()
{
    StartReload();
}

bool AALSWeapon::ServerStopReload_Validate()
{
    return true;
}

void AALSWeapon::ServerStopReload_Implementation()
{
    StopReload();
}

void AALSWeapon::ClientStartReload_Implementation()
{
    StartReload();
}

//////////////////////////////////////////////////////////////////////////
// Control

bool AALSWeapon::CanFire() const
{
    bool bCanFire = MyPawn && MyPawn->CanFire();
    bool bStateOKToFire = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );   
    return (( bCanFire == true ) && ( bStateOKToFire == true ) && ( bPendingReload == false ));
}

bool AALSWeapon::CanReload() const
{
    bool bCanReload = (!MyPawn || MyPawn->CanReload());
    bool bGotAmmo = ( CurrentAmmoInClip < WeaponConfig.AmmoPerClip) && (CurrentAmmo - CurrentAmmoInClip > 0 || HasInfiniteClip());
    bool bStateOKToReload = ( ( CurrentState ==  EWeaponState::Idle ) || ( CurrentState == EWeaponState::Firing) );
    return ( ( bCanReload == true ) && ( bGotAmmo == true ) && ( bStateOKToReload == true) );   
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage

void AALSWeapon::GiveAmmo(int AddAmount)
{
    const int32 MissingAmmo = FMath::Max(0, WeaponConfig.MaxAmmo - CurrentAmmo);
    AddAmount = FMath::Min(AddAmount, MissingAmmo);
    CurrentAmmo += AddAmount;

    //AALSAIController* BotAI = MyPawn ? Cast<AALSAIController>(MyPawn->GetController()) : NULL;
    //if (BotAI)
    //{
    //    BotAI->CheckAmmo(this);
    //}
    
    // start reload if clip was empty
    if (GetCurrentAmmoInClip() <= 0 &&
        CanReload() &&
        MyPawn && (MyPawn->GetWeapon() == this))
    {
        ClientStartReload();
    }
}

void AALSWeapon::UseAmmo()
{
    if (!HasInfiniteAmmo())
    {
        CurrentAmmoInClip--;
    }

    if (!HasInfiniteAmmo() && !HasInfiniteClip())
    {
        CurrentAmmo--;
    }

    //AALSAIController* BotAI = MyPawn ? Cast<AALSAIController>(MyPawn->GetController()) : NULL;  
    //AALSPlayerController* PlayerController = MyPawn ? Cast<AALSPlayerController>(MyPawn->GetController()) : NULL;
    //if (BotAI)
    //{
    //    BotAI->CheckAmmo(this);
    //}
    //else
    //if(PlayerController)
    //{
    //    AALSPlayerState* PlayerState = Cast<AALSPlayerState>(PlayerController->PlayerState);
    //    switch (GetAmmoType())
    //    {
    //        case EAmmoType::ERocket:
    //            PlayerState->AddRocketsFired(1);
    //            break;
    //        case EAmmoType::EBullet:
    //        default:
    //            PlayerState->AddBulletsFired(1);
    //            break;          
    //    }
    //}
}

void AALSWeapon::HandleReFiring()
{
    // Update TimerIntervalAdjustment
    UWorld* MyWorld = GetWorld();

    float SlackTimeThisFrame = FMath::Max(0.0f, (MyWorld->TimeSeconds - LastFireTime) - WeaponConfig.TimeBetweenShots);

    if (bAllowAutomaticWeaponCatchup)
    {
        TimerIntervalAdjustment -= SlackTimeThisFrame;
    }

    HandleFiring();
}

void AALSWeapon::HandleFiring()
{
    if ((CurrentAmmoInClip > 0 || HasInfiniteClip() || HasInfiniteAmmo()) && CanFire())
    {
        if (GetNetMode() != NM_DedicatedServer)
        {
            SimulateWeaponFire();
        }

        if (MyPawn && MyPawn->IsLocallyControlled())
        {
            FireWeapon();

            UseAmmo();
            
            // update firing FX on remote clients if function was called on server
            BurstCounter++;
        }
    }
    else
    if (CanReload())
    {
        StartReload();
    }
    else
    if (MyPawn && MyPawn->IsLocallyControlled())
    {
        if (GetCurrentAmmo() == 0 && !bRefiring)
        {
            PlayWeaponSound(OutOfAmmoSound);
            //AALSPlayerController* MyPC = Cast<AALSPlayerController>(MyPawn->Controller);
            //AALSHUD* MyHUD = MyPC ? Cast<AALSHUD>(MyPC->GetHUD()) : NULL;
            //if (MyHUD)
            //{
            //    MyHUD->NotifyOutOfAmmo();
            //}
        }
        
        // stop weapon fire FX, but stay in Firing state
        if (BurstCounter > 0)
        {
            OnBurstFinished();
        }
    }
    else
    {
        OnBurstFinished();
    }

    if (MyPawn && MyPawn->IsLocallyControlled())
    {
        // local client will notify server
        if (GetLocalRole() < ROLE_Authority)
        {
            ServerHandleFiring();
        }

        // reload after firing last round
        if (CurrentAmmoInClip <= 0 && CanReload())
        {
            StartReload();
        }

        // setup refire timer
        bRefiring = (CurrentState == EWeaponState::Firing && WeaponConfig.TimeBetweenShots > 0.0f);
        if (bRefiring)
        {
            GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AALSWeapon::HandleReFiring, FMath::Max<float>(WeaponConfig.TimeBetweenShots + TimerIntervalAdjustment, SMALL_NUMBER), false);
            TimerIntervalAdjustment = 0.f;
        }
    }

    LastFireTime = GetWorld()->GetTimeSeconds();
}

bool AALSWeapon::ServerHandleFiring_Validate()
{
    return true;
}

void AALSWeapon::ServerHandleFiring_Implementation()
{
    const bool bShouldUpdateAmmo = (CurrentAmmoInClip > 0 && CanFire());

    HandleFiring();

    if (bShouldUpdateAmmo)
    {
        // update ammo
        UseAmmo();

        // update firing FX on remote clients
        BurstCounter++;
    }
}

void AALSWeapon::ReloadWeapon()
{
    int32 ClipDelta = FMath::Min(WeaponConfig.AmmoPerClip - CurrentAmmoInClip, CurrentAmmo - CurrentAmmoInClip);

    if (HasInfiniteClip())
    {
        ClipDelta = WeaponConfig.AmmoPerClip - CurrentAmmoInClip;
    }

    if (ClipDelta > 0)
    {
        CurrentAmmoInClip += ClipDelta;
    }

    if (HasInfiniteClip())
    {
        CurrentAmmo = FMath::Max(CurrentAmmoInClip, CurrentAmmo);
    }
}

void AALSWeapon::SetWeaponState(EWeaponState::Type NewState)
{
    const EWeaponState::Type PrevState = CurrentState;

    if (PrevState == EWeaponState::Firing && NewState != EWeaponState::Firing)
    {
        OnBurstFinished();
    }

    CurrentState = NewState;

    if (PrevState != EWeaponState::Firing && NewState == EWeaponState::Firing)
    {
        OnBurstStarted();
    }
}

void AALSWeapon::DetermineWeaponState()
{
    EWeaponState::Type NewState = EWeaponState::Idle;

    if (bIsEquipped)
    {
        if( bPendingReload  )
        {
            if( CanReload() == false )
            {
                NewState = CurrentState;
            }
            else
            {
                NewState = EWeaponState::Reloading;
            }
        }       
        else if ( (bPendingReload == false ) && ( bWantsToFire == true ) && ( CanFire() == true ))
        {
            NewState = EWeaponState::Firing;
        }
    }
    else if (bPendingEquip)
    {
        NewState = EWeaponState::Equipping;
    }

    SetWeaponState(NewState);
}

void AALSWeapon::OnBurstStarted()
{
    // start firing, can be delayed to satisfy TimeBetweenShots
    const float GameTime = GetWorld()->GetTimeSeconds();
    if (LastFireTime > 0 && WeaponConfig.TimeBetweenShots > 0.0f &&
        LastFireTime + WeaponConfig.TimeBetweenShots > GameTime)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_HandleFiring, this, &AALSWeapon::HandleFiring, LastFireTime + WeaponConfig.TimeBetweenShots - GameTime, false);
    }
    else
    {
        HandleFiring();
    }
}

void AALSWeapon::OnBurstFinished()
{
    // stop firing FX on remote clients
    BurstCounter = 0;

    // stop firing FX locally, unless it's a dedicated server
    if (GetNetMode() != NM_DedicatedServer)
    {
        StopSimulatingWeaponFire();
    }
    
    GetWorldTimerManager().ClearTimer(TimerHandle_HandleFiring);
    bRefiring = false;

    // reset firing interval adjustment
    TimerIntervalAdjustment = 0.0f;
}


//////////////////////////////////////////////////////////////////////////
// Weapon usage helpers

UAudioComponent* AALSWeapon::PlayWeaponSound(USoundCue* Sound)
{
    UAudioComponent* AC = NULL;
    if (Sound && MyPawn)
    {
        //AC = UGameplayStatics::SpawnSoundAttached(Sound, MyPawn->GetRootComponent());
    }

    return AC;
}

float AALSWeapon::PlayWeaponAnimation(const FWeaponAnim& Animation)
{
    float Duration = 0.0f;
    //if (MyPawn)
    //{
    //    UAnimMontage* UseAnim = MyPawn->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
    //    if (UseAnim)
    //    {
    //        Duration = MyPawn->PlayAnimMontage(UseAnim);
    //    }
    //}
    if (IsValid(Animation.PawnAnim) && IsValid(MyPawn))
    {
        Duration = MyPawn->Replicated_PlayMontage(
            Animation.PawnAnim,
            Animation.PawnAnimPlayRate
            );
    }
    return Duration;
}

void AALSWeapon::StopWeaponAnimation(const FWeaponAnim& Animation)
{
    //if (MyPawn)
    //{
    //    UAnimMontage* UseAnim = MyPawn->IsFirstPerson() ? Animation.Pawn1P : Animation.Pawn3P;
    //    if (UseAnim)
    //    {
    //        MyPawn->StopAnimMontage(UseAnim);
    //    }
    //}
    if (IsValid(Animation.PawnAnim) && IsValid(MyPawn))
    {
        MyPawn->Replicated_StopMontage(0.f, Animation.PawnAnim);
    }
}

FVector AALSWeapon::GetCameraAim() const
{
    FVector FinalAim = FVector::ZeroVector;

    if (bIsPlayerControlled)
    {
        check(IsValid(MyPawnPC));

        FVector CamLoc;
        FRotator CamRot;
        MyPawnPC->GetPlayerViewPoint(CamLoc, CamRot);
        FinalAim = CamRot.Vector();
    }
    else
    if (GetInstigator())
    {
        FinalAim = GetInstigator()->GetBaseAimRotation().Vector();      
    }

    return FinalAim;
}

FVector AALSWeapon::GetAdjustedAim() const
{
    // Default is forward vector constant
    FVector FinalAim = FVector::ForwardVector;

    // Invalid pawn, abort and return default aim
    if (! IsValid(MyPawn))
    {
        return FinalAim;
    }

    // If we have a player controller use it for the aim
    if (bIsPlayerControlled)
    {
        check(MyPawnPC);

        //FVector CamLoc;
        //FRotator CamRot;
        //PlayerController->GetPlayerViewPoint(CamLoc, CamRot);
        //FinalAim = CamRot.Vector();

        FinalAim = MyPawnPC->GetControlRotation().Vector();
    }
    // Now see if we have an AI controller,
    // we will want to get the aim from there if we do
    else
    if (bIsAIControlled)
    {
        check(MyPawnAC);

        FinalAim = MyPawnAC->GetControlRotation().Vector();
    }
    else
    if (GetInstigator())
    {
        FinalAim = GetInstigator()->GetBaseAimRotation().Vector();
    }

    return FinalAim;
}

FVector AALSWeapon::GetCameraDamageStartLocation(const FVector& AimDir) const
{
    FVector OutStartTrace = FVector::ZeroVector;

    if (bIsPlayerControlled)
    {
        check(MyPawnPC);

        // use player's camera
        FRotator UnusedRot;
        MyPawnPC->GetPlayerViewPoint(OutStartTrace, UnusedRot);

        // Adjust trace so there is nothing blocking the ray between the camera and the pawn, and calculate distance from adjusted start
        OutStartTrace = OutStartTrace + AimDir * ((GetInstigator()->GetActorLocation() - OutStartTrace) | AimDir);
    }
    else
    if (bIsAIControlled)
    {
        OutStartTrace = GetMuzzleLocation();
    }

    return OutStartTrace;
}

FVector AALSWeapon::GetMuzzleLocation() const
{
    USkeletalMeshComponent* UseMesh = GetWeaponMesh();
    return UseMesh->GetSocketLocation(MuzzleAttachPoint);
}

FVector AALSWeapon::GetMuzzleDirection() const
{
    USkeletalMeshComponent* UseMesh = GetWeaponMesh();
    return UseMesh->GetSocketRotation(MuzzleAttachPoint).Vector();
}

FHitResult AALSWeapon::WeaponTrace(const FVector& StartTrace, const FVector& EndTrace) const
{

    // Perform trace to retrieve hit info
    FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(WeaponTrace), true, GetInstigator());
    TraceParams.bReturnPhysicalMaterial = true;

    FHitResult Hit(ForceInit);
    GetWorld()->LineTraceSingleByChannel(Hit, StartTrace, EndTrace, ECC_ALIAS_WEAPON, TraceParams);

    return Hit;
}

void AALSWeapon::SetOwningPawn(AALSCharacter* NewOwner)
{
    if (MyPawn != NewOwner)
    {
        SetInstigator(NewOwner);
        MyPawn = NewOwner;
        // net owner for RPC calls
        SetOwner(NewOwner);
    }

    AssignController();
}

void AALSWeapon::AssignController()
{
    MyPawnPC = nullptr;
    MyPawnAC = nullptr;

    bIsPlayerControlled = false;
    bIsAIControlled = false;

    if (IsValid(MyPawn))
    {
        // Assign PC

        MyPawnPC = GetInstigatorController<AALSPlayerController>();

        if (IsValid(MyPawnPC))
        {
            bIsPlayerControlled = true;
        }
        // No PC, Assign AIC
        else
        {
            MyPawnAC = GetInstigatorController<AALSAIController>();

            if (IsValid(MyPawnAC))
            {
                bIsAIControlled = true;
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// Replication & effects

void AALSWeapon::OnRep_MyPawn()
{
    if (IsValid(MyPawn))
    {
        OnEnterInventory(MyPawn);
    }
    else
    {
        OnLeaveInventory();
    }
}

void AALSWeapon::OnRep_BurstCounter()
{
    if (BurstCounter > 0)
    {
        SimulateWeaponFire();
    }
    else
    {
        StopSimulatingWeaponFire();
    }
}

void AALSWeapon::OnRep_Reload()
{
    if (bPendingReload)
    {
        StartReload(true);
    }
    else
    {
        StopReload();
    }
}

void AALSWeapon::SimulateWeaponFire()
{
    if (GetLocalRole() == ROLE_Authority && CurrentState != EWeaponState::Firing)
    {
        return;
    }

    if (MuzzleFX)
    {
        USkeletalMeshComponent* UseWeaponMesh = GetWeaponMesh();
        if (!bLoopedMuzzleFX || MuzzlePSC == NULL)
        {
            // Split screen requires we create 2 effects. One that we see and one that the other player sees.
            if( (MyPawn != NULL ) && ( MyPawn->IsLocallyControlled() == true ) )
            {
                AController* PlayerCon = MyPawn->GetController();               
                if( PlayerCon != NULL )
                {
                    Mesh1P->GetSocketLocation(MuzzleAttachPoint);
                    MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh1P, MuzzleAttachPoint);
                    MuzzlePSC->bOwnerNoSee = false;
                    MuzzlePSC->bOnlyOwnerSee = true;

                    Mesh3P->GetSocketLocation(MuzzleAttachPoint);
                    MuzzlePSCSecondary = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, Mesh3P, MuzzleAttachPoint);
                    MuzzlePSCSecondary->bOwnerNoSee = true;
                    MuzzlePSCSecondary->bOnlyOwnerSee = false;              
                }               
            }
            else
            {
                MuzzlePSC = UGameplayStatics::SpawnEmitterAttached(MuzzleFX, UseWeaponMesh, MuzzleAttachPoint);
            }
        }
    }

    if (!bLoopedFireAnim || !bPlayingFireAnim)
    {
        PlayWeaponAnimation(FireAnim);
        bPlayingFireAnim = true;
    }

    if (bLoopedFireSound)
    {
        if (FireAC == NULL)
        {
            FireAC = PlayWeaponSound(FireLoopSound);
        }
    }
    else
    {
        PlayWeaponSound(FireSound);
    }

    if (IsValid(MyPawnPC) && MyPawnPC->IsLocalController())
    {
        if (FireCameraShake != NULL)
        {
            MyPawnPC->ClientStartCameraShake(FireCameraShake, 1);
        }
        //if (FireForceFeedback != NULL && PC->IsVibrationEnabled())
        //{
        //    FForceFeedbackParameters FFParams;
        //    FFParams.Tag = "Weapon";
        //    PC->ClientPlayForceFeedback(FireForceFeedback, FFParams);
        //}
    }
}

void AALSWeapon::StopSimulatingWeaponFire()
{
    if (bLoopedMuzzleFX)
    {
        if( MuzzlePSC != NULL )
        {
            MuzzlePSC->DeactivateSystem();
            MuzzlePSC = NULL;
        }
        if( MuzzlePSCSecondary != NULL )
        {
            MuzzlePSCSecondary->DeactivateSystem();
            MuzzlePSCSecondary = NULL;
        }
    }

    if (bLoopedFireAnim && bPlayingFireAnim)
    {
        StopWeaponAnimation(FireAnim);
        bPlayingFireAnim = false;
    }

    if (FireAC)
    {
        FireAC->FadeOut(0.1f, 0.0f);
        FireAC = NULL;

        PlayWeaponSound(FireFinishSound);
    }
}

void AALSWeapon::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
    Super::GetLifetimeReplicatedProps( OutLifetimeProps );

    DOREPLIFETIME( AALSWeapon, MyPawn );

    DOREPLIFETIME_CONDITION( AALSWeapon, CurrentAmmo,       COND_OwnerOnly );
    DOREPLIFETIME_CONDITION( AALSWeapon, CurrentAmmoInClip, COND_OwnerOnly );

    DOREPLIFETIME_CONDITION( AALSWeapon, BurstCounter,      COND_SkipOwner );
    DOREPLIFETIME_CONDITION( AALSWeapon, bPendingReload,    COND_SkipOwner );
}

USkeletalMeshComponent* AALSWeapon::GetWeaponMesh() const
{
    //return (MyPawn != NULL && MyPawn->IsFirstPerson()) ? Mesh1P : Mesh3P;
    return Mesh3P;
}

class AALSCharacter* AALSWeapon::GetPawnOwner() const
{
    return MyPawn;
}

bool AALSWeapon::IsEquipped() const
{
    return bIsEquipped;
}

bool AALSWeapon::IsAttachedToPawn() const
{
    return bIsEquipped || bPendingEquip;
}

EWeaponState::Type AALSWeapon::GetCurrentState() const
{
    return CurrentState;
}

int32 AALSWeapon::GetCurrentAmmo() const
{
    return CurrentAmmo;
}

int32 AALSWeapon::GetCurrentAmmoInClip() const
{
    return CurrentAmmoInClip;
}

int32 AALSWeapon::GetAmmoPerClip() const
{
    return WeaponConfig.AmmoPerClip;
}

int32 AALSWeapon::GetMaxAmmo() const
{
    return WeaponConfig.MaxAmmo;
}

bool AALSWeapon::HasInfiniteAmmo() const
{
    //const AALSPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AALSPlayerController>(MyPawn->Controller) : NULL;
    //return WeaponConfig.bInfiniteAmmo || (MyPC && MyPC->HasInfiniteAmmo());
    return false;
}

bool AALSWeapon::HasInfiniteClip() const
{
    //const AALSPlayerController* MyPC = (MyPawn != NULL) ? Cast<const AALSPlayerController>(MyPawn->Controller) : NULL;
    //return WeaponConfig.bInfiniteClip || (MyPC && MyPC->HasInfiniteClip());
    return false;
}

float AALSWeapon::GetEquipStartedTime() const
{
    return EquipStartedTime;
}

float AALSWeapon::GetEquipDuration() const
{
    return EquipDuration;
}
