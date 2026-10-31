#include "ArenaGameMode.h"
#include "ArenaAssets.h"
#include "ArenaEnemy.h"
#include "ArenaHealthComponent.h"
#include "ArenaPickup.h"
#include "ArenaPlayerCharacter.h"
#include "ArenaPlayerController.h"
#include "ArenaProjectile.h"
#include "ArenaSaveGame.h"
#include "ArenaSurvivor.h"
#include "ArenaUpgradeSystem.h"
#include "ArenaWaveSpawner.h"
#include "UI/ArenaHUDWidget.h"
#include "UI/ArenaMainMenuWidget.h"
#include "UI/ArenaPauseWidget.h"
#include "UI/ArenaUpgradeWidget.h"
#include "UI/ArenaGameOverWidget.h"
#include "Blueprint/UserWidget.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

AArenaGameMode::AArenaGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AArenaPlayerCharacter::StaticClass();
	PlayerControllerClass = AArenaPlayerController::StaticClass();
	HUDClass = nullptr;
}

AArenaGameMode* AArenaGameMode::Get(const UObject* WorldContext)
{
	if (WorldContext == nullptr) return nullptr;
	UWorld* World = WorldContext->GetWorld();
	return World ? World->GetAuthGameMode<AArenaGameMode>() : nullptr;
}

void AArenaGameMode::BeginPlay()
{
	Super::BeginPlay();

	Assets = NewObject<UArenaAssets>(this, TEXT("ArenaAssets"));
	Assets->LoadAll();
	ArenaRadius = Assets->ArenaRadius;

	Spawner = NewObject<UArenaWaveSpawner>(this, TEXT("WaveSpawner"));
	Spawner->Init(this);
	Upgrades = NewObject<UArenaUpgradeSystem>(this, TEXT("UpgradeSystem"));
	Upgrades->Init(this);

	EnsurePlayer();
	EnsureArenaWorld();
	HighScore = UArenaSaveGame::LoadHighScore();
	CreateWidgets();
	SetState(EArenaState::Menu);
	UE_LOG(LogArena, Log, TEXT("[Arena] Game mode ready"));
}

void AArenaGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Player == nullptr) EnsurePlayer();
	if (Spawner != nullptr) Spawner->Tick(DeltaSeconds);
}

void AArenaGameMode::EnsurePlayer()
{
	if (Player != nullptr) return;
	Player = Cast<AArenaPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	if (Player == nullptr) return;
	Player->ApplyAssets(Assets);
	if (!bPlayerBound)
	{
		bPlayerBound = true;
		Player->Health->OnDied.AddUObject(this, &AArenaGameMode::OnPlayerDied);
	}
	Player->ResetForRun();
}

// ------------------------------------------------------------------ flow

void AArenaGameMode::StartRun()
{
	if (Assets == nullptr) return;
	Score = 0;
	Wave = 0;
	Level = 0;
	Xp = 0;
	bPendingLevelUpgrade = false;

	if (Spawner != nullptr) Spawner->StopWaves();
	ClearActors();
	if (Upgrades != nullptr) Upgrades->ResetRun();
	EnsurePlayer();
	if (Player == nullptr)
	{
		UE_LOG(LogArena, Error, TEXT("[Arena] No player pawn; cannot start"));
		return;
	}
	Player->ResetForRun();
	OnStatsChanged.Broadcast();

	SetState(EArenaState::Playing);
	NextWave();
}

void AArenaGameMode::ReturnToMenu()
{
	if (Spawner != nullptr) Spawner->StopWaves();
	ClearActors();
	if (Player != nullptr) Player->ResetForRun();
	SetState(EArenaState::Menu);
}

void AArenaGameMode::TogglePause()
{
	if (State == EArenaState::Playing) PauseGame();
	else if (State == EArenaState::Paused) Resume();
}

void AArenaGameMode::PauseGame()
{
	if (State != EArenaState::Playing) return;
	StateBeforePause = State;
	SetState(EArenaState::Paused);
}

