// Fill out your copyright notice in the Description page of Project Settings.


#include "Point.h"

#include "EditorModeManager.h"
#include "FileHelpers.h"
#include "ProceduralDungeonEdModeToolkit.h"
#include "RoomData.h"
#include "SProceduralDungeonEdModeWidget.h"


// Sets default values
APoint::APoint()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	#if !WITH_EDITOR
	bIsEditorOnlyActor = true;
	#endif
}
#if WITH_EDITOR
void APoint::PostInitProperties()
{
	Super::PostInitProperties();
	FProceduralDungeonEdMode::OnEnterMode.BindLambda([this]() {
		FProceduralDungeonEdMode* EdMode = GetProceduralEdMode();
		if (EdMode->GetID() != FProceduralDungeonEdMode::EM_ProceduralDungeon)return;
		TSharedPtr<FModeToolkit> MToolkit = EdMode->GetToolkit();
		TSharedPtr<FProceduralDungeonEdModeToolkit> Toolkit = StaticCastSharedPtr<FProceduralDungeonEdModeToolkit>(MToolkit);
		TSharedPtr<class SWidget> Widget = Toolkit->GetInlineContent();
		TSharedRef<SProceduralDungeonEdModeWidget> MyWidget = StaticCastSharedRef<SProceduralDungeonEdModeWidget>(Toolkit->GetInlineContent().ToSharedRef());
		CachedData = MyWidget->GetCachedData();
		// FPointInfo* PointInfo = CachedData->PointInfos.Find(PointIndex);
		// if (PointInfo)
		// {
		// 	UE_LOG( LogTemp, Warning, TEXT("print:进入编辑模式 Point %d"),PointInfo->TargetType);
		// 	switch (PointInfo->TargetType)
		// 	{
		// 	case ETargetType::ETT_StaticMesh:
		// 		// 创建一个新的 StaticMeshComponent
		// 		if (PointInfo->StaticMesh)
		// 		{
		// 			StaticMeshComponent = NewObject<UStaticMeshComponent>(this);
		// 			StaticMeshComponent->SetStaticMesh(PointInfo->StaticMesh);
		// 			StaticMeshComponent->SetWorldTransform(PointInfo->Transform);
		// 			StaticMeshComponent->RegisterComponent();
		// 			this->AddInstanceComponent(StaticMeshComponent);
		// 		}
		// 		break;
		// 	case ETargetType::ETT_SkeletalMesh:
		// 		// 创建一个新的 SkeletalMeshComponent
		// 		if (PointInfo->SkeletalMesh)
		// 		{
		// 			SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this);
		// 			SkeletalMeshComponent->SetSkeletalMesh(PointInfo->SkeletalMesh);
		// 			SkeletalMeshComponent->SetWorldTransform(PointInfo->Transform);
		// 			SkeletalMeshComponent->RegisterComponent();
		// 			this->AddInstanceComponent(SkeletalMeshComponent);
		// 		}
		// 		break;
		// 	case ETargetType::ETT_Actor:
		// 		// 创建一个新的 Actor
		// 		if (PointInfo->Actor)
		// 		{
		// 			FActorSpawnParameters SpawnParams;
		// 			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// 			SpawnedActor = GetWorld()->SpawnActor<AActor>(PointInfo->Actor, PointInfo->Transform, SpawnParams);
		// 			if (SpawnedActor)
		// 			{
		// 				// 可以在这里对 SpawnedActor 进行进一步的设置
		// 				SpawnedActor->SetActorLocation(PointInfo->Transform.GetLocation());
		// 				SpawnedActor->SetActorRotation(PointInfo->Transform.GetRotation());
		// 			}
		// 		}
		// 		break;
		// 	case ETargetType::ETT_Tag:
		// 		break;
		// 	default:
		// 		break;
		// 	}
		// }
	});
	FProceduralDungeonEdMode::OnExitMode.BindLambda([this]() {
		CachedData = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("print:退出编辑模式 Point"));
		// 解绑
		// FProceduralDungeonEdMode::OnEnterMode.Remove(EnterModeHandle);
		// FProceduralDungeonEdMode::OnEnterMode.Remove(ExitModeHandle);
	});
}
#endif
// Called when the game starts or when spawned
void APoint::BeginPlay() { Super::BeginPlay(); }

void APoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (CachedData == nullptr)return;
	CachedData->SetPointInfo(PointIndex, this->GetTransform());
	CachedData->Modify();
	// switch (PointInfo->TargetType)
	// {
	// case ETargetType::ETT_StaticMesh:
	// 	StaticMeshComponent->SetWorldTransform(this->GetTransform());
	// 	break;
	// case ETargetType::ETT_SkeletalMesh:
	// 		SkeletalMeshComponent->SetWorldTransform(this->GetTransform());
	// 	break;
	// case ETargetType::ETT_Actor:
	// 	SpawnedActor->SetActorLocation(this->GetTransform().GetLocation());
	// 	SpawnedActor->SetActorRotation(this->GetTransform().GetRotation());
	// 	break;
	// case ETargetType::ETT_Tag:
	// 	break;
	// default:
	// 	break;
	// }
}


// Called every frame
void APoint::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

FProceduralDungeonEdMode* APoint::GetProceduralEdMode()
{
	FEditorModeTools& ModeTools = GLevelEditorModeTools();
	return static_cast<FProceduralDungeonEdMode*>(ModeTools.GetActiveMode(FProceduralDungeonEdMode::EM_ProceduralDungeon));
}


void APoint::SaveDta()
{
	auto Result = FEditorFileUtils::PromptForCheckoutAndSave(
		{CachedData->GetPackage()},
		// 保存这个 RoomData 所在的包（UAsset 文件）
		/*bCheckDirty=*/
		true,
		// 只保存被修改过的资源
		/*bPromptToSave=*/
		true // 不弹出保存提示框
	);
}

void APoint::RemoveDta()
{
	CachedData->RemovePointInfo(PointIndex);
	CachedData->Modify();
}
