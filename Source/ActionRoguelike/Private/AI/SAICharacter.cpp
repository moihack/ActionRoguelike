// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SAICharacter.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "SAttributeComponent.h"
#include "BrainComponent.h"

// Sets default values
ASAICharacter::ASAICharacter()
{
    PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");

    AttributeComp = CreateDefaultSubobject<USAttributeComponent>("AttributeComp");

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ASAICharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    PawnSensingComp->OnSeePawn.AddDynamic(this, &ASAICharacter::OnPawnSeen);

    AttributeComp->OnHealthChanged.AddDynamic(this, &ASAICharacter::OnHealthChanged);
}

void ASAICharacter::OnHealthChanged(AActor* InstigatorActor, USAttributeComponent* OwningComp, float NewHealth, float Delta)
{
    if (Delta < 0.0f)
    {

        if (NewHealth <= 0.0f) // AI Character just died
        {
            // stop BT
            AAIController* AIC = Cast<AAIController>(GetController());
            if (AIC) // the pawn may become UnPossessed by other code resulting in AIC being nullptr
            {
                AIC->GetBrainComponent()->StopLogic("Killed");
            }

            // ragdoll
            GetMesh()->SetAllBodiesSimulatePhysics(true);
            GetMesh()->SetCollisionProfileName("Ragdoll");

            // set lifespan
            SetLifeSpan(10.0f);
        }

    }
}

void ASAICharacter::OnPawnSeen(APawn* Pawn) // Note, this function will never run until the AI has seen the Player. 
{
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC)
    {
        UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();

        // "TargetActor" key in BB can be left empty if not seen the pawn yet. 
        // Also see QueryContext_TargetActor BP asset in editor for a solution (Cast Failed part).
        BBComp->SetValueAsObject("TargetActor", Pawn); 

        DrawDebugString(GetWorld(), GetActorLocation(), "PLAYER STPOTTED", nullptr, FColor::White, 4.0f, true);
    }
}
