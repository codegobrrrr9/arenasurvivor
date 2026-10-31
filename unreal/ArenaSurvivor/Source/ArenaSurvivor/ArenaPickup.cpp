#include "ArenaPickup.h"
#include "ArenaAssets.h"
#include "ArenaGameMode.h"
#include "ArenaPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AArenaPickup::AArenaPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

void AArenaPickup::Init(EPickupType InType, int32 InValue, UArenaAssets* Assets)
{
	Type = InType;
	Value = InValue;
	SpawnTime = GetWorld()->GetTimeSeconds();
	BobOffset = FMath::FRandRange(0.f, 10.f);
	bCollected = false;
	if (Assets != nullptr)
	{
		Assets->ApplyMesh(Mesh, Type == EPickupType::Health ? Assets->HealthMesh : Assets->XpMesh, 0.f);
	}
}

void AArenaPickup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCollected) return;

	AArenaGameMode* Gm = AArenaGameMode::Get(this);
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - SpawnTime > Lifetime)
	{
		Destroy();
		return;
	}

	FVector P = GetActorLocation();
	P.Z = 50.f + FMath::Sin((Now + BobOffset) * 3.f) * 15.f;
	SetActorLocation(P);
	AddActorWorldRotation(FRotator(0.f, 120.f * DeltaSeconds, 0.f));

	if (Gm == nullptr || Gm->Player == nullptr || Gm->State != EArenaState::Playing) return;

	const FVector Target = Gm->Player->GetActorLocation();
	FVector To = Target - GetActorLocation();
	To.Z = 0.f;
	const float Dist = To.Size();
	if (Dist < MagnetRadius)
	{
		const float Boost = FMath::Lerp(2.f, 1.f, Dist / MagnetRadius);
		FVector Step = To.GetSafeNormal() * MagnetSpeed * DeltaSeconds * Boost;
		if (Step.Size() > Dist) Step = To;
		AddActorWorldOffset(Step);
		if (Dist < 60.f + Gm->Player->CapsuleRadius())
		{
			bCollected = true;
			Gm->OnPickupCollected(Type, Value);
			Destroy();
		}
	}
}
