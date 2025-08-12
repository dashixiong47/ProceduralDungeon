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
	#if WITH_EDITOR
	if (GIsEditor && !IsRunningGame())
	{
		StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
		StaticMeshComponent->SetupAttachment(RootComponent);

		SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
		SkeletalMeshComponent->SetupAttachment(RootComponent);
	}
	#endif
	#if !WITH_EDITOR
	bIsEditorOnlyActor = true;
	#endif
}
#if WITH_EDITOR
void APoint::PostRegisterAllComponents()
{
	
	if (GIsEditor && !IsRunningGame())
	{
		// 只在编辑器模式下执行（非游戏运行）
		Super::PostRegisterAllComponents();
		if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor)
		{
			// 只在编辑器世界绑定
			FProceduralDungeonEdMode::OnEnterMode.RemoveAll(this);
			FProceduralDungeonEdMode::OnExitMode.RemoveAll(this);
			EnterModeHandle = FProceduralDungeonEdMode::OnEnterMode.AddUObject(this, &APoint::OnEnter);
			ExitModeHandle = FProceduralDungeonEdMode::OnExitMode.AddUObject(this, &APoint::OnExit);
		}
	}
}

void APoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 获取被修改的属性名
	FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(APoint, Probability))
	{
		if (CachedData.IsValid())
		{
			if (PointInfo)
			{
				PointInfo->Probability = Probability;
				CachedData->Modify();
			}
		}
	}
}
#endif
// Called when the game starts or when spawned
void APoint::BeginPlay()
{
	Super::BeginPlay();
	// 销毁
	this->Destroy();
}

void APoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (CachedData == nullptr)return;
	CachedData->SetPointInfo(PointIndex, this->GetTransform());
	CachedData->Modify();
}

// Called every frame
void APoint::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

FProceduralDungeonEdMode* APoint::GetProceduralEdMode()
{
	FEditorModeTools& ModeTools = GLevelEditorModeTools();
	return static_cast<FProceduralDungeonEdMode*>(ModeTools.GetActiveMode(FProceduralDungeonEdMode::EM_ProceduralDungeon));
}


void APoint::RemoveDta()
{
	CachedData->RemovePointInfo(PointIndex);
	CachedData->Modify();
}

void APoint::OnRoomDataPropertyChanged(URoomData* RoomData)
{
	if (!CachedData->PointInfos.Contains(PointIndex))
	{
		Destroy();
		UE_LOG( LogTemp, Warning, TEXT("Point Index %d 不存在于 RoomData 中，已删除该 Point"), PointIndex);
		return;
	}
	LastPointInfo=PointInfo;
	PointInfo = CachedData->PointInfos.Find(PointIndex);
	SetMesh();
	Probability=PointInfo->Probability;
}

void APoint::OnEnter()
{

	if (!this)return;
	FProceduralDungeonEdMode* EdMode = GetProceduralEdMode();
	if (EdMode->GetID() != FProceduralDungeonEdMode::EM_ProceduralDungeon)return;
	TSharedPtr<FModeToolkit> MToolkit = EdMode->GetToolkit();
	TSharedPtr<FProceduralDungeonEdModeToolkit> Toolkit = StaticCastSharedPtr<FProceduralDungeonEdModeToolkit>(MToolkit);
	TSharedPtr<class SWidget> Widget = Toolkit->GetInlineContent();
	TSharedRef<SProceduralDungeonEdModeWidget> MyWidget = StaticCastSharedRef<SProceduralDungeonEdModeWidget>(Toolkit->GetInlineContent().ToSharedRef());

	CachedData = MyWidget->GetCachedData();
	// 监听所有对象属性变化
	CachedData->OnPropertiesChanged.RemoveAll(this);
	CachedData->OnPropertiesChanged.AddUObject(this, &APoint::OnRoomDataPropertyChanged);
	LastPointInfo=PointInfo;
	PointInfo = CachedData->PointInfos.Find(PointIndex);
	
	Probability=PointInfo->Probability;
	SetMesh();
}

