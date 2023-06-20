// Fill out your copyright notice in the Description page of Project Settings.


#include "SItemChest.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

void ASItemChest::Interact_Implementation(APawn* InstigatorPawn)
{
	bLidOpened = !bLidOpened;
	OnRep_LidOpened(); // RepNotify functions need to be manually called on the Server! This runs on the server thanks to how SInteractionComponent is setup.
	// In C++ RepNotify / ReplicatedUsing - functions only get called on clients on replication.
	// The server, after setting the replicated values, has to call the OnRep - function manually 
	// because the server is responsible for setting the replicated values and does not receive them as replication
	// the server has to call the RepNotifies manually. Explanation from : https://forums.unrealengine.com/t/repnotify-function-only-executes-on-clients-not-on-the-server/88010/
	// Does the same apply to Blueprints as well though?
}

void ASItemChest::OnActorLoaded_Implementation()
{
	OnRep_LidOpened(); // restore lid position after loading from a previously saved game
}

void ASItemChest::OnRep_LidOpened() // automatically called on all clients when variable changes
{
	float currPitch = bLidOpened ? TargetPitch : 0.0f;
	LidMesh->SetRelativeRotation(FRotator(currPitch, 0, 0));
}

// Sets default values
ASItemChest::ASItemChest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;

	LidMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LidMesh"));
	LidMesh->SetupAttachment(BaseMesh);

	TargetPitch = 110;

	SetReplicates(true);
}

// Called when the game starts or when spawned
void ASItemChest::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASItemChest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ASItemChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const //function is defined in the ClassName.generated.h
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASItemChest, bLidOpened);
}