// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionComponent.h"
#include "SAction.h"
#include "../ActionRoguelike.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"

DECLARE_CYCLE_STAT(TEXT("StartActionByName"), STAT_StartActionByName, STATGROUP_STANFORD);



// Sets default values for this component's properties
USActionComponent::USActionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void USActionComponent::BeginPlay()
{
	Super::BeginPlay();

	// Server-only
	if (GetOwner()->HasAuthority())
	{
		for (TSubclassOf<USAction> ActionClass : DefaultActions)
		{
			AddAction(GetOwner(), ActionClass);
		}
	}
}


// Called every frame
void USActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//FString DebugMsg = GetNameSafe(GetOwner()) + " : " + ActiveGameplayTags.ToStringSimple();
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, DebugMsg);

	// Draw All Actions
	for (USAction* Action : Actions)
	{
		FColor TextColor = Action->IsRunning() ? FColor::Blue : FColor::White;
		FString ActionMsg = FString::Printf(TEXT("[%s] Action: %s"), *GetNameSafe(GetOwner()), *GetNameSafe(Action));

		LogOnScreen(this, ActionMsg, TextColor, 0.0f);
	}
}

void USActionComponent::AddAction(AActor* Instigator, TSubclassOf<USAction> ActionClass)
{
	if (!ensure(ActionClass))
	{
		return;
	}

	// if a client tries to add an action - early out and warn us in debug log
	if (!GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("Client attempting to AddAction. [Class: %s]"), *GetNameSafe(ActionClass));
		return;
	}

	USAction* NewAction = NewObject<USAction>(GetOwner(), ActionClass); // setting Outer to Actor owning Component. Also see USAction::GetWorld() implementation.
	if (ensure(NewAction))
	{
		NewAction->Initialize(this);

		Actions.Add(NewAction);

		// if an Action shall be autostarted but can't yet start,
		// perhaps there is something wrong design wise hence the ensure
		if (NewAction->bAutoStart && ensure(NewAction->CanStart(Instigator)))
		{
			NewAction->StartAction(Instigator);
		}
	}
}

void USActionComponent::RemoveAction(USAction* ActionToRemove)
{
	if (!ensure(ActionToRemove && !ActionToRemove->IsRunning()))
	{
		return;
	}

	Actions.Remove(ActionToRemove);
}

USAction* USActionComponent::GetAction(TSubclassOf<USAction> ActionClass) const
{
	for (USAction* Action : Actions)
	{
		if (Action && Action->IsA(ActionClass))
		{
			return Action;
		}
	}

	return nullptr;
}

bool USActionComponent::StartActionByName(AActor* Instigator, FName ActionName)
{
	SCOPE_CYCLE_COUNTER(STAT_StartActionByName); // this counts the execution cost (time) for the whole StartActionByName function

	// you can only count a specific part of a function by using curly braces 
	// to "embrace" the SCOPE_CYCLE_COUNTER MACRO. see example below in /* */
	// info from : https://www.tomlooman.com/unreal-engine-profiling-stat-commands/#:~:text=called%20FrameworkZeroPCH.h)-,Finally,-%2C%20it%E2%80%99s%20important%20to
	
	/* 
		// This part isn't counted

		{
			SCOPE_CYCLE_COUNTER(STAT_GetSingleModuleByClass);
			// .. Only measures the code inside the curly braces.
		}

		// This part isn't counted either, it stops at the bracket above.
	*/

	for (USAction* Action : Actions)
	{
		if (Action && Action->ActionName == ActionName)
		{
			if (!Action->CanStart(Instigator))
			{
				FString FailedMsg = FString::Printf(TEXT("Failed to run: %s"), *ActionName.ToString());
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FailedMsg);
				continue; // although unlikely there may be another Action in Actions TArray with the same name which would potentially execute hence we don't immediately exit/return from the for loop.
			}

			// Is Client? - Otherwise infinite loop if called from server! since ServerStartAction_Implementation would call StartActionByName again.
			if (!GetOwner()->HasAuthority())
			{
				// Question : Why is the following line executed (indeed) on the server despite being called from a client?
				// 
				// Answer : Since the SActionComponent is owned by a PlayerCharacter (SCharacter),
				// which in turn is owned by a PlayerController which in turn is also owned by a connection
				// the RPC invoked from client will indeed execute/run on the server.
				// For further details read about Ownership here: https://docs.unrealengine.com/4.27/en-US/InteractiveExperiences/Networking/Actors/OwningConnections/
				// as well as about RPCs here : https://docs.unrealengine.com/4.27/en-US/InteractiveExperiences/Networking/Actors/RPCs/
				// Make sure to check the RPC tables showcased in the 2nd link ("RPC invoked from a client" table for this specific case).
				// 
				ServerStartAction(Instigator, ActionName);
				//
				// Note/non-working example : calling a server event from an "unowned actor" or "actor Owned by a different client" would not succeed (DROPPED)
				// since that actor would NOT be owned by a PlayerController (and therefore a connection) 
				// as explained in the "RPC invoked from a client" table in the 2nd link above.
			}

			// Bookmark for Unreal Insights
			TRACE_BOOKMARK(TEXT("StartAction::%s"), *GetNameSafe(Action));

			Action->StartAction(Instigator);
			return true;
		}
	}

	return false;
}

bool USActionComponent::StopActionByName(AActor* Instigator, FName ActionName)
{
	for (USAction* Action : Actions)
	{
		if (Action && Action->ActionName == ActionName)
		{
			if (Action->IsRunning())
			{
				// is client? then send server RPC to stop action. also see USActionComponent::StartActionByName comments above
				if (!GetOwner()->HasAuthority())
				{		
					ServerStopAction(Instigator, ActionName);
				}

				Action->StopAction(Instigator); // also stop the action locally so no lag occurs.
				return true;
			}
		}
	}

	return false;
}

void USActionComponent::ServerStartAction_Implementation(AActor* Instigator, FName ActionName) // _Implementation needs to be used for Server functions as well
{
	StartActionByName(Instigator, ActionName);
}

void USActionComponent::ServerStopAction_Implementation(AActor* Instigator, FName ActionName) // _Implementation needs to be used for Server functions as well
{
	StopActionByName(Instigator, ActionName);
}

bool USActionComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	for (USAction* Action : Actions)
	{
		if (Action)
		{
			WroteSomething |= Channel->ReplicateSubobject(Action, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void USActionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //function is defined in the ClassName.generated.h
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USActionComponent, Actions);
}