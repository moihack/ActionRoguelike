// Fill out your copyright notice in the Description page of Project Settings.


#include "SGameModeBase.h"
#include "EnvironmentQuery/EnvQueryManager.h"
//#include "EnvironmentQuery/EnvQueryTypes.h" // probably unnecessary but it was included in the lecture (it still compiles fine without it) - it is included in SGameModeBase.h anyway. 
#include "AI/SAICharacter.h"
#include "SAttributeComponent.h"
#include "EngineUtils.h" // for TActorIterator
#include "DrawDebugHelpers.h"
#include "SCharacter.h"
#include "SPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "SSaveGame.h"
#include "GameFramework/GameStateBase.h"
#include "SGameplayInterface.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "SMonsterData.h"

static TAutoConsoleVariable<bool> CVarSpawnBots(TEXT("su.SpawnBots"), true, TEXT("Enable spawning of bots via timer."), ECVF_Cheat);

ASGameModeBase::ASGameModeBase()
{
	SpawnTimerInterval = 2.0f;

	CreditsPerKill = 20;

	DesiredPowerupCount = 10;
	RequiredPowerupDistance = 2000;

	PlayerStateClass = ASPlayerState::StaticClass(); // alternative way to assign default GameMode classes without assigning them from Editor via GameMode inherited Blueprint.

	SlotName = "SaveGame01";
}

void ASGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	LoadSaveGame(); // load save game as early as possible
}

void ASGameModeBase::StartPlay()
{

	Super::StartPlay(); // calls BeginPlay on actors.

	// looping timer for spawning bots
	GetWorldTimerManager().SetTimer(TimerHandle_SpawnBots, this, &ASGameModeBase::SpawnBotTimerElapsed, SpawnTimerInterval, true);

	// Make sure we have assigned at least one power-up class
	if (ensure(PowerupClasses.Num() > 0))
	{
		// Run EQS to find potential power-up spawn locations
		UEnvQueryInstanceBlueprintWrapper* QueryInstance = UEnvQueryManager::RunEQSQuery(this, PowerupSpawnQuery, this, EEnvQueryRunMode::AllMatching, nullptr);
		if (ensure(QueryInstance))
		{
			QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &ASGameModeBase::OnPowerupSpawnQueryCompleted);
		}
	}

}

void ASGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{	
	// Calling Before Super:: so we set variables before 'beginplayingstate' is called in PlayerController (which is where we instantiate UI in PlayerController_BP)
	ASPlayerState* PS = NewPlayer->GetPlayerState<ASPlayerState>();
	if (ensure(PS))
	{
		PS->LoadPlayerState(CurrentSaveGame);
	}

	// this eventually will call APlayerController::BeginPlayingState 
	// (which is where we instantiate UI in PlayerController_BP which needs PlayerState to be available)
	// hence we first load the PlayerState from savegame to ensure UI gets correct initial (loaded) values.
	Super::HandleStartingNewPlayer_Implementation(NewPlayer); 
}

void ASGameModeBase::KillAll()
{
	for (TActorIterator<ASAICharacter> It(GetWorld()); It; ++It) // see comments below in SpawnBotTimerElapsed() for alternate implementations
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
		QueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &ASGameModeBase::OnBotSpawnQueryCompleted);
	}
}

void ASGameModeBase::OnBotSpawnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	if (QueryStatus != EEnvQueryStatus::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawn bot EQS Query failed!"));
		return;
	}

	TArray<FVector> Locations = QueryInstance->GetResultsAsLocations(); // actual function returns FOccluderVertexArray which is a typedef for TArray<FVector>

	if (Locations.Num() > 0) // alternative : Locations.IsValidIndex(0)
	{

		if (MonsterTable)
		{
			TArray<FMonsterInfoRow*> Rows;
			MonsterTable->GetAllRows("", Rows);

			// Get Random Enemy
			int32 RandomIndex = FMath::RandRange(0, Rows.Num() - 1);
			FMonsterInfoRow* SelectedRow = Rows[RandomIndex];

			GetWorld()->SpawnActor<AActor>(SelectedRow->MonsterData->MonsterClass, Locations[0], FRotator::ZeroRotator);

		}

		// Track al the used spawn locations
		DrawDebugSphere(GetWorld(), Locations[0], 50.0f, 20, FColor::Blue, false, 60.0f);
	}
}

