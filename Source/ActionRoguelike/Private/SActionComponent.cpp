// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionComponent.h"
#include "SAction.h"

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

	for (TSubclassOf<USAction> ActionClass : DefaultActions)
	{
		AddAction(GetOwner(), ActionClass);
	}
	
}


// Called every frame
void USActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FString DebugMsg = GetNameSafe(GetOwner()) + " : " + ActiveGameplayTags.ToStringSimple();
	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, DebugMsg);
}

void USActionComponent::AddAction(AActor* Instigator, TSubclassOf<USAction> ActionClass)
{
	if (!ensure(ActionClass))
	{
		return;
	}

	USAction* NewAction = NewObject<USAction>(this, ActionClass);
	if (ensure(NewAction))
	{
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

bool USActionComponent::StartActionByName(AActor* Instigator, FName ActionName)
{
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
				Action->StopAction(Instigator);
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
