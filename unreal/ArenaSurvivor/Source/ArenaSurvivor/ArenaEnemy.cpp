#include "ArenaEnemy.h"
#include "ArenaAssets.h"
#include "ArenaGameMode.h"
#include "ArenaHealthComponent.h"
#include "ArenaMovement.h"
#include "ArenaPlayerCharacter.h"
#include "ArenaWaveSpawner.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AArenaEnemy::AArenaEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(40.f, 40.f);
	Capsule->SetCollisionProfileName(TEXT("ArenaEnemy"));
	Capsule->SetCanEverAffectNavigation(false);
	SetRootComponent(Capsule);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Capsule);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Health = CreateDefaultSubobject<UArenaHealthComponent>(TEXT("Health"));
}

void AArenaEnemy::Init(const FEnemyDef* InDef, float HpMultiplier, float Scale, UArenaAssets* Assets)
{
	Def = InDef;
	if (Def == nullptr) return;

	Capsule->SetCapsuleSize(Def->CapsuleRadius, Def->CapsuleHalfHeight);
	SetActorScale3D(FVector(Scale));
	if (Assets != nullptr)
	{
		Assets->ApplyMesh(Mesh, Assets->EnemyMesh(Def->Id), -Def->CapsuleHalfHeight);
		ArenaRadius = Assets->ArenaRadius;
	}
	Health->Configure(Def->Hp * HpMultiplier);
	if (!bBound)
	{
		bBound = true;
		Health->OnDamaged.AddUObject(this, &AArenaEnemy::HandleDamaged);
		Health->OnDied.AddUObject(this, &AArenaEnemy::HandleDied);
	}

	bDead = false;
	bDashing = false;
	DashTimer = FMath::FRandRange(0.5f, 1.5f);
	OrbitSign = FMath::RandBool() ? -1 : 1;
	Knockback = FVector::ZeroVector;
	NextContactTime = GetWorld()->GetTimeSeconds() + 0.3f;
}

float AArenaEnemy::Radius() const
{
	return Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;
}

void AArenaEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bDead || Def == nullptr) return;

	AArenaGameMode* Gm = AArenaGameMode::Get(this);
	if (Gm == nullptr || Gm->State != EArenaState::Playing || Gm->Player == nullptr) return;
	AArenaPlayerCharacter* Player = Gm->Player;

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	const float Dist = ToPlayer.Size();
	const FVector Dir = Dist > 1.f ? ToPlayer / Dist : FVector::ForwardVector;

	FVector Desired;
	switch (Def->Behaviour)
	{
	case EEnemyBehaviour::Dasher:
		Desired = DasherVelocity(Dir, Dist, DeltaSeconds);
		break;
	default:
		Desired = Dir * Def->MoveSpeed;
		break;
	}

	Knockback = FMath::VInterpConstantTo(Knockback, FVector::ZeroVector, DeltaSeconds, 3000.f);
	ArenaMovement::MoveFlat(this, (Desired + Knockback) * DeltaSeconds);

	if (Desired.SizeSquared() > 1.f)
	{
		SetActorRotation(FRotator(0.f, Desired.Rotation().Yaw, 0.f));
	}

	const FVector Clamped = ArenaMovement::ClampToArena(GetActorLocation(), ArenaRadius + 100.f);
	if (!Clamped.Equals(GetActorLocation()))
	{
		SetActorLocation(Clamped);
	}

	TryContactDamage(Player, GetWorld()->GetTimeSeconds());
}

FVector AArenaEnemy::DasherVelocity(const FVector& Dir, float Dist, float DeltaSeconds)
{
	DashTimer -= DeltaSeconds;
	if (bDashing)
	{
		if (DashTimer <= 0.f)
		{
			bDashing = false;
			DashTimer = FMath::FRandRange(1.2f, 2.0f);
		}
		return DashDir * Def->MoveSpeed * 3.2f;
	}
	if (DashTimer <= 0.f && Dist < 900.f)
	{
		bDashing = true;
		DashDir = Dir;
		DashTimer = 0.35f;
		return DashDir * Def->MoveSpeed * 3.2f;
	}
	// Orbit at ~5 m while slowly closing in.
	const FVector Tangent = FVector::CrossProduct(FVector::UpVector, Dir) * OrbitSign;
	const float Approach = FMath::Clamp((Dist - 500.f) * 0.005f, -1.f, 1.f);
	return (Tangent * 0.8f + Dir * Approach).GetSafeNormal() * Def->MoveSpeed;
}

void AArenaEnemy::TryContactDamage(AArenaPlayerCharacter* Player, float Now)
{
	if (Now < NextContactTime) return;
	const float Dist = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());
	if (Dist > Radius() + Player->CapsuleRadius() + 8.f) return;

	NextContactTime = Now + Def->ContactCooldown;
	FVector Dir = Player->GetActorLocation() - GetActorLocation();
	Dir.Z = 0.f;
	Dir = Dir.GetSafeNormal();
	Player->Health->ApplyArenaDamage(Def->ContactDamage, Dir);
	// Small self-knockback so chasers do not stack inside the player.
	Knockback = -Dir * 300.f * (1.f - Def->KnockbackResistance);
}

void AArenaEnemy::HandleDamaged(float Amount)
{
	AArenaGameMode* Gm = AArenaGameMode::Get(this);
	if (Gm == nullptr || Gm->Player == nullptr || Def == nullptr) return;
	FVector Away = GetActorLocation() - Gm->Player->GetActorLocation();
	Away.Z = 0.f;
	Knockback += Away.GetSafeNormal() * 400.f * (1.f - Def->KnockbackResistance);
}

void AArenaEnemy::HandleDied()
{
	if (bDead) return;
	bDead = true;
	if (AArenaGameMode* Gm = AArenaGameMode::Get(this))
	{
		if (Def != nullptr) Gm->OnEnemyKilled(*Def, GetActorLocation());
		if (Gm->Spawner != nullptr) Gm->Spawner->OnEnemyRemoved(this);
	}
	Destroy();
}
