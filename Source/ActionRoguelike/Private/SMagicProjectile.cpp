// Fill out your copyright notice in the Description page of Project Settings.


#include "SMagicProjectile.h"
#include "Components/SphereComponent.h"
//#include "GameFramework/ProjectileMovementComponent.h"
//#include "Particles/ParticleSystemComponent.h"
#include "SAttributeComponent.h"
#include "SGameplayFunctionLibrary.h"
#include "SActionComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "SActionEffect.h"

// Sets default values
ASMagicProjectile::ASMagicProjectile()
{
	SphereComp->SetSphereRadius(20.0f);
	SphereComp->OnComponentBeginOverlap.AddDynamic(this, &ASMagicProjectile::OnActorOverlap);

	// Keep projectile alive for a maximum of 10sec (when not hitting a target). 
	// Otherwise after many missed projectiles have been launched they start to heavily increase frame time
	// since they keep doing calculations (particle effects etc).
	InitialLifeSpan = 10.0f;  

	DamageAmount = 20.0f;

}

void ASMagicProjectile::OnActorOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != GetInstigator())
	{
		//static FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Status.Parrying"); // C++ example on how to request a tag (not ideal since it needs a hardcoded string - better expose a UPROPERTY and assign via BP like we did with ParryTag)

		USActionComponent* ActionComp = Cast<USActionComponent>(OtherActor->GetComponentByClass(USActionComponent::StaticClass()));
		
		if (ActionComp && ActionComp->ActiveGameplayTags.HasTag(ParryTag))
		{
			MoveComp->Velocity = -MoveComp->Velocity; // revert velocity so the projetile flies back to where it came from (only if parrying)
			SetInstigator(Cast<APawn>(OtherActor)); // set new instigator otherwise reflected projectile won't hit the target due to `OtherActor != GetInstigator()`
			return;
		}

		if (USGameplayFunctionLibrary::ApplyDirectionalDamage(GetInstigator(), OtherActor, DamageAmount, SweepResult))
		{
			Explode();

			if (ActionComp && HasAuthority()) // allow adding burning action only on server side
			{
				ActionComp->AddAction(GetInstigator(), BurningActionClass);
			}

		}
	}
}

// Called when the game starts or when spawned
void ASMagicProjectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASMagicProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

