// Copyright Benoit Pelletier 2023 - 2025 All Rights Reserved.
//
// This software is available under different licenses depending on the source from which it was obtained:
// - The Fab EULA (https://fab.com/eula) applies when obtained from the Fab marketplace.
// - The CeCILL-C license (https://cecill.info/licences/Licence_CeCILL-C_V1-en.html) applies when obtained from any other source.
// Please refer to the accompanying LICENSE file for further details.

#include "ProceduralDungeonEdMode.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "Engine/LevelScriptBlueprint.h"
#include "ProceduralDungeonEdModeToolkit.h"
#include "ProceduralDungeonEdLog.h"
#include "ProceduralDungeonEditorSettings.h"
#include "ProceduralDungeonEditorObject.h"
#include "Tools/ProceduralDungeonEditorTool_Size.h"
#include "Tools/ProceduralDungeonEditorTool_Door.h"
#include "Room.h"
#include "RoomLevel.h"
#include "RoomData.h"
#include "Selection.h"

#define ROUTE_TO_TOOL(FuncCall) ActiveTool ? ActiveTool->FuncCall : FEdMode::FuncCall

const FEditorModeID FProceduralDungeonEdMode::EM_ProceduralDungeon(TEXT("EM_ProceduralDungeon"));
FSimpleDelegate FProceduralDungeonEdMode::OnEnterMode;
FSimpleDelegate FProceduralDungeonEdMode::OnExitMode;

FProceduralDungeonEdMode::FProceduralDungeonEdMode()
	: FEdMode()
{
	Tools.Add(MakeUnique<FProceduralDungeonEditorTool_Size>(this));
	Tools.Add(MakeUnique<FProceduralDungeonEditorTool_Door>(this));

	Settings = NewObject<UProceduralDungeonEditorObject>(GetTransientPackage(), TEXT("Editor Settings"), RF_Transactional);
}

void FProceduralDungeonEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdMode::AddReferencedObjects(Collector);
	Collector.AddReferencedObject(Settings);
}

void FProceduralDungeonEdMode::Enter()
{
	DungeonEd_LogInfo("Enter Room Editor Mode.");
	FEdMode::Enter();

	if (!Toolkit.IsValid())
	{
		Toolkit = MakeShareable(new FProceduralDungeonEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	UpdateLevelBlueprint();

	// Turn on the flag to force the debug drawings.
	ARoomLevel::bIsDungeonEditorMode = true;
	RefreshPoint();
	if (!bDeleteActorDelegateBound)
	{
		FEditorDelegates::OnDeleteActorsBegin.AddRaw(this, &FProceduralDungeonEdMode::HandleActorDeleteBegin);
		bDeleteActorDelegateBound = true;
	}
	OnEnterMode.ExecuteIfBound();
}

void FProceduralDungeonEdMode::Exit()
{
	RegisterLevelCompilationDelegate(false);

	FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
	Toolkit.Reset();

	if (ActiveTool)
	{
		ActiveTool->ExitTool();
		ActiveTool = nullptr;
	}

	CachedLevelInstance.Reset();
	CachedLevelBlueprint.Reset();

	ARoomLevel::bIsDungeonEditorMode = false;

	FEdMode::Exit();
	if (bDeleteActorDelegateBound)
	{
		FEditorDelegates::OnDeleteActorsBegin.RemoveAll(this);
		bDeleteActorDelegateBound = false;
	}
	RefreshPoint();

	OnExitMode.ExecuteIfBound();

	DungeonEd_LogInfo("Exit Room Editor Mode.");
}

void FProceduralDungeonEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	if (ActiveTool)
		ActiveTool->Render(View, Viewport, PDI);

	FEdMode::Render(View, Viewport, PDI);
}

void FProceduralDungeonEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);

	if (ActiveTool)
		ActiveTool->Tick(ViewportClient, DeltaTime);
}

bool FProceduralDungeonEdMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) { return ROUTE_TO_TOOL(HandleClick(InViewportClient, HitProxy, Click)); }

bool FProceduralDungeonEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) { return ROUTE_TO_TOOL(InputKey(ViewportClient, Viewport, Key, Event)); }

bool FProceduralDungeonEdMode::InputAxis(FEditorViewportClient* InViewportClient, FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime) { return ROUTE_TO_TOOL(InputAxis(InViewportClient, Viewport, ControllerId, Key, Delta, DeltaTime)); }

bool FProceduralDungeonEdMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) { return ROUTE_TO_TOOL(InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale)); }

bool FProceduralDungeonEdMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY) { return ROUTE_TO_TOOL(MouseMove(ViewportClient, Viewport, MouseX, MouseY)); }

