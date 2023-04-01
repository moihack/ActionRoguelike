// Fill out your copyright notice in the Description page of Project Settings.


#include "SDashProjectile.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

ASDashProjectile::ASDashProjectile()
{
	TeleportDelay = 0.2f;
	DetonateDelay = 0.2f;

	MoveComp->InitialSpeed = 6000.f;
}


void ASDashProjectile::BeginPlay()
{
	Super::BeginPlay();

	// don't get confused that &ASDashProjectile::Explode is passed as a parameter
	// actually Explode_Implementation will be called (and not the plain Explode) after DetonateDelay has passsed
	GetWorldTimerManager().SetTimer(TimerHandle_DelayedDetonate, this, &ASDashProjectile::Explode, DetonateDelay);
}

// Base class using BlueprintNativeEvent, we must reimplement the _Implementation not the plain Explode()
void ASDashProjectile::Explode_Implementation()
{
	// Clear timer if the Explode was already called through another source (like OnActorHit) to avoid entering here twice (and spawning particles twice etc).
	GetWorldTimerManager().ClearTimer(TimerHandle_DelayedDetonate);

	UGameplayStatics::SpawnEmitterAtLocation(this, ImpactVFX, GetActorLocation(), GetActorRotation());

	EffectComp->DeactivateSystem();

	MoveComp->StopMovementImmediately(); // stop moving so the character teleports at correct (impact) location
	SetActorEnableCollision(false); // make sure that no other events like overlaps/hits occur triggering something else

	FTimerHandle TimerHandle_DelayedTeleport; // local variable 2nd TimerHandle - not exposed on header as we never want to cancel this timer hence a reference would not be needed
	GetWorldTimerManager().SetTimer(TimerHandle_DelayedTeleport, this, &ASDashProjectile::TeleportInstigator, TeleportDelay);

	// Skip base implementation as it will destroy actor
	// and cancel our second timer (TimerHandle_DelayedTeleport) 
	// as a result TeleportInstigator() will never get called
	//Super::Explode_Implementation();
}


void ASDashProjectile::TeleportInstigator()
{
	AActor* ActorToTeleport = GetInstigator();
	if (ensure(ActorToTeleport))
	{
		// Keep instigator rotation or it may end up jarring
		ActorToTeleport->TeleportTo(GetActorLocation(), ActorToTeleport->GetActorRotation(), false, false);
	}
}