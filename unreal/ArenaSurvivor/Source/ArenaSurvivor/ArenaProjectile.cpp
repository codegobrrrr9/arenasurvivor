#include "ArenaProjectile.h"
#include "ArenaAssets.h"
#include "ArenaGameMode.h"
#include "ArenaHealthComponent.h"
#include "ArenaTypes.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

AArenaProjectile::AArenaProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
}

void AArenaProjectile::Launch(const FVector& InDirection, float InDamage, float InSpeed, float InArenaRadius, AActor* InShooter, UArenaAssets* Assets)
{
	Direction = InDirection.GetSafeNormal2D();
	if (Direction.IsNearlyZero()) Direction = FVector::ForwardVector;
	Damage = InDamage;
	Speed = InSpeed;
	ArenaRadius = InArenaRadius + 200.f;
	Shooter = InShooter;
	DieAt = GetWorld()->GetTimeSeconds() + Lifetime;
	SetActorRotation(Direction.Rotation());
	if (Assets != nullptr)
	{
		Assets->ApplyMesh(Mesh, Assets->ProjectileMesh, 0.f);
	}
}

void AArenaProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Step = Speed * DeltaSeconds;
	const FVector Start = GetActorLocation();
	const FVector End = Start + Direction * Step;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_ArenaEnemy);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ArenaProjectile), false, this);
	if (Shooter != nullptr) QueryParams.AddIgnoredActor(Shooter);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Start, End, ObjectParams, QueryParams))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (UArenaHealthComponent* Health = HitActor->FindComponentByClass<UArenaHealthComponent>())
			{
				Health->ApplyArenaDamage(Damage, Direction);
				if (AArenaGameMode* Gm = AArenaGameMode::Get(this))
				{
					Gm->PlaySfx(Gm->Assets ? Gm->Assets->Hit : nullptr, 0.4f);
				}
			}
		}
		Destroy();
		return;
	}

	SetActorLocation(End);
	if (GetWorld()->GetTimeSeconds() >= DieAt || FVector2D(End.X, End.Y).Size() > ArenaRadius)
	{
		Destroy();
	}
}
