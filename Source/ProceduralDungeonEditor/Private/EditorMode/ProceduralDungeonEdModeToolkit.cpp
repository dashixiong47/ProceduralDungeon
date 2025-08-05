// Copyright Benoit Pelletier 2023 - 2025 All Rights Reserved.
//
// This software is available under different licenses depending on the source from which it was obtained:
// - The Fab EULA (https://fab.com/eula) applies when obtained from the Fab marketplace.
// - The CeCILL-C license (https://cecill.info/licences/Licence_CeCILL-C_V1-en.html) applies when obtained from any other source.
// Please refer to the accompanying LICENSE file for further details.

#include "ProceduralDungeonEdModeToolkit.h"
#include "ProceduralDungeonEdMode.h"
#include "SProceduralDungeonEdModeWidget.h"
#include "EditorModeManager.h"
#include "ISinglePropertyView.h"
#include "ProceduralDungeonEditorCommands.h"
#include "ProceduralDungeonEditorObject.h"
#include "ProceduralDungeonEditorSettings.h"
#include "ProceduralDungeonEdLog.h"
#include "RoomLevel.h"
#include "Tools/ProceduralDungeonEditorTool.h"

#define LOCTEXT_NAMESPACE "ProceduralDungeonEditor"

void FProceduralDungeonEdModeToolkit::Init(const TSharedPtr<class IToolkitHost>& InitToolkitHost)
{
	auto NameToCommandMap = FProceduralDungeonEditorCommands::Get().NameToCommandMap;

	TSharedRef<FUICommandList> CommandList = GetToolkitCommands();
	CommandList->MapAction(NameToCommandMap.FindChecked("Tool_Size"),
	                       FUIAction(
		                       FExecuteAction::CreateSP(this, &FProceduralDungeonEdModeToolkit::OnChangeTool, FName("Tool_Size")),
		                       FCanExecuteAction::CreateSP(this, &FProceduralDungeonEdModeToolkit::IsToolEnabled, FName("Tool_Size")),
		                       FIsActionChecked::CreateSP(this, &FProceduralDungeonEdModeToolkit::IsToolActive, FName("Tool_Size")))
	);

	CommandList->MapAction(NameToCommandMap.FindChecked("Tool_Door"),
	                       FUIAction(
		                       FExecuteAction::CreateSP(this, &FProceduralDungeonEdModeToolkit::OnChangeTool, FName("Tool_Door")),
		                       FCanExecuteAction::CreateSP(this, &FProceduralDungeonEdModeToolkit::IsToolEnabled, FName("Tool_Door")),
		                       FIsActionChecked::CreateSP(this, &FProceduralDungeonEdModeToolkit::IsToolActive, FName("Tool_Door")))
	);

	CommandList->MapAction(NameToCommandMap.FindChecked("Tool_Point"),
	                       FUIAction(
		                       FExecuteAction::CreateLambda([this]() {
			                       // 这里写你点击按钮后要执行的代码
			                       UE_LOG(LogTemp, Log, TEXT("Tool_Point 被点击了"));
			                       const UProceduralDungeonEditorSettings* EditorSettings = GetDefault<UProceduralDungeonEditorSettings>();
			                       UClass* DefaultClass = EditorSettings->DefaultPointClass.Get();
			                       if (!DefaultClass)
				                       DefaultClass = APoint::StaticClass();
		                       		FProceduralDungeonEdMode* ProceduralDungeonEdMode = static_cast<FProceduralDungeonEdMode*>(GetEditorMode());
		                       		URoomData* RoomData=ProceduralDungeonEdMode->GetLevel()->Data;
			                       UWorld* World = nullptr;
			                       if (GEditor)
			                       {
				                       World = GEditor->GetEditorWorldContext().World();
				                       if (World && DefaultClass)
				                       {
					                       FActorSpawnParameters SpawnParams;
					                       SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				                       		
					                       // 设置生成位置，这里示例用零点，你可以换成合适的位置
					                       FVector SpawnLocation = FVector::ZeroVector;
					                       FRotator SpawnRotation = FRotator::ZeroRotator;

					                       AActor* NewActor = World->SpawnActor<AActor>(DefaultClass, SpawnLocation, SpawnRotation, SpawnParams);
				                       		APoint* Point=static_cast<APoint*>(NewActor);
											Point->PointIndex=RoomData->GetPointIndex();
					                       if (NewActor) { UE_LOG(LogTemp, Log, TEXT("成功创建Actor: %s"), *NewActor->GetName()); }
					                       else { UE_LOG(LogTemp, Error, TEXT("创建Actor失败")); }
				                       }
			                       }
		                       }),
		                       FCanExecuteAction::CreateLambda([]() { return true; }),
		                       FIsActionChecked::CreateSP(this, &FProceduralDungeonEdModeToolkit::IsToolActive, FName("Tool_Point"))
	                       ));

	SAssignNew(EdModeWidget, SProceduralDungeonEdModeWidget, SharedThis(this));
	FModeToolkit::Init(InitToolkitHost);
}

