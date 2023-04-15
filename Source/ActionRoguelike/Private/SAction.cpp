// Fill out your copyright notice in the Description page of Project Settings.


#include "SAction.h"

// Note : _Implementation methods are declared as virtual in "SAction.generated.h" !
// virtual keyword cannot appear in definition (e.g. virtual void USAction::StartAction_Implementation(AActor* Instigator) is not valid in .cpp file).
// _Implementation is used for definition due to them being marked with UPROPERTY specifier BlueprintNativeEvent

void USAction::StartAction_Implementation(AActor* Instigator) 
{
	UE_LOG(LogTemp, Log, TEXT("Running: %s"), *GetNameSafe(this));
}

void USAction::StopAction_Implementation(AActor* Instigator)
{
	UE_LOG(LogTemp, Log, TEXT("Stopped: %s"), *GetNameSafe(this));
}

UWorld* USAction::GetWorld() const
{
	// Outer is set when creating action via NewObject<T> (in USActionComponent::AddAction)
	UActorComponent* Comp = Cast<UActorComponent>(GetOuter()); // No need to cast to USActionComponent helps avoid an extra include.
	if (Comp) // Comp could be null since the Engine/Editor may call GetWorld() in a SAction object with Outer assigned to something else than USActionComponent.
	{
		return Comp->GetWorld();
	}

	return nullptr;
}
