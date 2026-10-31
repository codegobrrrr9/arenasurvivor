#pragma once

#include "CoreMinimal.h"
#include "ArenaTypes.generated.h"

// Custom object channels declared in Config/DefaultEngine.ini.
#define ECC_ArenaPlayer ECC_GameTraceChannel1
#define ECC_ArenaEnemy  ECC_GameTraceChannel2

UENUM()
enum class EArenaState : uint8
{
	Menu,
	Playing,
	Upgrading,
	Paused,
	GameOver
};

UENUM()
enum class EEnemyBehaviour : uint8
{
	Chaser,   // runs straight at the player
	Dasher,   // circles, then lunges
	Tank      // slow, heavy, shrugs off knockback
};

UENUM()
enum class EPickupType : uint8
{
	Xp,
	Health
};

UENUM()
enum class EArenaStat : uint8
{
	MoveSpeed,
	FireRate,
	Damage,
	MaxHp,
	ProjectileCount,
	ProjectileSpeed
};

/** Enemy archetype. Distances in centimetres, speeds in cm/s. */
struct FEnemyDef
{
	FName Id;
	FString DisplayName;
	EEnemyBehaviour Behaviour = EEnemyBehaviour::Chaser;
	float Hp = 30.f;
	float MoveSpeed = 360.f;
	float ContactDamage = 10.f;
	float ContactCooldown = 0.8f;
	float Mass = 1.f;
	float KnockbackResistance = 0.f;
	int32 XpValue = 3;
	int32 ScoreValue = 10;
	float DropHealthChance = 0.04f;
	int32 FirstWave = 1;
	float CapsuleRadius = 40.f;
	float CapsuleHalfHeight = 40.f;
};

struct FUpgradeDef
{
	FString Title;
	FString Description;
	EArenaStat Stat = EArenaStat::FireRate;
	float Value = 1.1f;
	bool bIsMultiplier = true;
	int32 MaxStacks = 8;
	bool bHealOnPick = false;
};

/** Runtime stat block for the player. Upgrades mutate it; ResetToBase restores defaults. */
struct FArenaPlayerStats
{
	float BaseMoveSpeed = 650.f;
	float BaseFireRate = 4.f;          // shots per second
	float BaseDamage = 10.f;
	float BaseMaxHp = 100.f;
	int32 BaseProjectileCount = 1;
	float BaseProjectileSpeed = 2400.f;

	float MoveSpeed = 650.f;
	float FireRate = 4.f;
	float Damage = 10.f;
	float MaxHp = 100.f;
	int32 ProjectileCount = 1;
	float ProjectileSpeed = 2400.f;

	void ResetToBase()
	{
		MoveSpeed = BaseMoveSpeed;
		FireRate = BaseFireRate;
		Damage = BaseDamage;
		MaxHp = BaseMaxHp;
		ProjectileCount = BaseProjectileCount;
		ProjectileSpeed = BaseProjectileSpeed;
	}

	void ApplyUpgrade(const FUpgradeDef& Def)
	{
		auto Apply = [&Def](float Current) { return Def.bIsMultiplier ? Current * Def.Value : Current + Def.Value; };
		switch (Def.Stat)
		{
		case EArenaStat::MoveSpeed: MoveSpeed = Apply(MoveSpeed); break;
		case EArenaStat::FireRate: FireRate = Apply(FireRate); break;
		case EArenaStat::Damage: Damage = Apply(Damage); break;
		case EArenaStat::MaxHp: MaxHp = Apply(MaxHp); break;
		case EArenaStat::ProjectileCount: ProjectileCount = FMath::RoundToInt(Apply((float)ProjectileCount)); break;
		case EArenaStat::ProjectileSpeed: ProjectileSpeed = Apply(ProjectileSpeed); break;
		}
	}
};