void APoint::OnExit()
{
	if (!CachedData.IsValid())return;
	CachedData->OnPropertiesChanged.RemoveAll(this);
	CachedData = nullptr;
	UE_LOG(LogTemp, Warning, TEXT("print:退出编辑模式 Point"));
}

void APoint::SetMesh()
{
	if (ArePointInfosEqual(LastPointInfo,PointInfo))return;
	if (PointInfo)
	{
		switch (PointInfo->TargetType)
		{
		case ETargetType::ETT_StaticMesh:
			if (PointInfo->StaticMesh)
			{
				if (!StaticMeshComponent)return;
				StaticMeshComponent->SetStaticMesh(PointInfo->StaticMesh);
				if (SkeletalMeshComponent->IsVisible()) { SkeletalMeshComponent->SetSkeletalMesh(nullptr); }
				if (SpawnedActor)
				{
					SpawnedActor->Destroy();
					SpawnedActor = nullptr;
				}
			}
			break;
		case ETargetType::ETT_SkeletalMesh:
			if (PointInfo->SkeletalMesh)
			{
				if (!StaticMeshComponent)return;
				SkeletalMeshComponent->SetSkeletalMesh(PointInfo->SkeletalMesh);
				if (StaticMeshComponent->IsVisible()) { StaticMeshComponent->SetStaticMesh(nullptr); }
				if (SpawnedActor)
				{
					SpawnedActor->Destroy();
					SpawnedActor = nullptr;
				}
			}
			break;
		case ETargetType::ETT_Actor:
			// 创建一个新的 Actor
			if (PointInfo->Actor)
			{
				FActorSpawnParameters SpawnParams;
				SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				SpawnedActor = GetWorld()->SpawnActor<AActor>(PointInfo->Actor, PointInfo->Transform, SpawnParams);
				if (SpawnedActor)
				{
					// 可以在这里对 SpawnedActor 进行进一步的设置
					SpawnedActor->SetActorLocation(PointInfo->Transform.GetLocation());
					SpawnedActor->SetActorRotation(PointInfo->Transform.GetRotation());
					SpawnedActor->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
				}
				if (StaticMeshComponent->IsVisible()) { StaticMeshComponent->SetStaticMesh(nullptr); }
				if (SkeletalMeshComponent->IsVisible()) { SkeletalMeshComponent->SetSkeletalMesh(nullptr); }
			}
			break;
		case ETargetType::ETT_Tag:
			if (StaticMeshComponent->IsVisible()) { StaticMeshComponent->SetStaticMesh(nullptr); }
			if (SkeletalMeshComponent->IsVisible()) { SkeletalMeshComponent->SetSkeletalMesh(nullptr); }
			if (SpawnedActor)
			{
				SpawnedActor->Destroy();
				SpawnedActor = nullptr;
			}
			break;
		default:
			break;
		}
	}
}

bool APoint::ArePointInfosEqual(const FPointInfo* NewInfo, const FPointInfo* OldInfo)
{
	if (NewInfo||OldInfo)return false; // 如果有一个为空，直接返回false
	if (NewInfo->TargetType != OldInfo->TargetType)
		return false;
	switch (NewInfo->TargetType)
	{
	case ETargetType::ETT_StaticMesh:
		
		return NewInfo->StaticMesh == OldInfo->StaticMesh;

	case ETargetType::ETT_SkeletalMesh:
		return NewInfo->SkeletalMesh == OldInfo->SkeletalMesh;

	case ETargetType::ETT_Actor:
		return NewInfo->Actor == OldInfo->Actor;

	case ETargetType::ETT_Tag:
		// 如果需要比较Tag也可以加，这里先返回true表示其他字段忽略
		return true;

	default:
		return false;
	}
}

void APoint::Init(int32 Index)
{
	FProceduralDungeonEdMode::OnEnterMode.RemoveAll(this);
	FProceduralDungeonEdMode::OnExitMode.RemoveAll(this);
	EnterModeHandle = FProceduralDungeonEdMode::OnEnterMode.AddUObject(this, &APoint::OnEnter);
	ExitModeHandle = FProceduralDungeonEdMode::OnExitMode.AddUObject(this, &APoint::OnExit);
	PointIndex = Index;
	OnEnter();
}
