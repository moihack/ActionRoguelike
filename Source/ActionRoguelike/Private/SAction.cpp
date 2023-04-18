// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction.h"
#include "SActionComponent.h"

bool USAction::CanStart_Implementation(AActor* Instigator) // called in USActionComponent::StartActionByName
{
	if (IsRunning()) // Do NOT allow starting an already running action - solves the issue with attack spamming in StopAction_Implementation.
	{
		return false;
	}

	USActionComponent* Comp = GetOwningComponent();
	if (Comp->ActiveGameplayTags.HasAny(BlockedTags))
	{
		return false;
	}

	return true;
}

// Note : _Implementation methods are declared as virtual in "SAction.generated.h" !
// virtual keyword cannot appear in definition (e.g. virtual void USAction::StartAction_Implementation(AActor* Instigator) is not valid in .cpp file).
// _Implementation is used for definition due to them being marked with UPROPERTY specifier BlueprintNativeEvent

void USAction::StartAction_Implementation(AActor* Instigator) 
{
	UE_LOG(LogTemp, Log, TEXT("Running: %s"), *GetNameSafe(this));

	USActionComponent* Comp = GetOwningComponent();
	Comp->ActiveGameplayTags.AppendTags(GrantsTags);

	bIsRunning = true;
}

void USAction::StopAction_Implementation(AActor* Instigator)
{
	UE_LOG(LogTemp, Log, TEXT("Stopped: %s"), *GetNameSafe(this));

	// This ensure would trigger when launching the SAME ATTACK TWICE in QUICK SUCCESSION (via double-clicking, press Q twice quickly etc).
	// It can happen for all abilities (MagicProjectile, Dash, Blackhole) regardless of the attack type. 
	// Each projectile's USAction_ProjectileAttack::AttackDelay_Elapsed is calling StopAction_Implementation (directly and not via SActionComponent like sprint does in SCharacter).
	// It runs correctly for the first projectile (stoppping the action) but the second one tries to stop an already stopped action thus triggering the ensure.
	// 
	// Solution in USAction::CanStart_Implementation. 
	// Note : The ensure firing is also fixed (as a side-effect) when adding Action.Attacking in BlockedTags of each action BP asset 
	// to stop the player from launching two different types of attacks at the same time. 
	// This however only works if the Content is setup correctly (e.g. all BP attack actions have BlockedTags properly set).
	// That's why the IsRunning() check has been left in place in USAction::CanStart_Implementation.
	// This way the ensure won't trigger regardless of BlockedTags set in Action_XXX_BP asset.
	ensureAlways(bIsRunning); 

	USActionComponent* Comp = GetOwningComponent();
	Comp->ActiveGameplayTags.RemoveTags(GrantsTags);

	bIsRunning = false;
}

UWorld* USAction::GetWorld() const
{
	// Outer is set when creating action via NewObject<T> (in USActionComponent::AddAction)
	UActorComponent* Comp = Cast<UActorComponent>(GetOuter()); // No need to cast to USActionComponent.
	if (Comp) // Comp could be null since the Engine/Editor may call GetWorld() in a SAction object with Outer assigned to something else than USActionComponent.
	{
		return Comp->GetWorld();
	}

	return nullptr;
}

USActionComponent* USAction::GetOwningComponent() const
{
	return Cast<USActionComponent>(GetOuter());
}

bool USAction::IsRunning() const
{
	return bIsRunning;
}