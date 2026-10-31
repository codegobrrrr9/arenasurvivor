#include "ArenaPlayerCharacter.h"
#include "ArenaAssets.h"
#include "ArenaCameraActor.h"
#include "ArenaGameMode.h"
#include "ArenaHealthComponent.h"
#include "ArenaMovement.h"
#include "ArenaPlayerController.h"
#include "ArenaProjectile.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AArenaPlayerCharacter::AArenaPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::Disabled;

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->InitCapsuleSize(40.f, 90.f);
	Capsule->SetCollisionProfileName(TEXT("ArenaPlayer"));
	Capsule->SetCanEverAffectNavigation(false);
	SetRootComponent(Capsule);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Capsule);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeLocation(FVector(0.f, 0.f, -90.f));

	Health = CreateDefaultSubobject<UArenaHealthComponent>(TEXT("Health"));
	Health->InvulnerabilityAfterHit = 0.25f;

	Stats.ResetToBase();
}

void AArenaPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health->OnDamaged.AddUObject(this, &AArenaPlayerCharacter::HandleDamaged);
	Health->Configure(Stats.MaxHp);
}

void AArenaPlayerCharacter::ApplyAssets(UArenaAssets* InAssets)
{
	Assets = InAssets;
	if (Assets == nullptr) return;
	Assets->ApplyMesh(Mesh, Assets->PlayerMesh, -CapsuleHalfHeight());
	ArenaRadius = Assets->ArenaRadius;
}

void AArenaPlayerCharacter::ResetForRun()
{
	Stats.ResetToBase();
	Health->Configure(Stats.MaxHp);
	Cooldown = 0.f;
	bFireHeld = false;
	MoveInput = FVector2D::ZeroVector;
	AimDirection = FVector::ForwardVector;
	const FVector Start(0.f, 0.f, CapsuleHalfHeight());
	SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator::ZeroRotator);
	if (AArenaPlayerController* PC = Cast<AArenaPlayerController>(GetController()))
	{
		if (PC->Camera != nullptr) PC->Camera->SnapTo(Start);
	}
}

float AArenaPlayerCharacter::CapsuleRadius() const
{
	return Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;
}

float AArenaPlayerCharacter::CapsuleHalfHeight() const
{
	return Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f;
}

void AArenaPlayerCharacter::SetMouseAim(const FVector& GroundPoint, bool bPointerMoved)
{
	if (bPointerMoved) bUsingStick = false;
	if (bUsingStick) return;
	FVector Dir = GroundPoint - GetActorLocation();
	Dir.Z = 0.f;
	if (Dir.SizeSquared() > 25.f)
	{
		AimDirection = Dir.GetSafeNormal();
		AimPoint = GroundPoint;
	}
}

void AArenaPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AArenaGameMode* Gm = AArenaGameMode::Get(this);
	if (Gm == nullptr || Gm->State != EArenaState::Playing) return;

	if (AimStick.SizeSquared() > 0.1f)
	{
		bUsingStick = true;
		AimDirection = FVector(AimStick.Y, AimStick.X, 0.f).GetSafeNormal();
		AimPoint = GetActorLocation() + AimDirection * 600.f;
	}

	const FRotator TargetRot(0.f, AimDirection.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), TargetRot, DeltaSeconds, TurnSpeed));

	// Screen up is world +X (the camera sits at -X looking +X); screen right is world +Y.
	const FVector Velocity = FVector(MoveInput.Y, MoveInput.X, 0.f) * Stats.MoveSpeed;
	ArenaMovement::MoveFlat(this, Velocity * DeltaSeconds);
	const FVector Clamped = ArenaMovement::ClampToArena(GetActorLocation(), ArenaRadius);
	if (!Clamped.Equals(GetActorLocation()))
	{
		SetActorLocation(Clamped);
	}

	Cooldown -= DeltaSeconds;
	if (bFireHeld && Cooldown <= 0.f)
	{
		Cooldown = 1.f / FMath::Max(0.1f, Stats.FireRate);
		Fire(Gm);
	}
}

void AArenaPlayerCharacter::Fire(AArenaGameMode* Gm)
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;

	const FVector Origin = GetActorLocation() + AimDirection * 60.f + FVector(0.f, 0.f, 10.f);
	const int32 Count = FMath::Max(1, Stats.ProjectileCount);
	const float Start = -FanAngle * (Count - 1) * 0.5f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	for (int32 i = 0; i < Count; i++)
	{
		const FVector Dir = FRotator(0.f, Start + FanAngle * i, 0.f).RotateVector(AimDirection);
		AArenaProjectile* P = World->SpawnActor<AArenaProjectile>(AArenaProjectile::StaticClass(), Origin, Dir.Rotation(), Params);
		if (P != nullptr)
		{
			P->Launch(Dir, Stats.Damage, Stats.ProjectileSpeed, ArenaRadius, this, Assets);
		}
	}
	Gm->PlaySfx(Assets ? Assets->Shoot : nullptr, 0.35f);
}

void AArenaPlayerCharacter::HandleDamaged(float Amount)
{
	if (AArenaGameMode* Gm = AArenaGameMode::Get(this))
	{
		Gm->PlaySfx(Assets ? Assets->PlayerHurt : nullptr, 0.8f);
	}
}
