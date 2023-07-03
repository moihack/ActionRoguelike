// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SMonsterData.generated.h"

class USAction;

/**
 * 
 */
UCLASS()
class ACTIONROGUELIKE_API USMonsterData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn Info")
	TSubclassOf<AActor> MonsterClass;

	/* Actions/buffs to grant this Monster */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn Info")
	TArray<TSubclassOf<USAction>> Actions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	UTexture2D* Icon;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		// the following constructor calls FPrimaryAssetId::ParseTypeAndName
		// which means PrimaryAssetType will get assigned to "Monsters" (see DefaultGame.ini).
		// PrimaryAssetType is a struct with just an FName to describe the logical type of this object.
		// 
		// also see struct FMonsterInfoRow in SGameModeBase.h 
		// on how to do DataAsset filtering in Editor using FPrimaryAssetId and `UPROPERTY(meta=(AllowedTypes="abc"))` specifier.
		return FPrimaryAssetId("Monsters", GetFName()); //GetFName will return the Name of the DataAsset (not the name of the class but of the asset as it's named in ContentBrowser)
	}

};
