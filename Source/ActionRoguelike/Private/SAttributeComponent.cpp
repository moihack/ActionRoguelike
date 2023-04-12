// Fill out your copyright notice in the Description page of Project Settings.


#include "SAttributeComponent.h"

// Sets default values for this component's properties
USAttributeComponent::USAttributeComponent()
{
	HealthMax = 100;
	Health = HealthMax;
}

bool USAttributeComponent::Kill(AActor* InstigatorActor)
{
	return ApplyHealthChange(InstigatorActor, -GetHealthMax()); // -(minus) sign so that this becomes damage and not healing
}

bool USAttributeComponent::IsAlive() const
{
	return (Health > 0.0f);
}

bool USAttributeComponent::IsFullHealth() const
{
	return Health == HealthMax;
}

float USAttributeComponent::GetHealth() const
{
	return Health;
}

float USAttributeComponent::GetHealthMax() const
{
	return HealthMax;
}

bool USAttributeComponent::ApplyHealthChange(AActor* InstigatorActor, float Delta)
{
	// God mode is already a console command (God) that will set CanBeDamaged to false on Pawn controlled by player. 
	// Also check for Delta < 0 to only allow healing to go through as health change in God mode.
	if (!GetOwner()->CanBeDamaged() && Delta < 0.f)
	{
		return false;
	}

	float OldHealth = Health;

	Health = FMath::Clamp(Health + Delta, 0.0f, HealthMax);

	float ActualDelta = Health - OldHealth;
	OnHealthChanged.Broadcast(InstigatorActor, this, Health, ActualDelta);

	return ActualDelta != 0;
}

USAttributeComponent* USAttributeComponent::GetAttributes(AActor* FromActor)
{
	if (FromActor)
	{
		return Cast<USAttributeComponent>(FromActor->GetComponentByClass(USAttributeComponent::StaticClass()));
	}

	return nullptr;
}

bool USAttributeComponent::IsActorAlive(AActor* Actor) // static function defined in header file. Do NOT repeat the static keyword in implementation!
{
	USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(Actor); // since this is a static function there is NO access to this->IsAlive() , hence the use of (static as well) GetAttributes
	if (AttributeComp)
	{
		return AttributeComp->IsAlive();
	}
	return false; // default behavior : if no AttributeComponent is found consider actor dead
}