void ASGameModeBase::OnPowerupSpawnQueryCompleted(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	if (QueryStatus != EEnvQueryStatus::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawn PowerUp EQS Query Failed!"));
		return;
	}

	TArray<FVector> Locations = QueryInstance->GetResultsAsLocations();

	// Keep used locations to easily check distance between points
	TArray<FVector> UsedLocations;

	int32 SpawnCounter = 0;
	// Break out if we reached the desired count or if we have no more potential positions remaining
	while (SpawnCounter < DesiredPowerupCount && Locations.Num() > 0)
	{
		// Pick a random location from remaining points.
		int32 RandomLocationIndex = FMath::RandRange(0, Locations.Num() - 1);

		FVector PickedLocation = Locations[RandomLocationIndex];
		// Remove to avoid picking again
		Locations.RemoveAt(RandomLocationIndex);

		// Check minimum distance requirement
		bool bValidLocation = true;
		for (FVector OtherLocation : UsedLocations)
		{
			float DistanceTo = (PickedLocation - OtherLocation).Size();

			if (DistanceTo < RequiredPowerupDistance)
			{
				// Show skipped locations due to distance
				//DrawDebugSphere(GetWorld(), PickedLocation, 50.0f, 20, FColor::Red, false, 10.0f);

				// too close, skip to next attempt
				bValidLocation = false;
				break;
			}
		}

		// Failed the distance test
		if (!bValidLocation)
		{
			continue;
		}

		// Pick a random powerup-class
		int32 RandomClassIndex = FMath::RandRange(0, PowerupClasses.Num() - 1);
		TSubclassOf<AActor> RandomPowerupClass = PowerupClasses[RandomClassIndex];

		GetWorld()->SpawnActor<AActor>(RandomPowerupClass, PickedLocation, FRotator::ZeroRotator);

		// Keep for distance checks
		UsedLocations.Add(PickedLocation);
		SpawnCounter++;
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
	UE_LOG(LogTemp, Log, TEXT("OnActorKilled: Victim: %s, Killer: %s"), *GetNameSafe(VictimActor), *GetNameSafe(Killer));

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

	APawn* KillerPawn = Cast<APawn>(Killer);
	// Don't credit kills of self
	if (KillerPawn && KillerPawn != VictimActor)
	{
		// Only Players will have a 'PlayerState' instance, bots have nullptr
		// AI enemies don't have PlayerState by default (unless bWantsPlayerState is set to true in AAIController)
		if (ASPlayerState* PS = KillerPawn->GetPlayerState<ASPlayerState>()) // < can cast and check for nullptr within if-statement.
		{
			PS->AddCredits(CreditsPerKill);
		}
	}
}

void ASGameModeBase::WriteSaveGame()
{
	// Iterate all player states, we don't have proper ID to match yet (requires Steam or EOS)
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		ASPlayerState* PS = Cast<ASPlayerState>(GameState->PlayerArray[i]);
		if (PS)
		{
			PS->SavePlayerState(CurrentSaveGame);
			break; // single player only at this point
		}
	}

	// clear saved actors array before saving 
	// otherwise we end up appending actors to (previously saved/loaded) CurrentSaveGame
	// Not only will the SaveGame grow big in size as more saves happen
	// but loading would also be broken as the game would always load "the first version" ever saved of the previously saved actors
	CurrentSaveGame->SavedActors.Empty(); 

	for (FActorIterator It(GetWorld()); It; ++It) // see comments above in SpawnBotTimerElapsed() for alternate implementations
	{
		AActor* Actor = *It;
		// Only interested in our 'gameplay actors'
		if (!Actor->Implements<USGameplayInterface>()) // we could search using a filter above e.g. make a new interface like USSaveGameInterface
		{
			continue;
		}

		FActorSaveData ActorData;
		ActorData.ActorName = Actor->GetName();
		ActorData.Transform = Actor->GetActorTransform();

		///// NOTE : comments about saving taken from : https://www.tomlooman.com/unreal-engine-cpp-save-system/
		// also see comments in LoadSaveGame()
		 
		// To convert variables into a binary array we need an FMemoryWriter. 
		// Pass the array to fill with data from Actor
		FMemoryWriter MemWriter(ActorData.ByteData);

		// FObjectAndNameAsStringProxyArchive which is derived from FArchive
		// (Unreal’s data container for all sorts of serialized data including your game content).
		FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);

		// Find only variables with UPROPERTY(SaveGame)
		Ar.ArIsSaveGame = true;

		// Converts Actor's SaveGame UPROPERTIES into binary array
		// the Serialize() function available in every UObject / Actor 
		// to convert our variables to a binary array and back into variables again. 
		// To decide which variables to store, Unreal uses a ‘SaveGame’ UPROPERTY specifier.
		Actor->Serialize(Ar);

		CurrentSaveGame->SavedActors.Add(ActorData);
	}

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SlotName, 0);
}

void ASGameModeBase::LoadSaveGame()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		CurrentSaveGame = Cast<USSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (CurrentSaveGame == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load SaveGame Data."));
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("Loaded SaveGame Data."));

		for (FActorIterator It(GetWorld()); It; ++It) // see comments above in SpawnBotTimerElapsed() for alternate implementations
		{
			AActor* Actor = *It;
			// Only interested in our 'gameplay actors'
			if (!Actor->Implements<USGameplayInterface>())
			{
				continue;
			}

			for (FActorSaveData ActorData : CurrentSaveGame->SavedActors)
			{
				if (ActorData.ActorName == Actor->GetName()) 
				{
					Actor->SetActorTransform(ActorData.Transform);	

					///// NOTE : comments about saving taken from : https://www.tomlooman.com/unreal-engine-cpp-save-system/
					// also see comments in WriteSaveGame()

					// use an FMemoryReader to convert each Actor’s binary data back into “Unreal” Variables.
					FMemoryReader MemReader(ActorData.ByteData);

					FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
					Ar.ArIsSaveGame = true;

					// Convert binary array back into actor's variables
					// Somewhat confusingly we still use Serialize() on the Actor, 
					// but because we pass in an FMemoryReader instead of an FMemoryWriter 
					// the function can be used to pass saved variables back into the Actors.
					Actor->Serialize(Ar);

					ISGameplayInterface::Execute_OnActorLoaded(Actor);

					break; // we found the actor to restore its saved data - time to search for next actor in world to load its saved data.
				}
			}

		}

	}
	else
	{
		CurrentSaveGame = Cast<USSaveGame>(UGameplayStatics::CreateSaveGameObject(USSaveGame::StaticClass()));

		UE_LOG(LogTemp, Warning, TEXT("Created New SaveGame Data."));
	}

	

}