bool FProceduralDungeonEdMode::ShowModeWidgets() const { return true; }

bool FProceduralDungeonEdMode::ShouldDrawWidget() const { return true; }

bool FProceduralDungeonEdMode::UsesTransformWidget() const { return ROUTE_TO_TOOL(UsesTransformWidget()); }

bool FProceduralDungeonEdMode::UsesTransformWidget(WidgetMode CheckMode) const { return ROUTE_TO_TOOL(UsesTransformWidget(CheckMode)); }

FVector FProceduralDungeonEdMode::GetWidgetLocation() const { return ROUTE_TO_TOOL(GetWidgetLocation()); }

bool FProceduralDungeonEdMode::GetPivotForOrbit(FVector& OutPivot) const
{
	const auto* PluginSettings = GetDefault<UProceduralDungeonEditorSettings>();
	if (!PluginSettings->bUseRoomAsOrbitPivot)
		return false;

	if (!CachedLevelInstance.IsValid())
		return false;

	URoomData* Data = CachedLevelInstance->Data;
	if (!IsValid(Data))
		return false;

	FBoxCenterAndExtent RoomBounds = Data->GetBounds();
	OutPivot = RoomBounds.Center;
	return true;
}

bool FProceduralDungeonEdMode::GetCursor(EMouseCursor::Type& OutCursor) const { return ROUTE_TO_TOOL(GetCursor(OutCursor)); }

bool FProceduralDungeonEdMode::GetTool(FName ToolName, FProceduralDungeonEditorTool*& OutTool) const
{
	for (auto& Tool : Tools)
	{
		if (Tool.IsValid() && Tool->GetToolName() == ToolName)
		{
			OutTool = Tool.Get();
			return true;
		}
	}
	return false;
}

FProceduralDungeonEditorTool* FProceduralDungeonEdMode::GetActiveTool() const { return ActiveTool; }

void FProceduralDungeonEdMode::SetActiveTool(FName ToolName)
{
	if (ActiveTool && ActiveTool->GetToolName() == ToolName)
		return;

	FProceduralDungeonEditorTool* NewTool = nullptr;
	if (!GetTool(ToolName, NewTool))
	{
		DungeonEd_LogError("Tool '%s' is not a valid tool.", *ToolName.ToString());
		return;
	}

	check(NewTool);
	SetActiveTool(NewTool);
}

void FProceduralDungeonEdMode::ResetActiveTool() { SetActiveTool(nullptr); }

void FProceduralDungeonEdMode::SetActiveTool(FProceduralDungeonEditorTool* NewTool)
{
	if (ActiveTool)
		ActiveTool->ExitTool();

	DungeonEd_LogInfo("Set active tool to '%s'.", NewTool ? NewTool->GetToolName() : TEXT("None"));
	ActiveTool = NewTool;

	if (ActiveTool)
		ActiveTool->EnterTool();
}

void FProceduralDungeonEdMode::SetDefaultTool()
{
	if (!ActiveTool && IsToolEnabled("Tool_Size"))
		SetActiveTool("Tool_Size");
}

bool FProceduralDungeonEdMode::IsToolEnabled(FName ToolName) const
{
	auto Level = GetLevel();
	return Level.IsValid() && IsValid(Level->Data);
}

ULevelScriptBlueprint* FProceduralDungeonEdMode::GetLevelBlueprint(bool bCreate) const
{
	UWorld* World = GetWorld();
	check(World);
	ULevelScriptBlueprint* LevelBlueprint = World->PersistentLevel->GetLevelScriptBlueprint(/*bDontCreate = */ !bCreate);
	return LevelBlueprint;
}

TWeakObjectPtr<ARoomLevel> FProceduralDungeonEdMode::GetLevel() const
{
	ULevelScriptBlueprint* LevelBlueprint = GetLevelBlueprint();
	if (!IsValid(LevelBlueprint))
		return nullptr;
	return Cast<ARoomLevel>(LevelBlueprint->GeneratedClass->GetDefaultObject());
}

void FProceduralDungeonEdMode::UpdateLevelBlueprint()
{
	ULevelScriptBlueprint* LevelBlueprint = GetLevelBlueprint();
	if (CachedLevelBlueprint == LevelBlueprint)
		return;

	if (CachedLevelBlueprint.IsValid())
		RegisterLevelCompilationDelegate(false);

	CachedLevelBlueprint = LevelBlueprint;

	if (CachedLevelBlueprint.IsValid())
		RegisterLevelCompilationDelegate(true);

	OnLevelBlueprintCompiled();
}

