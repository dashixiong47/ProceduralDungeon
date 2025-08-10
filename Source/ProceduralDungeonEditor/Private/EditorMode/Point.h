// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProceduralDungeonEdMode.h"
#include "RoomData.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/Actor.h"
#include "Point.generated.h"


UCLASS(Blueprintable)
class PROCEDURALDUNGEONEDITOR_API APoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APoint();
	TWeakObjectPtr<URoomData> CachedData;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Point")
	int32 PointIndex = 0;
	
	UPROPERTY()
	UStaticMeshComponent* StaticMeshComponent=nullptr;
	UPROPERTY()
	USkeletalMeshComponent* SkeletalMeshComponent=nullptr;
	UPROPERTY()
	AActor* SpawnedActor=nullptr;
	FDelegateHandle EnterModeHandle;
	FDelegateHandle ExitModeHandle;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void PostEditMove(bool bFinished) override;
	
#if WITH_EDITOR
	virtual void PostInitProperties() override;
#endif
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	FProceduralDungeonEdMode* GetProceduralEdMode();
	UFUNCTION( BlueprintCallable, Category = "ProceduralDungeon")
	void SaveDta();
	void RemoveDta();
};
