#include "ArenaAssets.h"
#include "ArenaSurvivor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

template <typename T>
T* UArenaAssets::LoadByPath(const TCHAR* Path) const
{
	T* Asset = LoadObject<T>(nullptr, Path);
	if (Asset == nullptr)
	{
		UE_LOG(LogArena, Warning, TEXT("[Arena] Missing asset %s (run the Python bootstrap: see README)"), Path);
	}
	return Asset;
}

void UArenaAssets::LoadAll()
{
	PlayerMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/player.player"));
	BlobMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/enemy_blob.enemy_blob"));
	SpikyMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/enemy_spiky.enemy_spiky"));
	TankMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/enemy_tank.enemy_tank"));
	ProjectileMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/projectile.projectile"));
	XpMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/pickup_xp.pickup_xp"));
	HealthMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/pickup_health.pickup_health"));
	FloorMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/arena_floor.arena_floor"));
	PillarMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/pillar.pillar"));
	CrateMesh = LoadByPath<UStaticMesh>(TEXT("/Game/Models/crate.crate"));

	PaletteMaterial = LoadByPath<UMaterialInterface>(TEXT("/Game/Materials/M_Palette.M_Palette"));

	Shoot = LoadByPath<USoundBase>(TEXT("/Game/Audio/shoot.shoot"));
	Hit = LoadByPath<USoundBase>(TEXT("/Game/Audio/hit.hit"));
	EnemyDie = LoadByPath<USoundBase>(TEXT("/Game/Audio/enemy_die.enemy_die"));
	Pickup = LoadByPath<USoundBase>(TEXT("/Game/Audio/pickup.pickup"));
	Upgrade = LoadByPath<USoundBase>(TEXT("/Game/Audio/upgrade.upgrade"));
	PlayerHurt = LoadByPath<USoundBase>(TEXT("/Game/Audio/player_hurt.player_hurt"));
	GameOver = LoadByPath<USoundBase>(TEXT("/Game/Audio/game_over.game_over"));
	UiClick = LoadByPath<USoundBase>(TEXT("/Game/Audio/ui_click.ui_click"));
	WaveStart = LoadByPath<USoundBase>(TEXT("/Game/Audio/wave_start.wave_start"));

	BuildTables();
	MeshYaw = DetectMeshYaw();
	UE_LOG(LogArena, Log, TEXT("[Arena] Assets loaded. Mesh yaw correction: %.0f"), MeshYaw);
}

void UArenaAssets::BuildTables()
{
	Enemies.Reset();
	Upgrades.Reset();

	auto AddEnemy = [this](const TCHAR* Id, const TCHAR* Display, EEnemyBehaviour Behaviour, float Hp, float Speed, float Damage,
		float Mass, float Resist, int32 Xp, int32 Score, float Heal, int32 FirstWave, float Radius, float HalfHeight)
	{
		FEnemyDef D;
		D.Id = FName(Id);
		D.DisplayName = Display;
		D.Behaviour = Behaviour;
		D.Hp = Hp;
		D.MoveSpeed = Speed;
		D.ContactDamage = Damage;
		D.Mass = Mass;
		D.KnockbackResistance = Resist;
		D.XpValue = Xp;
		D.ScoreValue = Score;
		D.DropHealthChance = Heal;
		D.FirstWave = FirstWave;
		D.CapsuleRadius = Radius;
		D.CapsuleHalfHeight = HalfHeight;
		Enemies.Add(D);
	};
	AddEnemy(TEXT("blob"), TEXT("Blob"), EEnemyBehaviour::Chaser, 30.f, 360.f, 10.f, 1.f, 0.f, 3, 10, 0.04f, 1, 40.f, 40.f);
	AddEnemy(TEXT("spiky"), TEXT("Spiky"), EEnemyBehaviour::Dasher, 45.f, 420.f, 15.f, 1.2f, 0.2f, 5, 25, 0.06f, 3, 50.f, 50.f);
	AddEnemy(TEXT("tank"), TEXT("Tank"), EEnemyBehaviour::Tank, 220.f, 180.f, 25.f, 12.f, 0.9f, 12, 80, 0.25f, 5, 100.f, 100.f);

	auto AddUpgrade = [this](const TCHAR* Title, const TCHAR* Description, EArenaStat Stat, float Value, bool bMultiplier, int32 MaxStacks, bool bHeal)
	{
		FUpgradeDef U;
		U.Title = Title;
		U.Description = Description;
		U.Stat = Stat;
		U.Value = Value;
		U.bIsMultiplier = bMultiplier;
		U.MaxStacks = MaxStacks;
		U.bHealOnPick = bHeal;
		Upgrades.Add(U);
	};
	AddUpgrade(TEXT("Rapid Fire"), TEXT("+15% fire rate"), EArenaStat::FireRate, 1.15f, true, 8, false);
	AddUpgrade(TEXT("Heavy Rounds"), TEXT("+20% damage"), EArenaStat::Damage, 1.2f, true, 8, false);
	AddUpgrade(TEXT("Swift Boots"), TEXT("+10% move speed"), EArenaStat::MoveSpeed, 1.1f, true, 6, false);
	AddUpgrade(TEXT("Vitality"), TEXT("+20 max HP and heal 25%"), EArenaStat::MaxHp, 20.f, false, 8, true);
	AddUpgrade(TEXT("Multishot"), TEXT("+1 projectile per shot"), EArenaStat::ProjectileCount, 1.f, false, 4, false);
	AddUpgrade(TEXT("Velocity"), TEXT("+20% projectile speed"), EArenaStat::ProjectileSpeed, 1.2f, true, 4, false);
}

