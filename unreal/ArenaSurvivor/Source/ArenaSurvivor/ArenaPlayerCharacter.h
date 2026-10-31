#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArenaTypes.h"
#include "ArenaPlayerCharacter.generated.h"

class UCapsuleComponent;
class UStaticMeshComponent;
class UArenaHealthComponent;
class UArenaAssets;
class AArenaGameMode;

/**
 * Kinematic top-down pawn on the z = 90 plane: capsule root swept against props and
 * enemies, mesh child, no gravity, no character movement component.
 */
UCLASS()
class ARENASURVIVOR_API AArenaPlayerCharacter : public APawn
{
	GENERATED_BODY()

public:
	AArenaPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void ApplyAssets(UArenaAssets* InAssets);
	void ResetForRun();

	void SetMoveInput(FVector2D Value) { MoveInput = Value.GetClampedToMaxSize(1.f); }
	void SetAimStick(FVector2D Value) { AimStick = Value; }
	void SetFireHeld(bool bHeld) { bFireHeld = bHeld; }
	void SetMouseAim(const FVector& GroundPoint, bool bPointerMoved);

	float CapsuleRadius() const;
	float CapsuleHalfHeight() const;

	UPROPERTY() TObjectPtr<UCapsuleComponent> Capsule;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY() TObjectPtr<UArenaHealthComponent> Health;

	FArenaPlayerStats Stats;
	FVector AimDirection = FVector::ForwardVector;
	FVector AimPoint = FVector::ZeroVector;
	float TurnSpeed = 720.f;
	float FanAngle = 12.f;

private:
	void Fire(AArenaGameMode* Gm);
	void HandleDamaged(float Amount);

	UPROPERTY() TObjectPtr<UArenaAssets> Assets;

	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector2D AimStick = FVector2D::ZeroVector;
	bool bFireHeld = false;
	bool bUsingStick = false;
	float Cooldown = 0.f;
	float ArenaRadius = 2000.f;
};