void AArenaGameMode::Resume()
{
	if (State != EArenaState::Paused) return;
	SetState(StateBeforePause);
}

void AArenaGameMode::NextWave()
{
	if (Assets == nullptr) return;
	Wave++;
	OnStatsChanged.Broadcast();
	OnWaveStarted.Broadcast(Wave);
	PlaySfx(Assets->WaveStart, 0.7f);
	if (Spawner != nullptr) Spawner->StartWave(Wave);
}

void AArenaGameMode::OnWaveCleared()
{
	if (State != EArenaState::Playing || Player == nullptr) return;
	Player->Health->Heal(Player->Stats.MaxHp * 0.15f);
	OpenUpgradeChoice();
}

void AArenaGameMode::OnUpgradeChosen()
{
	if (State != EArenaState::Upgrading) return;
	SetState(EArenaState::Playing);
	bPendingLevelUpgrade = false;
	if (Spawner != nullptr && !Spawner->bWaveInProgress)
	{
		NextWave();
	}
}

void AArenaGameMode::OnPlayerDied()
{
	if (State == EArenaState::GameOver) return;
	if (Spawner != nullptr) Spawner->StopWaves();
	if (Score > HighScore)
	{
		HighScore = Score;
		UArenaSaveGame::SaveHighScore(HighScore);
	}
	if (Assets != nullptr) PlaySfx(Assets->GameOver);
	SetState(EArenaState::GameOver);
}

// ------------------------------------------------------------------ scoring

void AArenaGameMode::OnEnemyKilled(const FEnemyDef& Def, const FVector& Position)
{
	if (Assets == nullptr) return;
	Score += Def.ScoreValue;
	PlaySfx(Assets->EnemyDie, 0.6f);
	OnStatsChanged.Broadcast();

	UWorld* World = GetWorld();
	if (World == nullptr) return;
	const bool bDropHealth = FMath::FRand() < Def.DropHealthChance;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Loc(Position.X, Position.Y, 50.f);
	if (AArenaPickup* Pickup = World->SpawnActor<AArenaPickup>(AArenaPickup::StaticClass(), Loc, FRotator::ZeroRotator, Params))
	{
		Pickup->Init(bDropHealth ? EPickupType::Health : EPickupType::Xp, bDropHealth ? 25 : Def.XpValue, Assets);
	}
}

void AArenaGameMode::OnPickupCollected(EPickupType Type, int32 Value)
{
	if (Assets == nullptr || Player == nullptr) return;
	PlaySfx(Assets->Pickup, 0.6f);
	if (Type == EPickupType::Health)
	{
		Player->Health->Heal(Value);
		return;
	}
	Xp += Value;
	Score += Value;
	while (Xp >= XpToNext())
	{
		Xp -= XpToNext();
		Level++;
		bPendingLevelUpgrade = true;
	}
	OnStatsChanged.Broadcast();
	if (bPendingLevelUpgrade && State == EArenaState::Playing)
	{
		OpenUpgradeChoice();
	}
}

// ------------------------------------------------------------------ helpers

void AArenaGameMode::PlaySfx(USoundBase* Sound, float Volume) const
{
	if (Assets != nullptr) Assets->Play(this, Sound, Volume);
}

void AArenaGameMode::OpenUpgradeChoice()
{
	if (Upgrades == nullptr || Upgrades->Roll(3).Num() == 0)
	{
		bPendingLevelUpgrade = false;
		if (Spawner != nullptr && !Spawner->bWaveInProgress) NextWave();
		return;
	}
	if (Assets != nullptr) PlaySfx(Assets->Upgrade, 0.7f);
	SetState(EArenaState::Upgrading);
}

void AArenaGameMode::SetState(EArenaState Next)
{
	State = Next;
	UGameplayStatics::SetGamePaused(this, Next != EArenaState::Playing);
	OnStateChanged.Broadcast(Next);
}