void FProceduralDungeonEdMode::OnLevelBlueprintCompiled(UBlueprint* Blueprint)
{
	CachedLevelInstance = Cast<ARoomLevel>(GetWorld()->GetLevelScriptActor());

	auto Level = GetLevel();
	DungeonEd_LogInfo("Room Level: %s", *GetNameSafe(Level.Get()));

	if (Level.IsValid())
		SetDefaultTool();
	else
		ResetActiveTool();

	auto RoomToolkit = (FProceduralDungeonEdModeToolkit*)Toolkit.Get();
	check(RoomToolkit);
	RoomToolkit->OnLevelChanged();

	if (ActiveTool)
		ActiveTool->OnLevelChanged(Level.Get());
}

void FProceduralDungeonEdMode::RegisterLevelCompilationDelegate(bool Register)
{
	if (!CachedLevelBlueprint.IsValid())
	{
		DungeonEd_LogWarning("Can't (un)register level blueprint compilation delegate: the level blueprint is invalid.");
		return;
	}

	if (Register)
	{
		if (LevelBlueprintDelegateHandle.IsValid()) { DungeonEd_LogWarning("Can't register level blueprint compilation delegate: the delegate is already registered."); }
		else
		{
			LevelBlueprintDelegateHandle = CachedLevelBlueprint->OnCompiled().AddRaw(this, &FProceduralDungeonEdMode::OnLevelBlueprintCompiled);
			DungeonEd_LogInfo("Regitered level blueprint compilation delegate.");
		}
	}
	else
	{
		if (LevelBlueprintDelegateHandle.IsValid())
		{
			CachedLevelBlueprint->OnCompiled().Remove(LevelBlueprintDelegateHandle);
			LevelBlueprintDelegateHandle.Reset();
			DungeonEd_LogInfo("Unregitered level blueprint compilation delegate.");
		}
		else { DungeonEd_LogWarning("Can't unregister level blueprint compilation delegate: the delegate is not registered."); }
	}
}

void FProceduralDungeonEdMode::RefreshPoint()
{
	UWorld* World = GetWorld();
	if (!World) return;
	TWeakObjectPtr<ARoomLevel> Level = GetLevel();
	if (!Level.IsValid()) return;
	URoomData* RoomData = Level->Data;
	if (!RoomData) return;
	

	// 所有合法的索引
	TSet<int32> ValidPointIndices;
	RoomData->PointInfos.GetKeys(ValidPointIndices);
	
	const UProceduralDungeonEditorSettings* EditorSettings = GetDefault<UProceduralDungeonEditorSettings>();
	UClass* DefaultClass = EditorSettings->DefaultPointClass.Get();
	if (!DefaultClass)
		DefaultClass = APoint::StaticClass();
	// 已存在的索引
	TSet<int32> ExistingIndices;

	// 遍历场景中所有 APoint，删除非法的，记录合法的
	for (TActorIterator<APoint> It(World); It; ++It)
	{
	
		APoint* Point = *It;
		
		if (!Point) continue;
		if (Point->GetLevel() != World->PersistentLevel)
			continue; // 忽略非当前主关卡的 APoint
		UE_LOG(LogTemp, Warning, TEXT("Checking APoint: %d"), It->PointIndex);
		int32 Index = Point->PointIndex;

		if (!ValidPointIndices.Contains(Index))
		{
			// 不存在于 RoomData 的，删除
			GEditor->GetEditorWorldContext().World()->DestroyActor(Point);
			continue;
		}
		else if (ExistingIndices.Contains(Index))
		{
			GEditor->GetEditorWorldContext().World()->DestroyActor(Point);
			continue;
		}

		// 有效点，记录下
		ExistingIndices.Add(Index);

		// 更新 Transform 信息（如果需要）
		RoomData->SetPointInfo(Index, Point->GetTransform());
	}
	UE_LOG( LogTemp, Log, TEXT("Checked all APoint actors in the world.%d"),ValidPointIndices.Num());
	// 对于 RoomData 中存在但场景中没有的 Point，创建新的 APoint
	for (int32 Index : ValidPointIndices)
	{
		if (ExistingIndices.Contains(Index)) continue;

		FTransform Transform = RoomData->GetPointInfo(Index).Transform;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		APoint* NewPoint = World->SpawnActor<APoint>(DefaultClass, Transform, SpawnParams);
		if (NewPoint) { NewPoint->PointIndex = Index; }
	}

	RoomData->SaveDataAsset();
}

void FProceduralDungeonEdMode::HandleActorDeleteBegin()
{
	USelection* SelectedActors = GEditor->GetSelectedActors();
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		AActor* Actor = Cast<AActor>(*It);
		if (Actor && Actor->IsA<APoint>())
		{
			static_cast<APoint*>(Actor)->RemoveDta();
		}
	}
}
