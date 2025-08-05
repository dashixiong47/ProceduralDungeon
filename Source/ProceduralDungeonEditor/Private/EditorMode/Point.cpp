// Fill out your copyright notice in the Description page of Project Settings.


#include "Point.h"

#include "EditorModeManager.h"
#include "ProceduralDungeonEdModeToolkit.h"
#include "SProceduralDungeonEdModeWidget.h"


// Sets default values
APoint::APoint()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bIsEditorOnlyActor = true;
}

// Called when the game starts or when spawned
void APoint::BeginPlay()
{
	Super::BeginPlay();
	FProceduralDungeonEdMode* EdMode=GetProceduralEdMode();
	TSharedPtr<FModeToolkit> MToolkit = EdMode->GetToolkit();
	TSharedPtr<FProceduralDungeonEdModeToolkit> Toolkit = StaticCastSharedPtr<FProceduralDungeonEdModeToolkit>(MToolkit);
	TSharedPtr<class SWidget> Widget=Toolkit->GetInlineContent();
	TSharedRef<SProceduralDungeonEdModeWidget> MyWidget = StaticCastSharedRef<SProceduralDungeonEdModeWidget>(Toolkit->GetInlineContent().ToSharedRef());
 
}

// Called every frame
void APoint::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

FProceduralDungeonEdMode* APoint::GetProceduralEdMode()
{
	FEditorModeTools& ModeTools = GLevelEditorModeTools();
	return static_cast<FProceduralDungeonEdMode*>(ModeTools.GetActiveMode(FProceduralDungeonEdMode::EM_ProceduralDungeon));
}
