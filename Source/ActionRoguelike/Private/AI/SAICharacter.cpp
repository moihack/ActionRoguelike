// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SAICharacter.h"
#include "Perception/PawnSensingComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "SAttributeComponent.h"
#include "BrainComponent.h"
#include "Blueprint/UserWidget.h"
#include "SWorldUserWidget.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SActionComponent.h"

// Sets default values
ASAICharacter::ASAICharacter()
{
    PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");

    AttributeComp = CreateDefaultSubobject<USAttributeComponent>("AttributeComp");

    ActionComp = CreateDefaultSubobject<USActionComponent>("ActionComp");

    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
    GetMesh()->SetGenerateOverlapEvents(true);

    TimeToHitParamName = "TimeToHit";
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

        if (InstigatorActor != this)
        {
            SetTargetActor(InstigatorActor); // Currently not checking if who hit is also an AICharacter. This could lead to AI fighting each other, similar to Monster infighting in DOOM games.
        } 

        if (ActiveHealthBar == nullptr)
        {
            ActiveHealthBar = CreateWidget<USWorldUserWidget>(GetWorld(), HealthBarWidgetClass);
            if (ActiveHealthBar)
            {
                ActiveHealthBar->AttachedActor = this;
                ActiveHealthBar->AddToViewport();
            }
        }

        GetMesh()->SetScalarParameterValueOnMaterials(TimeToHitParamName, GetWorld()->TimeSeconds);

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

            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision); // disable collision in capsule component left behind by dead AI character (use Show Collision to see it in Editor)
            GetCharacterMovement()->DisableMovement();

            // set lifespan
            SetLifeSpan(10.0f);
        }

    }
}

void ASAICharacter::SetTargetActor(AActor* NewTarget)
{
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC)
    {
        AIC->GetBlackboardComponent()->SetValueAsObject("TargetActor", NewTarget);
        // "TargetActor" key in BB can be left empty if not seen the pawn yet. 
        // See QueryContext_TargetActor BP asset in editor for a solution (Cast Failed part).
    }
}

void ASAICharacter::OnPawnSeen(APawn* Pawn) // Note, this function will never run until the AI has seen the Player. 
{
    SetTargetActor(Pawn);
    DrawDebugString(GetWorld(), GetActorLocation(), "PLAYER STPOTTED", nullptr, FColor::White, 4.0f, true);
}
