// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction.h"
#include "SActionComponent.h"
#include "../ActionRoguelike.h"
#include "Net/UnrealNetwork.h"

void USAction::Initialize(USActionComponent* NewActionComp)
{
	ActionComp = NewActionComp;
}

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
	UE_LOG(LogTemp, Log, TEXT("Started: %s"), *GetNameSafe(this));
	//LogOnScreen(this, FString::Printf(TEXT("Started: %s"), *ActionName.ToString()), FColor::Green);

	USActionComponent* Comp = GetOwningComponent();
	Comp->ActiveGameplayTags.AppendTags(GrantsTags);

	RepData.bIsRunning = true;
	RepData.Instigator = Instigator;

	// this part should only run on the server - alternative check to HasAuthority showcased here.
	if (GetOwningComponent()->GetOwnerRole() == ROLE_Authority) 
	{
		TimeStarted = GetWorld()->TimeSeconds; // TimeStarted now set on server and since it is replicated it has the same value for all clients
	}

	// TODO: we could replace GetOwningComponent() with Comp variable created above but currently following class code 1:1
	GetOwningComponent()->OnActionStarted.Broadcast(GetOwningComponent(), this);
}

void USAction::StopAction_Implementation(AActor* Instigator)
{
	UE_LOG(LogTemp, Log, TEXT("Stopped: %s"), *GetNameSafe(this));
	//LogOnScreen(this, FString::Printf(TEXT("Stopped: %s"), *ActionName.ToString()), FColor::White);

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

	// ensureAlways(bIsRunning); // disabled ensure in L21 : Networking UObjects & Actions (Action System) as it only makes sense on the server. Due to how multiplayer is setup it will unnecessarily trigger on the client.

	USActionComponent* Comp = GetOwningComponent();
	Comp->ActiveGameplayTags.RemoveTags(GrantsTags);

	RepData.bIsRunning = false;
	RepData.Instigator = Instigator;

	// TODO: we could replace GetOwningComponent() with Comp variable created above but currently following class code 1:1
	GetOwningComponent()->OnActionStopped.Broadcast(GetOwningComponent(), this);
}

UWorld* USAction::GetWorld() const
{
	// Outer is set when creating action via NewObject<T> (in USActionComponent::AddAction)
	AActor* Actor = Cast<AActor>(GetOuter()); // No need to cast to USActionComponent.
	if (Actor) // Comp (now called Actor) could be null since the Engine/Editor may call GetWorld() in a SAction object with Outer assigned to something else than USActionComponent.
	{		   // Comp was actually null due to that in L21 : Networking UObjects & Actions (Action System) at 14:55, hence the change to AActor type from UActorComponent
		return Actor->GetWorld();
	}

	// question from lecture regarding the change above, where Comp need to be changed to Actor:
	// why did unreal change the outer from actioncomp to the playercharacter?
	// 
	// answer (from Tom Looman) : It's because Unreal is instantiating those UObjects on the client side for us. 
	// The server tells the clients to make those instances via Replication. 
	// The client then instantiates them but specifies its own Outer (not the one we defined on the server) 
	// and so there is a difference. (Unreal didn't change anything on the server-side).

	// Note (by me): Server set Outer to AActor while client set Outer to USActionComp so there was a mismatch between types
	// and calling GetOuter() and GetOwningComponent() (also changed below) would not yield the same results on both client and server.

	// According to other comments in lecture this change seems to be uneccessary in UE5+ . 

	return nullptr;
}

USActionComponent* USAction::GetOwningComponent() const
{
	//return Cast<USActionComponent>(GetOuter()); // see above comments in USAction::GetWorld() on why this had to be changed.

	return ActionComp;
}

void USAction::OnRep_RepData()
{
	if (RepData.bIsRunning)
	{
		StartAction(RepData.Instigator);
	}
	else
	{
		StopAction(RepData.Instigator);
	}
}

bool USAction::IsRunning() const
{
	return RepData.bIsRunning;
}

void USAction::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const //function is defined in the ClassName.generated.h
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USAction, RepData);
	DOREPLIFETIME(USAction, ActionComp);
	//DOREPLIFETIME_CONDITION_NOTIFY(USAction, ActionComp, COND_None, REPNOTIFY_Always); // see NOTE above bIsRunning in SAction.h
	DOREPLIFETIME(USAction, TimeStarted);
}