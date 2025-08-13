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
	
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Point", meta=(ClampMin="0.00", ClampMax="1.00", UIMin="0.00", UIMax="1.00"))
	float Probability=1;
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Point")
	ETargetType TargetType = ETargetType::ETT_Tag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point", meta=(EditCondition="TargetType == ETargetType::ETT_StaticMesh"))
	UStaticMesh* StaticMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point", meta=(EditCondition="TargetType == ETargetType::ETT_SkeletalMesh"))
	USkeletalMesh* SkeletalMesh = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point", meta=(EditCondition="TargetType == ETargetType::ETT_Actor"))
	TSubclassOf<AActor> Actor = nullptr;
	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Point", meta=(EditCondition="TargetType == ETargetType::ETT_Tag"))
	FGameplayTag Tag;
	
	FPointInfo* PointInfo=nullptr;
	FPointInfo* LastPointInfo=nullptr;
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
	virtual void PostRegisterAllComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	FProceduralDungeonEdMode* GetProceduralEdMode();
	void RemoveDta();
	void OnRoomDataPropertyChanged(URoomData* RoomData);
	UFUNCTION()
	void OnEnter();
	UFUNCTION()
	void OnExit();
	void SetMesh();
	static bool ArePointInfosEqual(const FPointInfo* NewInfo, const FPointInfo* OldInfo);
	void Init(int32 Index);
};
