// Fill out your copyright notice in the Description page of Project Settings.

#include "SPowerup_HealthPotion.h"
#include "SAttributeComponent.h"
#include "SPlayerState.h"


#define LOCTEXT_NAMESPACE "InteractableActors"


ASPowerup_HealthPotion::ASPowerup_HealthPotion()
{
	CreditCost = 50;
}


void ASPowerup_HealthPotion::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!ensure(InstigatorPawn))
	{
		return;
	}

	USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(InstigatorPawn);
	// Check if not already at max health
	if (ensure(AttributeComp) && !AttributeComp->IsFullHealth())
	{
		if (ASPlayerState* PS = InstigatorPawn->GetPlayerState<ASPlayerState>())
		{
			HideAndCooldownPowerup();
			if (PS->RemoveCredits(CreditCost) && AttributeComp->ApplyHealthChange(this, AttributeComp->GetHealthMax()))
			{
				// Only activate if healed successfully
				HideAndCooldownPowerup();
			}
		}
	}
}

FText ASPowerup_HealthPotion::GetInteractText_Implementation(APawn* InstigatorPawn)
{
	USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(InstigatorPawn);

	if (AttributeComp && AttributeComp->IsFullHealth())
	{
		// NSLOCTEXT is an alternative to LOCTEXT incase LOCTEXT_NAMESPACE has not been defined.
		// (NSLOCTEXT stands for NameSpace Localization Text).
		// It accepts an extra parameter at the front to specify Localization NameSpace. 
		// Otherwise it follows the same syntax with LOCTEXT which uses LOCTEXT_NAMESPACE defined at the top of the current file!
		// Remember to undef LOCTEXT_NAMESPACE at the end of the current file as well!
		// Syntax for NSLOCTEXT is : LOCTEXT_NAMESPACE/Category , Key, DefaultTextValue whereas for LOCTEXT it is the same but Category is skipped as explained above.
		
		// return NSLOCTEXT("InteractableActors", "HealthPotion_FullHealthWarning", "Already at full health.");
		return LOCTEXT("HealthPotion_FullHealthWarning", "Already at full health.");
	}

	// read comments in if statement above about NSLOCTEXT usage
	// return FText::Format(NSLOCTEXT("InteractableActors", "HealthPotion_InteractMessage", "Cost {0} Credits. Restores health to maximum."), CreditCost);
	return FText::Format(LOCTEXT("HealthPotion_InteractMessage", "Cost {0} Credits. Restores health to maximum."), CreditCost);

	// Alternatively, instead of using the (NS)LOCTEXT UE MACROS, 
	// an FText variable could be exposed in BP and just have the exposed one localized.
	// However it is not always possible/wanted to expose something in BP
	// so the current approach was used as a showcase of localizable Text in C++.
}


#undef LOCTEXT_NAMESPACE // Note : Remember to undef LOCTEXT_NAMESPACE at EOF!