void AArenaGameMode::ClearActors()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	for (TActorIterator<AArenaEnemy> It(World); It; ++It) It->Destroy();
	for (TActorIterator<AArenaPickup> It(World); It; ++It) It->Destroy();
	for (TActorIterator<AArenaProjectile> It(World); It; ++It) It->Destroy();
}

void AArenaGameMode::CreateWidgets()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC == nullptr) return;
	auto Add = [this, PC](TSubclassOf<UUserWidget> Class, int32 ZOrder)
	{
		UUserWidget* Widget = CreateWidget<UUserWidget>(PC, Class);
		if (Widget != nullptr)
		{
			Widget->AddToViewport(ZOrder);
			Widgets.Add(Widget);
		}
	};
	Add(UArenaHUDWidget::StaticClass(), 0);
	Add(UArenaUpgradeWidget::StaticClass(), 10);
	Add(UArenaPauseWidget::StaticClass(), 20);
	Add(UArenaGameOverWidget::StaticClass(), 30);
	Add(UArenaMainMenuWidget::StaticClass(), 40);
}

void AArenaGameMode::EnsureArenaWorld()
{
	UWorld* World = GetWorld();
	if (World == nullptr || Assets == nullptr) return;

	TArray<AActor*> Existing;
	UGameplayStatics::GetAllActorsWithTag(this, FName(TEXT("ArenaWorld")), Existing);
	if (Existing.Num() > 0) return;

	UE_LOG(LogArena, Warning, TEXT("[Arena] No ArenaWorld actors in this level; building the arena at runtime (run the Python bootstrap for the authored map)."));

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	auto Place = [&](UStaticMesh* Mesh, const FVector& Location, float Yaw, bool bCollide)
	{
		if (Mesh == nullptr) return;
		AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, FRotator(0.f, Yaw, 0.f), Params);
		if (Actor == nullptr) return;
		Actor->SetMobility(EComponentMobility::Movable);
		UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
		Component->SetStaticMesh(Mesh);
		if (Assets->PaletteMaterial != nullptr) Component->SetMaterial(0, Assets->PaletteMaterial);
		Component->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		Actor->Tags.Add(FName(TEXT("ArenaWorld")));
	};

	Place(Assets->FloorMesh, FVector::ZeroVector, 0.f, false);
	const int32 Pillars = 16;
	for (int32 i = 0; i < Pillars; i++)
	{
		const float A = i * 2.f * PI / Pillars;
		const float R = ArenaRadius - 250.f;
		Place(Assets->PillarMesh, FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f), -FMath::RadiansToDegrees(A), true);
	}
	FRandomStream Rng(42);
	for (int32 i = 0; i < 14; i++)
	{
		const float A = Rng.FRand() * 2.f * PI;
		const float D = 500.f + Rng.FRand() * 900.f;
		Place(Assets->CrateMesh, FVector(FMath::Cos(A) * D, FMath::Sin(A) * D, 0.f), Rng.FRand() * 90.f, true);
	}

	if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.f, 0.f, 500.f), FRotator(-55.f, -35.f, 0.f), Params))
	{
		Sun->SetMobility(EComponentMobility::Movable);
		if (ULightComponent* Light = Sun->GetLightComponent())
		{
			Light->SetIntensity(4.f);
			Light->SetLightColor(FLinearColor(1.f, 0.96f, 0.9f));
		}
		Sun->Tags.Add(FName(TEXT("ArenaWorld")));
	}
	if (ASkyLight* Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0.f, 0.f, 500.f), FRotator::ZeroRotator, Params))
	{
		if (USkyLightComponent* SkyComponent = Sky->GetLightComponent())
		{
			SkyComponent->SetMobility(EComponentMobility::Movable);
			SkyComponent->SetIntensity(1.f);
		}
		Sky->Tags.Add(FName(TEXT("ArenaWorld")));
	}
}
