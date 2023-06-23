// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameplayTagContainer.h"
#include "SAction.generated.h"

class UWorld;
class USActionComponent;

USTRUCT()
struct FActionRepData
{
	GENERATED_BODY()

public:

	// UPROPERTY(NotReplicated) // if you don't want to replicate something within a struct that will be replicated you would have to use the NotReplicated specifier!
	UPROPERTY()
	bool bIsRunning;

	UPROPERTY()
	AActor* Instigator;
};

/**
 *
 */
UCLASS(Blueprintable) // mark as Blueprintable otherwise child objects cannot be created in BPs (will not show up in editor when creating a new BP class).
class ACTIONROGUELIKE_API USAction : public UObject
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* Icon;

	UPROPERTY(Replicated)
	USActionComponent* ActionComp;

	UFUNCTION(BlueprintCallable, Category = "Action")
	USActionComponent* GetOwningComponent() const;

	// Tags added to owning actor when activated, removed when action stops.
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer GrantsTags;

	// Action can only start if owning actor has none of these Tags applied.
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer BlockedTags;

	// A note regarding RepNotify execution:
	// RepNotify executes only when the client variable is different from what the server sent.
	// 
	// Example: in this case a client (let's call him client1) has set bIsRunning to true, all other clients still have bIsRunning to false.
	// The server then sents a packet (to all clients) stating that bIsRunning is now set to true/ has now been changed to true.
	// Now client1 will NOT execute the RepNotify since it already has set its copy of bIsRunning variable to true.
	// This could lead to a variety of issues where some logic does not get executed on client1 .
	// 
	// In this project though, we change the bIsRunning variable on client on purpose to avoid an infinite loop of launching the same action again and again.
	// NOTE : You can still make the client always execute the RepNotify even if they have changed the variable locally already.
	// You can do so by using //DOREPLIFETIME_CONDITION_NOTIFY(USAction, ActionComp, COND_None, REPNOTIFY_Always); // Info from : https://forums.unrealengine.com/t/repnotify-from-c-confusion/3327/10
	// in USAction::Get Lifetime Replicated Props
	// WARNING : There seems to be a bug in UnrealHeaderTool (UHT) which would not generate the "Get Lifetime Replicated Props" function declaration in generated.h file
	// resulting in compilation error in SAction cpp. Splitting the words that make up the function name with a space seems to solve the compilation issue.
	// Still it is pretty weird that UHT did get confused by that despite being marked in a comment //.

	// All comments above are still valid but bIsRunning changed type (and name) in class L21 : Limiting Authority of the Client . 
	// It was only kept in place just for the sake of the example mentioned in the comments above.
	//UPROPERTY(ReplicatedUsing="OnRep_IsRunning")
	//bool bIsRunning;

	// Making a struct with 2 values instead of simply adding a second replicated value (e.g. the Instigator), 
	// ensures that both values will arrive at the same time.
	UPROPERTY(ReplicatedUsing = "OnRep_RepData")
	FActionRepData RepData;

	UFUNCTION()
	void OnRep_RepData();

	UPROPERTY(Replicated)
	float TimeStarted;

public:

	void Initialize(USActionComponent* NewActionComp);

	// Start immediately when added to an ActionComponent
	UPROPERTY(EditDefaultsOnly, Category = "Action")
	bool bAutoStart;

	UFUNCTION(BlueprintCallable, Category = "Action")
	bool IsRunning() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	bool CanStart(AActor* Instigator);

	// Although both Start/Stop Action are overriden in child classes (but called from base SAction references) 
	// virtual keyword is not added here (if added you get -> LogCompile: Error: BlueprintNativeEvent functions must be non-virtual.)
	// See SAction.cpp and SAction_ProjectileAttack.h on why this works

	UFUNCTION(BlueprintNativeEvent, Category = "Action")
	void StartAction(AActor* Instigator);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Action")
	void StopAction(AActor* Instigator);

	// Action nickname to start/stop without a reference to the object
	UPROPERTY(EditDefaultsOnly, Category = "Action")
	FName ActionName; // FName is hashed - Faster than FString for comparing between different ActionNames

		// must implement this for UObjects otherwise certain functions :
		// (like gameplay statics, spawning of actors, line traces, sweeps etc) 
		// will not show up in blueprint editor window in child classes.
	UWorld* GetWorld() const override;

	virtual bool IsSupportedForNetworking() const override // implement directly in header since this just returns true
	{
		return true;
	}
};
