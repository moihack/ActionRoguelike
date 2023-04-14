// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameModeBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"
//#include "EnvironmentQuery/EnvQueryTypes.h" // probably unnecessary but it was included in the lecture (it still compiles fine without it) - it is included in SGameModeBase.h anyway. 
#include "AI/SAICharacter.h"
#include "SAttributeComponent.h"
#include "EngineUtils.h" // for TActorIterator
#include "DrawDebugHelpers.h"
#include "SCharacter.h"

static TAutoConsoleVariable<bool> CVarSpawnBots(TEXT("su.SpawnBots"), true, TEXT("Enable spawning of bots via timer."), ECVF_Cheat);

ASGameModeBase::ASGameModeBase()
{
	SpawnTimerInterval = 2.0f;
}

void ASGameModeBase::StartPlay()
{

	Super::StartPlay(); // calls BeginPlay on actors.

	// looping timer for spawning bots
	GetWorldTimerManager().SetTimer(TimerHandle_SpawnBots, this, &ASGameModeBase::SpawnBotTimerElapsed, SpawnTimerInterval, true);

}

void ASGameModeBase::KillAll()
{
	for (TActorIterator<ASAICharacter> It(GetWorld()); It; ++It) // see comments below for alternate implementations
	{
		ASAICharacter* Bot = *It;

		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(Bot);
		if (ensure(AttributeComp) && AttributeComp->IsAlive())
		{
			// @fixme: maybe pass in player? for kill credit
			AttributeComp->Kill(this); // SGameModeBase inheritance chain : AGameModeBase->AInfo->AActor . So GameMode is also an actor and can be passed as instigator actor in Kill function.
		}
	}
}

void ASGameModeBase::SpawnBotTimerElapsed()
{
	if (!CVarSpawnBots.GetValueOnGameThread())
	{
		UE_LOG(LogTemp, Warning, TEXT("Bot spawning disabled via cvar 'CVarSpawnBots'."));
		return;
	}

	int32 NrOfAliveBots = 0;
	for (TActorIterator<ASAICharacter> It(GetWorld()); It; ++It) // see comments below for alternate implementations
	{
		ASAICharacter* Bot = *It;

		USAttributeComponent* AttributeComp = USAttributeComponent::GetAttributes(Bot);
		if (ensure(AttributeComp) && AttributeComp->IsAlive())
		{
			NrOfAliveBots++;
		}
	}
	// Instead of TActorIterator we could have also used UGameplayStatics::GetAllActorsOfClass -> then for loop the result
	// or TActorRange
	// for (ASAICharacter* Bot : TActorRange<ASAICharacter>(GetWorld())) {for loop body stays the same}

	UE_LOG(LogTemp, Log, TEXT("Found %i alive bots."), NrOfAliveBots);

	float MaxBotCount = 10.0f; // we used a float curve for DifficultyCurve asset hence this is a float and not an int

	if (DifficultyCurve)
	{
		MaxBotCount = DifficultyCurve->GetFloatValue(GetWorld()->TimeSeconds); // if we have assigned a DifficultyCurve set MaxBotCount to that value in time
	}

	if (NrOfAliveBots >= MaxBotCount) // don't spawn a new bot if too many exist
	{
		UE_LOG(LogTemp, Log, TEXT("At maximum bot capacity. Skipping bot spawn."));
		return;
	}

	// quit early - only execute an EQSQuery if bot count < max! this should save some performance.
	UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(this, SpawnBotQuery, this, EEnvQueryRunMode::RandomBest5Pct, nullptr);
	if (ensure(QueryInstance))
	{
		QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &ASGameModeBase::OnQueryCompleted);
	}
}

void ASGameModeBase::OnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	if (QueryStatus != EEnvQueryStatus::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawn bot EQS Query failed!"));
		return;
	}

	TArray<FVector> Locations = QueryInstance->GetResultsAsLocations(); // actual function returns FOccluderVertexArray which is a typedef for TArray<FVector>

	if (Locations.Num() > 0) // alternative : Locations.IsValidIndex(0)
	{
		GetWorld()->SpawnActor<AActor>(MinionClass, Locations[0], FRotator::ZeroRotator);

		// Track al the used spawn locations
		DrawDebugSphere(GetWorld(), Locations[0], 50.0f, 20, FColor::Blue, false, 60.0f);
	}
}

void ASGameModeBase::RespawnPlayerElapsed(AController* Controller)
{
	if (ensure(Controller))
	{
		Controller->UnPossess();

		RestartPlayer(Controller);
	}
}

void ASGameModeBase::OnActorKilled(AActor* VictimActor, AActor* Killer)
{
	ASCharacter* Player = Cast<ASCharacter>(VictimActor);
	if (Player)
	{
		// FTimerHandle as local variable and NOT exposed in header on purpose.
		// Otherwise this could lead to a bug in a multiplayer setting.
		// In case the TimerHandle exists on the header, 
		// if two players die almost simultaneously then
		// SetTimer would be called again for the same handle and end up resetting the timer instead!
		// On that occasion the first player would be killed but never respawn
		// while the 2nd player would respawn normally.
		FTimerHandle TimerHandle_RespawnDelay; 

		FTimerDelegate Delegate;
		Delegate.BindUFunction(this, "RespawnPlayerElapsed", Player->GetController());

		float RespawnDelay = 2.0f;
		
		GetWorldTimerManager().SetTimer(TimerHandle_RespawnDelay, Delegate, RespawnDelay, false);

		// Example on how timer was set inside SCharacter
		// GetWorldTimerManager().SetTimer(TimerHandle_PrimaryAttack, this, &ASCharacter::PrimaryAttack_TimeElapsed, AttackAnimDelay);

		// Setting it similarly here would not work as intended as we need to also pass a parameter in RespawnPlayerElapsed 
		// hence we have to use an FTimerDelegate and bind to a UFUNCTION to achieve that instead.
		// GetWorldTimerManager().SetTimer(TimerHandle_RespawnDelay, this, &ASGameModeBase::RespawnPlayerElapsed, RespawnDelay);
	}

	UE_LOG(LogTemp, Log, TEXT("OnActorKilled : Victim: %s, Killer: %s"), *GetNameSafe(VictimActor), *GetNameSafe(Killer));
}