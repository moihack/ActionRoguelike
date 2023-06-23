// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionEffect.h"
#include "SActionComponent.h"
#include "GameFramework/GameStateBase.h"

USActionEffect::USActionEffect()
{
	bAutoStart = true;
}

void USActionEffect::StartAction_Implementation(AActor* Instigator)
{
	Super::StartAction_Implementation(Instigator);

	if (Duration > 0.0f)
	{
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this, "StopAction", Instigator);

		GetWorld()->GetTimerManager().SetTimer(DurationHandle, Delegate, Duration, false);
	}

	if (Period > 0.0f)
	{
		FTimerDelegate Delegate;
		Delegate.BindUFunction(this, "ExecutePeriodicEffect", Instigator);

		GetWorld()->GetTimerManager().SetTimer(PeriodHandle, Delegate, Period, true); // looping timer
	}

}

void USActionEffect::StopAction_Implementation(AActor* Instigator)
{
	// Issue: DurationTimer might fire just a little bit before PeriodTimer could fire for one last time.
	
	// Example : DurationTimer=3 sec, PeriodTimer=1 sec (looping). 
	// PeriodTimer fires 2 times successfully but just before executing a 3rd (and final) time
	// DurationTimer elapses calling StopAction, thereby cancelling both timers.
	
	// Solution:
	// We want to make sure that Period Timer gets to execute before cancelling both timers below.
	// We also need to do this before calling Super::StopAction_Implementation 
	// because that would remove Grants/Blocked tags potentially affecting the functionality of the ActionEffect.
	if (GetWorld()->GetTimerManager().GetTimerRemaining(PeriodHandle) < KINDA_SMALL_NUMBER)
	{
		ExecutePeriodicEffect(Instigator);
	}

	Super::StopAction_Implementation(Instigator);

	GetWorld()->GetTimerManager().ClearTimer(PeriodHandle);
	GetWorld()->GetTimerManager().ClearTimer(DurationHandle);

	USActionComponent* Comp = GetOwningComponent();
	if (Comp)
	{
		Comp->RemoveAction(this);
	}

}

float USActionEffect::GetTimeRemaining() const
{
	AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	if (GS) // GameState may still getting replicated to a client that just joined the game hence the check if GS is valid
	{
		float EndTime = TimeStarted + Duration;
		// GS->GetServerWorldTimeSeconds() gets the seconds the game is running on the server
		// which may be diffrent from a client that just joined the game and used GetWorld()->TimeSeconds (which gets the time game is running on that particular client)
		return EndTime - GS->GetServerWorldTimeSeconds(); 
	}

	return Duration; // just a sane default value to return in case GS was not valid
}

void USActionEffect::ExecutePeriodicEffect_Implementation(AActor* InstigatorActor)
{

}