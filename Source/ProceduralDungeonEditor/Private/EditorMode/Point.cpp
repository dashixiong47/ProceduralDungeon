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
	// 创建一个默认的场景组件作为根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent->bAutoActivate = true;
	#if !WITH_EDITOR
	bIsEditorOnlyActor = true;
	#endif
}
#if WITH_EDITOR
void APoint::PostInitProperties()
{
	Super::PostInitProperties();
	FProceduralDungeonEdMode::OnEnterMode.BindLambda([this]() {
		FProceduralDungeonEdMode* EdMode=GetProceduralEdMode();
	if (EdMode->GetID()!=FProceduralDungeonEdMode::EM_ProceduralDungeon)return;
	TSharedPtr<FModeToolkit> MToolkit = EdMode->GetToolkit();
	TSharedPtr<FProceduralDungeonEdModeToolkit> Toolkit = StaticCastSharedPtr<FProceduralDungeonEdModeToolkit>(MToolkit);
	TSharedPtr<class SWidget> Widget=Toolkit->GetInlineContent();
	TSharedRef<SProceduralDungeonEdModeWidget> MyWidget = StaticCastSharedRef<SProceduralDungeonEdModeWidget>(Toolkit->GetInlineContent().ToSharedRef());
	CachedData= MyWidget->GetCachedData();
		CachedData->SetPointInfo( PointIndex,this->GetTransform());
	UE_LOG( LogTemp, Warning, TEXT("print:进入编辑模式 Point"));
	});
	FProceduralDungeonEdMode::OnExitMode.BindLambda([this]() {
		CachedData = nullptr;
		UE_LOG( LogTemp, Warning, TEXT("print:退出编辑模式 Point"));
	});
}
#endif
// Called when the game starts or when spawned
void APoint::BeginPlay()
{
	Super::BeginPlay();
}

void APoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (CachedData==nullptr)return;
	CachedData->SetPointInfo( PointIndex,this->GetTransform());
	CachedData->Modify();
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
	{ CachedData->GetPackage() },  // 保存这个 RoomData 所在的包（UAsset 文件）
	/*bCheckDirty=*/true,    // 只保存被修改过的资源
	/*bPromptToSave=*/true  // 不弹出保存提示框
);
}
