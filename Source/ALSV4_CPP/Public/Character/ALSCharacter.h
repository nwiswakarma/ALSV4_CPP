// Project:         Advanced Locomotion System V4 on C++
// Copyright:       Copyright (C) 2021 Doğa Can Yanıkoğlu
// License:         MIT License (http://www.opensource.org/licenses/mit-license.php)
// Source Code:     https://github.com/dyanikoglu/ALSV4_CPP
// Original Author: Doğa Can Yanıkoğlu
// Contributors:    


#pragma once

#include "CoreMinimal.h"
#include "Character/ALSBaseCharacter.h"
#include "ALSCharacter.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnALSCharacterEquipWeapon, AALSCharacter*, AALSWeapon*);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnALSCharacterUnEquipWeapon, AALSCharacter*, AALSWeapon*);

/**
 * Specialized character class, with additional features like held object etc.
 */
UCLASS(Blueprintable, BlueprintType)
class ALSV4_CPP_API AALSCharacter : public AALSBaseCharacter
{
	GENERATED_BODY()

public:
	AALSCharacter(const FObjectInitializer& ObjectInitializer);

	/** spawn inventory, setup initial variables */
	virtual void PostInitializeComponents() override;

	/** setup replication properties */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** setup pawn specific input handlers */
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** cleanup inventory */
	virtual void Destroyed() override;

	/** reattach weapon */
	virtual void PawnClientRestart() override;

	/** Implemented on BP to update held objects */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|HeldObject")
	void UpdateHeldObject();

	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	void ClearHeldObject();

	UFUNCTION(BlueprintCallable, Category = "ALS|HeldObject")
	void AttachToHand(UStaticMesh* NewStaticMesh, USkeletalMesh* NewSkeletalMesh,
	                  class UClass* NewAnimClass, bool bLeftHand, FVector Offset);

	virtual void RagdollStart() override;

	virtual void RagdollEnd() override;

	virtual ECollisionChannel GetThirdPersonTraceParams(FVector& TraceOrigin, float& TraceRadius) override;

	virtual FTransform GetThirdPersonPivotTarget() override;

	virtual FVector GetFirstPersonCameraTarget() override;

    /** HANDLE WEAPON USAGE */

	/**
	* [server + local] equips weapon from inventory
	*
	* @param Weapon	Weapon to equip
	*/
	void EquipWeapon(class AALSWeapon* Weapon);

	/** get firing state */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	bool IsFiring() const
    {
        return bWantsToFire;
    }

	/** get currently equipped weapon */
	UFUNCTION(BlueprintCallable, Category = "Game|Weapon")
	class AALSWeapon* GetWeapon() const
    {
        return CurrentWeapon;
    }

	/** check if pawn can fire weapon */
	bool CanFire() const;

	/** check if pawn can reload weapon */
	bool CanReload() const;

protected:
	virtual void Tick(float DeltaTime) override;

	virtual void BeginPlay() override;

	virtual void OnOverlayStateChanged(EALSOverlayState PreviousState) override;

	/** Implement on BP to update animation states of held objects */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "ALS|HeldObject")
	void UpdateHeldObjectAnimations();

	//////////////////////////////////////////////////////////////////////////
	// Inventory

	/** [server] spawns default inventory */
	void SpawnDefaultInventory();

	/** [server] remove all weapons from inventory and destroy them */
	void DestroyInventory();

	/**
	* [server] add weapon to inventory
	*
	* @param Weapon	Weapon to add.
	*/
	void AddWeapon(class AALSWeapon* Weapon);

	/**
	* [server] remove weapon from inventory
	*
	* @param Weapon	Weapon to remove.
	*/
	void RemoveWeapon(class AALSWeapon* Weapon);

	/** equip weapon */
	UFUNCTION(reliable, server, WithValidation)
	void ServerEquipWeapon(class AALSWeapon* NewWeapon);

	/** updates current weapon */
	void SetCurrentWeapon(class AALSWeapon* NewWeapon, class AALSWeapon* LastWeapon = NULL);

	/** current weapon rep handler */
	UFUNCTION()
	void OnRep_CurrentWeapon(class AALSWeapon* LastWeapon);

    //////////////////////////////////////////////////////////////////////////
    // Weapon usage

	/** [local] starts weapon fire */
	void StartWeaponFire();

	/** [local] stops weapon fire */
	void StopWeaponFire();

	/** Player pressed start fire action */
	virtual void OnStartFire();

	/** Player released start fire action */
	virtual void OnStopFire();

	/** Player pressed reload action */
	virtual void OnReload();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USceneComponent* HeldObjectRoot = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMeshComponent* SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UStaticMeshComponent* StaticMesh = nullptr;

	/**
     * Global notification when a character equips a weapon.
     * Needed for replication graph.
     */
	static FOnALSCharacterEquipWeapon NotifyEquipWeapon;

	/**
     * Global notification when a character un-equips a weapon.
     * Needed for replication graph.
     */
	static FOnALSCharacterUnEquipWeapon NotifyUnEquipWeapon;

protected:
	/** default inventory list */
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	TArray<TSubclassOf<class AALSWeapon> > DefaultInventoryClasses;

	/** weapons in inventory */
	UPROPERTY(Transient, Replicated)
	TArray<class AALSWeapon*> Inventory;

	/** currently equipped weapon */
	UPROPERTY(Transient, ReplicatedUsing = OnRep_CurrentWeapon)
	class AALSWeapon* CurrentWeapon;

	/** current firing state */
	bool bWantsToFire = false;
private:
	bool bNeedsColorReset = false;
};
