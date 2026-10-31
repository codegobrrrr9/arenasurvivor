#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArenaPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class AArenaCameraActor;
class AArenaPlayerCharacter;
struct FInputActionValue;

/**
 * Builds all Enhanced Input actions in code (no .inputactions asset), resolves the
 * mouse aim point on the ground plane, owns the chase camera and handles pause.
 */
UCLASS()
class ARENASURVIVOR_API AArenaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArenaPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	UPROPERTY() TObjectPtr<AArenaCameraActor> Camera;

private:
	void BuildInput();
	AArenaPlayerCharacter* ArenaPawn() const;
	bool MouseToGround(FVector& OutPoint, float PlaneZ) const;

	void OnMove(const FInputActionValue& Value);
	void OnMoveEnd(const FInputActionValue& Value);
	void OnAimStick(const FInputActionValue& Value);
	void OnAimStickEnd(const FInputActionValue& Value);
	void OnFireStart(const FInputActionValue& Value);
	void OnFireEnd(const FInputActionValue& Value);
	void OnPause(const FInputActionValue& Value);
	void OnConfirm(const FInputActionValue& Value);
	void OnPick1(const FInputActionValue& Value);
	void OnPick2(const FInputActionValue& Value);
	void OnPick3(const FInputActionValue& Value);
	void Pick(int32 Index);

	UPROPERTY() TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> AimStickAction;
	UPROPERTY() TObjectPtr<UInputAction> FireAction;
	UPROPERTY() TObjectPtr<UInputAction> PauseAction;
	UPROPERTY() TObjectPtr<UInputAction> ConfirmAction;
	UPROPERTY() TObjectPtr<UInputAction> Pick1Action;
	UPROPERTY() TObjectPtr<UInputAction> Pick2Action;
	UPROPERTY() TObjectPtr<UInputAction> Pick3Action;
};
