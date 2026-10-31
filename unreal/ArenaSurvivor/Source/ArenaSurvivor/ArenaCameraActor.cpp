#include "ArenaCameraActor.h"
#include "ArenaGameMode.h"
#include "ArenaPlayerCharacter.h"
#include "Camera/CameraComponent.h"

AArenaCameraActor::AArenaCameraActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
	SetTickableWhenPaused(true);
	if (UCameraComponent* Cam = GetCameraComponent())
	{
		Cam->SetFieldOfView(50.f);
		Cam->SetConstraintAspectRatio(false);
	}
}

void AArenaCameraActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AArenaGameMode* Gm = AArenaGameMode::Get(this);
	if (Gm == nullptr || Gm->Player == nullptr) return;

	const FVector Target = Gm->Player->GetActorLocation();
	const FVector Focus = Target + Gm->Player->AimDirection * AimLead;
	const FVector Desired = Focus + Offset;
	const FVector NewLoc = FMath::VInterpTo(GetActorLocation(), Desired, DeltaSeconds, InterpSpeed);
	SetActorLocation(NewLoc);
	SetActorRotation((Focus - NewLoc).Rotation());
}

void AArenaCameraActor::SnapTo(const FVector& Target)
{
	SetActorLocation(Target + Offset);
	SetActorRotation((Target - GetActorLocation()).Rotation());
}