void FProceduralDungeonEdModeToolkit::GetToolPaletteNames(TArray<FName>& InPaletteName) const { InPaletteName.Add(FName("DefaultPalette")); }

FText FProceduralDungeonEdModeToolkit::GetToolPaletteDisplayName(FName PaletteName) const
{
	if (PaletteName == FName("DefaultPalette")) { return LOCTEXT("Mode.Default", "Default"); }
	return FText();
}

void FProceduralDungeonEdModeToolkit::BuildToolPalette(FName Palette, FToolBarBuilder& ToolbarBuilder)
{
	auto CommandList = FProceduralDungeonEditorCommands::Get();

	// DoorType property from Settings
	UProceduralDungeonEditorObject* EditorSettings = GetDungeonEditorMode()->Settings;
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FSinglePropertyParams Params;
	#if !COMPATIBILITY
	Params.bHideAssetThumbnail = true;
	#endif
	Params.NamePlacement = EPropertyNamePlacement::Hidden;
	TSharedPtr<ISinglePropertyView> SinglePropView = PropertyEditorModule.CreateSingleProperty(EditorSettings, "DoorType", Params);

	TSharedPtr<SWidget> Widget =
		SNew(SHorizontalBox)
		.IsEnabled(this, &FProceduralDungeonEdModeToolkit::IsDoorTypeEnabled)
		+ SHorizontalBox::Slot()
		.VAlign(EVerticalAlignment::VAlign_Center)
		.AutoWidth()
		[
			SinglePropView.ToSharedRef()
		];

	ToolbarBuilder.BeginSection("Tools");
	ToolbarBuilder.AddToolBarButton(CommandList.SizeTool);
	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddToolBarButton(CommandList.DoorTool);
	ToolbarBuilder.AddToolBarButton(CommandList.PointTool);
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("Settings");
	ToolbarBuilder.AddWidget(Widget.ToSharedRef());
	ToolbarBuilder.EndSection();
}

FEdMode* FProceduralDungeonEdModeToolkit::GetEditorMode() const { return GLevelEditorModeTools().GetActiveMode(FProceduralDungeonEdMode::EM_ProceduralDungeon); }

FProceduralDungeonEdMode* FProceduralDungeonEdModeToolkit::GetDungeonEditorMode() const { return (FProceduralDungeonEdMode*)GetEditorMode(); }

TSharedPtr<SWidget> FProceduralDungeonEdModeToolkit::GetInlineContent() const { return EdModeWidget; }

void FProceduralDungeonEdModeToolkit::OnChangeTool(FName ToolName) const
{
	FProceduralDungeonEdMode* EdMode = GetDungeonEditorMode();
	if (!EdMode)
	{
		DungeonEd_LogError("Editor Mode is invalid.");
		return;
	}

	DungeonEd_LogInfo("Change Tool to '%s'", *ToolName.ToString());
	EdMode->SetActiveTool(ToolName);
}

bool FProceduralDungeonEdModeToolkit::IsToolEnabled(FName ToolName) const
{
	FProceduralDungeonEdMode* EdMode = GetDungeonEditorMode();
	return EdMode && EdMode->IsToolEnabled(ToolName);
}

bool FProceduralDungeonEdModeToolkit::IsToolActive(FName ToolName) const
{
	FProceduralDungeonEdMode* EdMode = GetDungeonEditorMode();
	if (EdMode)
	{
		FProceduralDungeonEditorTool* Tool = nullptr;
		if (EdMode->GetTool(ToolName, Tool))
			return EdMode->GetActiveTool() == Tool;
	}
	return false;
}

bool FProceduralDungeonEdModeToolkit::IsDoorTypeEnabled() const { return IsToolEnabled("Tool_Door"); }

void FProceduralDungeonEdModeToolkit::OnLevelChanged() { EdModeWidget->OnLevelChanged(); }

#undef LOCTEXT_NAMESPACE
