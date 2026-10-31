#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "ArenaCameraActor.generated.h"

/** Top-down chase camera with a little lead toward the aim point. Ticks even while paused. */
UCLASS()
class ARENASURVIVOR_API AArenaCameraActor : public ACameraActor
{
	GENERATED_BODY()

public:
	AArenaCameraActor(const FObjectInitializer& ObjectInitializer);

	virtual void Tick(float DeltaSeconds) override;
	void SnapTo(const FVector& Target);

	FVector Offset = FVector(-1100.f, 0.f, 1900.f);
	float AimLead = 250.f;
	float InterpSpeed = 7.f;
};
