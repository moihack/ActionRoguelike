// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "SAction.generated.h"

class UWorld;
class USActionComponent;

/**
 * 
 */
UCLASS(Blueprintable) // mark as Blueprintable otherwise child objects cannot be created in BPs (will not show up in editor when creating a new BP class).
class ACTIONROGUELIKE_API USAction : public UObject
{
	GENERATED_BODY()

protected:

	UFUNCTION(BlueprintCallable, Category = "Action")
	USActionComponent* GetOwningComponent() const;

	// Tags added to owning actor when activated, removed when action stops.
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer GrantsTags;

	// Action can only start if owning actor has none of these Tags applied.
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer BlockedTags;


public:

	// Although both Start/Stop Action are overriden in child classes (but called from base SAction references) 
	// virtual keyword is not added here (if added you get -> LogCompile: Error: BlueprintNativeEvent functions must be non-virtual.)
	// See SAction.cpp and SAction_ProjectileAttack.h on why this works

	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	void StartAction(AActor* Instigator); 

	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	void StopAction(AActor* Instigator);

	// Action nickname to start/stop without a reference to the object
	UPROPERTY(EditDefaultsOnly, Category = "Action")
	FName ActionName; // FName is hashed - Faster than FString for comparing between different ActionNames

	// must implement this for UObjects otherwise certain functions :
	// (like gameplay statics, spawning of actors, line traces, sweeps etc) 
	// will not show up in blueprint editor window in child classes.
	UWorld* GetWorld() const override; 
};
