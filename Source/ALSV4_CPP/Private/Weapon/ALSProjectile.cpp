// 

#include "Weapon/ALSProjectile.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
//#include "Effects/ALSExplosionEffect.h"

AALSProjectile::AALSProjectile(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    CollisionComp = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(this, TEXT("SphereComp"));
    CollisionComp->InitSphereRadius(5.0f);
    CollisionComp->AlwaysLoadOnClient = true;
    CollisionComp->AlwaysLoadOnServer = true;
    CollisionComp->bTraceComplexOnMove = true;
    CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionComp->SetCollisionObjectType(ECC_ALIAS_PROJECTILE);
    CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    RootComponent = CollisionComp;

    ParticleComp = ObjectInitializer.CreateDefaultSubobject<UParticleSystemComponent>(this, TEXT("ParticleComp"));
    ParticleComp->bAutoActivate = false;
    ParticleComp->bAutoDestroy = false;
    ParticleComp->SetupAttachment(RootComponent);

    MovementComp = ObjectInitializer.CreateDefaultSubobject<UProjectileMovementComponent>(this, TEXT("ProjectileComp"));
    MovementComp->UpdatedComponent = CollisionComp;
    MovementComp->bInitialVelocityInLocalSpace = false;
    MovementComp->InitialSpeed = 2000.0f;
    MovementComp->MaxSpeed = 2000.0f;
    MovementComp->bRotationFollowsVelocity = true;
    MovementComp->ProjectileGravityScale = 0.f;

    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
    bReplicates = true;
    SetReplicatingMovement(true);
}

void AALSProjectile::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    MovementComp->OnProjectileStop.AddDynamic(this, &AALSProjectile::OnImpact);
    CollisionComp->MoveIgnoreActors.Add(GetInstigator());

    AALSWeapon_Projectile* OwnerWeapon = Cast<AALSWeapon_Projectile>(GetOwner());
    if (OwnerWeapon)
    {
        OwnerWeapon->ApplyWeaponConfig(WeaponConfig);
    }

    SetLifeSpan( WeaponConfig.ProjectileLife );
    MyController = GetInstigatorController();
}

void AALSProjectile::InitVelocity(FVector& ShootDirection)
{
    if (MovementComp)
    {
        MovementComp->Velocity = ShootDirection * MovementComp->InitialSpeed;
    }
}

void AALSProjectile::OnImpact(const FHitResult& HitResult)
{
    if (GetLocalRole() == ROLE_Authority && !bExploded)
    {
        Explode(HitResult);
        DisableAndDestroy();
    }
}

void AALSProjectile::Explode(const FHitResult& Impact)
{
    if (ParticleComp)
    {
        ParticleComp->Deactivate();
    }

    // effects and damage origin shouldn't be placed inside mesh at impact point
    const FVector NudgedImpactLocation = Impact.ImpactPoint + Impact.ImpactNormal * 10.0f;

    if (WeaponConfig.ExplosionDamage > 0 && WeaponConfig.ExplosionRadius > 0 && WeaponConfig.DamageType)
    {
        UGameplayStatics::ApplyRadialDamage(this, WeaponConfig.ExplosionDamage, NudgedImpactLocation, WeaponConfig.ExplosionRadius, WeaponConfig.DamageType, TArray<AActor*>(), this, MyController.Get());
    }

    //if (ExplosionTemplate)
    //{
    //    FTransform const SpawnTransform(Impact.ImpactNormal.Rotation(), NudgedImpactLocation);
    //    AALSExplosionEffect* const EffectActor = GetWorld()->SpawnActorDeferred<AALSExplosionEffect>(ExplosionTemplate, SpawnTransform);
    //    if (EffectActor)
    //    {
    //        EffectActor->SurfaceHit = Impact;
    //        UGameplayStatics::FinishSpawningActor(EffectActor, SpawnTransform);
    //    }
    //}

    bExploded = true;
}

void AALSProjectile::DisableAndDestroy()
{
    UAudioComponent* ProjAudioComp = FindComponentByClass<UAudioComponent>();
    if (ProjAudioComp && ProjAudioComp->IsPlaying())
    {
        ProjAudioComp->FadeOut(0.1f, 0.f);
    }

    MovementComp->StopMovementImmediately();

    // give clients some time to show explosion
    SetLifeSpan( 2.0f );
}

///CODE_SNIPPET_START: AActor::GetActorLocation AActor::GetActorRotation
void AALSProjectile::OnRep_Exploded()
{
    FVector ProjDirection = GetActorForwardVector();

    const FVector StartTrace = GetActorLocation() - ProjDirection * 200;
    const FVector EndTrace = GetActorLocation() + ProjDirection * 150;
    FHitResult Impact;

    bool bHasImpact = GetWorld()->LineTraceSingleByChannel(
        Impact,
        StartTrace,
        EndTrace,
        ECC_ALIAS_PROJECTILE,
        FCollisionQueryParams(
            SCENE_QUERY_STAT(ProjClient),
            true,
            GetInstigator()
            )
        );
    
    if (! bHasImpact)
    {
        // failsafe
        Impact.ImpactPoint = GetActorLocation();
        Impact.ImpactNormal = -ProjDirection;
    }

    Explode(Impact);
}
///CODE_SNIPPET_END

void AALSProjectile::PostNetReceiveVelocity(const FVector& NewVelocity)
{
    if (MovementComp)
    {
        MovementComp->Velocity = NewVelocity;
    }
}

void AALSProjectile::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
    Super::GetLifetimeReplicatedProps( OutLifetimeProps );
    
    DOREPLIFETIME( AALSProjectile, bExploded );
}
