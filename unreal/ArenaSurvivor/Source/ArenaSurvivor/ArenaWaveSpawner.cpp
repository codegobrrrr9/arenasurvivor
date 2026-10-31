#include "ArenaWaveSpawner.h"
#include "ArenaAssets.h"
#include "ArenaEnemy.h"
#include "ArenaGameMode.h"
#include "ArenaMovement.h"
#include "ArenaSurvivor.h"
#include "Engine/World.h"

void UArenaWaveSpawner::Init(AArenaGameMode* InOwner)
{
	Owner = InOwner;
}

void UArenaWaveSpawner::StartWave(int32 InWave)
{
	if (Owner == nullptr || Owner->Assets == nullptr) return;
	Wave = InWave;
	Queue.Reset();

	const UArenaAssets* Assets = Owner->Assets;
	const FEnemyDef* Blob = Assets->FindEnemy(FName(TEXT("blob")));
	const FEnemyDef* Spiky = Assets->FindEnemy(FName(TEXT("spiky")));
	const FEnemyDef* Tank = Assets->FindEnemy(FName(TEXT("tank")));

	const float HpMul = 1.f + 0.12f * (Wave - 1);
	const int32 Blobs = 6 + Wave * 3;
	const int32 Dashers = Wave >= 3 ? 2 + Wave : 0;
	const int32 Tanks = Wave >= 5 ? 1 + (Wave - 5) / 2 : 0;

	TArray<FQueued> List;
	if (Blob) for (int32 i = 0; i < Blobs; i++) List.Add({Blob, 1.f, HpMul});
	if (Spiky) for (int32 i = 0; i < Dashers; i++) List.Add({Spiky, 1.f, HpMul});
	if (Tank) for (int32 i = 0; i < Tanks; i++) List.Add({Tank, 1.f, HpMul});
	for (int32 i = List.Num() - 1; i > 0; i--)
	{
		List.Swap(i, FMath::RandRange(0, i));
	}
	if (Tank && Wave % 5 == 0) List.Add({Tank, 1.8f, HpMul * 4.f});   // boss last

	Queue = MoveTemp(List);
	SpawnInterval = FMath::Max(0.25f, 1.2f - 0.05f * Wave);
	NextSpawn = Owner->GetWorld()->GetTimeSeconds() + 0.5f;
	bWaveInProgress = true;
}

void UArenaWaveSpawner::StopWaves()
{
	Queue.Reset();
	bWaveInProgress = false;
	for (int32 i = Enemies.Num() - 1; i >= 0; i--)
	{
		if (Enemies[i] != nullptr) Enemies[i]->Destroy();
	}
	Enemies.Reset();
}

void UArenaWaveSpawner::OnEnemyRemoved(AArenaEnemy* Enemy)
{
	Enemies.Remove(Enemy);
	CheckCleared();
}

void UArenaWaveSpawner::Tick(float DeltaSeconds)
{
	if (Owner == nullptr || Owner->State != EArenaState::Playing) return;
	Enemies.RemoveAll([](const TObjectPtr<AArenaEnemy>& E) { return E == nullptr || !IsValid(E); });
	SeparateEnemies(DeltaSeconds);

	if (!bWaveInProgress) return;
	if (Queue.Num() == 0)
	{
		CheckCleared();
		return;
	}
	const float Now = Owner->GetWorld()->GetTimeSeconds();
	if (Now < NextSpawn) return;
	NextSpawn = Now + SpawnInterval;
	const FQueued Entry = Queue[0];
	Queue.RemoveAt(0);
	Spawn(Entry);
}

void UArenaWaveSpawner::Spawn(const FQueued& Entry)
{
	UWorld* World = Owner->GetWorld();
	if (World == nullptr || Entry.Def == nullptr) return;

	const float Angle = FMath::FRandRange(0.f, 2.f * PI);
	const float R = Owner->ArenaRadius - SpawnRadiusPadding;
	const FVector Pos(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R, Entry.Def->CapsuleHalfHeight * Entry.Scale);
	const FRotator Rot = (-Pos).GetSafeNormal2D().Rotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArenaEnemy* Enemy = World->SpawnActor<AArenaEnemy>(AArenaEnemy::StaticClass(), Pos, Rot, Params);
	if (Enemy == nullptr)
	{
		UE_LOG(LogArena, Warning, TEXT("[Arena] Failed to spawn enemy %s"), *Entry.Def->Id.ToString());
		return;
	}
	Enemy->Init(Entry.Def, Entry.HpMul, Entry.Scale, Owner->Assets);
	Enemies.Add(Enemy);
}

void UArenaWaveSpawner::CheckCleared()
{
	if (!bWaveInProgress || Queue.Num() > 0 || Enemies.Num() > 0) return;
	bWaveInProgress = false;
	if (Owner != nullptr) Owner->OnWaveCleared();
}

void UArenaWaveSpawner::SeparateEnemies(float DeltaSeconds)
{
	const int32 N = Enemies.Num();
	for (int32 i = 0; i < N; i++)
	{
		AArenaEnemy* A = Enemies[i];
		if (A == nullptr || A->IsDead()) continue;
		for (int32 j = i + 1; j < N; j++)
		{
			AArenaEnemy* B = Enemies[j];
			if (B == nullptr || B->IsDead()) continue;
			FVector Delta = B->GetActorLocation() - A->GetActorLocation();
			Delta.Z = 0.f;
			const float Dist = Delta.Size();
			const float MinDist = A->Radius() + B->Radius();
			if (Dist >= MinDist || Dist < 1.f) continue;
			const FVector Dir = Delta / Dist;
			const float Push = (MinDist - Dist);
			const float Total = A->Mass() + B->Mass();
			ArenaMovement::MoveFlat(A, -Dir * Push * (B->Mass() / Total));
			ArenaMovement::MoveFlat(B, Dir * Push * (A->Mass() / Total));
		}
	}
}