const FEnemyDef* UArenaAssets::FindEnemy(FName Id) const
{
	for (const FEnemyDef& D : Enemies)
	{
		if (D.Id == Id) return &D;
	}
	return nullptr;
}

UStaticMesh* UArenaAssets::EnemyMesh(FName Id) const
{
	if (Id == FName(TEXT("blob"))) return BlobMesh;
	if (Id == FName(TEXT("spiky"))) return SpikyMesh;
	if (Id == FName(TEXT("tank"))) return TankMesh;
	return nullptr;
}

float UArenaAssets::DetectMeshYaw() const
{
	// The player's gun extends further in front of the body than anything behind it, so the
	// bounding box is asymmetric along the model's front axis. Use that to find which world
	// axis the importer put the front on, then rotate every mesh so the front is +X.
	if (PlayerMesh == nullptr) return 0.f;
	const FBox Box = PlayerMesh->GetBoundingBox();
	const float AsymX = Box.Max.X + Box.Min.X;
	const float AsymY = Box.Max.Y + Box.Min.Y;
	float Yaw = 0.f;
	if (FMath::Abs(AsymX) >= FMath::Abs(AsymY))
	{
		Yaw = AsymX >= 0.f ? 0.f : 180.f;
	}
	else
	{
		Yaw = AsymY > 0.f ? -90.f : 90.f;
	}
	if (Yaw != 0.f)
	{
		UE_LOG(LogArena, Log, TEXT("[Arena] Player mesh front is not +X (asymmetry x=%.1f y=%.1f); applying yaw %.0f to all meshes"), AsymX, AsymY, Yaw);
	}
	return Yaw;
}

void UArenaAssets::ApplyMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh, float ZOffset) const
{
	if (Component == nullptr) return;
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCollisionProfileName(TEXT("NoCollision"));
	if (Mesh != nullptr)
	{
		Component->SetStaticMesh(Mesh);
	}
	if (PaletteMaterial != nullptr)
	{
		const int32 Count = FMath::Max(1, Component->GetNumMaterials());
		for (int32 i = 0; i < Count; i++)
		{
			Component->SetMaterial(i, PaletteMaterial);
		}
	}
	Component->SetRelativeLocation(FVector(0.f, 0.f, ZOffset));
	Component->SetRelativeRotation(FRotator(0.f, MeshYaw, 0.f));
}

void UArenaAssets::Play(const UObject* WorldContext, USoundBase* Sound, float Volume) const
{
	if (Sound == nullptr || WorldContext == nullptr) return;
	const float Pitch = FMath::FRandRange(0.95f, 1.05f);
	UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume * MasterVolume, Pitch, 0.f, nullptr, nullptr, true);
}
