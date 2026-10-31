#include "ArenaPlayerController.h"
#include "ArenaCameraActor.h"
#include "ArenaGameMode.h"
#include "ArenaPlayerCharacter.h"
#include "ArenaSurvivor.h"
#include "ArenaUpgradeSystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

AArenaPlayerController::AArenaPlayerController()
{
	bShowMouseCursor = true;
	bAutoManageActiveCameraTarget = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void AArenaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	SetInputMode(Mode);

	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Camera = World->SpawnActor<AArenaCameraActor>(AArenaCameraActor::StaticClass(), FVector(-1100.f, 0.f, 1900.f), FRotator::ZeroRotator, Params);
		if (Camera != nullptr)
		{
			Camera->SnapTo(FVector(0.f, 0.f, 90.f));
			SetViewTarget(Camera);
		}
	}
}

void AArenaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	BuildInput();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (EIC == nullptr)
	{
		UE_LOG(LogArena, Error, TEXT("[Arena] InputComponent is not an EnhancedInputComponent. Check Config/DefaultInput.ini."));
		return;
	}

	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArenaPlayerController::OnMove);
	EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AArenaPlayerController::OnMoveEnd);
	EIC->BindAction(AimStickAction, ETriggerEvent::Triggered, this, &AArenaPlayerController::OnAimStick);
	EIC->BindAction(AimStickAction, ETriggerEvent::Completed, this, &AArenaPlayerController::OnAimStickEnd);
	EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AArenaPlayerController::OnFireStart);
	EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AArenaPlayerController::OnFireEnd);
	EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AArenaPlayerController::OnPause);
	EIC->BindAction(ConfirmAction, ETriggerEvent::Started, this, &AArenaPlayerController::OnConfirm);
	EIC->BindAction(Pick1Action, ETriggerEvent::Started, this, &AArenaPlayerController::OnPick1);
	EIC->BindAction(Pick2Action, ETriggerEvent::Started, this, &AArenaPlayerController::OnPick2);
	EIC->BindAction(Pick3Action, ETriggerEvent::Started, this, &AArenaPlayerController::OnPick3);

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(Mapping, 0);
		}
	}
}

void AArenaPlayerController::BuildInput()
{
	Mapping = NewObject<UInputMappingContext>(this, TEXT("ArenaMapping"));

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type, bool bWhenPaused)
	{
		UInputAction* Action = NewObject<UInputAction>(this, Name);
		Action->ValueType = Type;
		Action->bTriggerWhenPaused = bWhenPaused;
		return Action;
	};
	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D, false);
	AimStickAction = MakeAction(TEXT("IA_AimStick"), EInputActionValueType::Axis2D, false);
	FireAction = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean, false);
	PauseAction = MakeAction(TEXT("IA_Pause"), EInputActionValueType::Boolean, true);
	ConfirmAction = MakeAction(TEXT("IA_Confirm"), EInputActionValueType::Boolean, true);
	Pick1Action = MakeAction(TEXT("IA_Pick1"), EInputActionValueType::Boolean, true);
	Pick2Action = MakeAction(TEXT("IA_Pick2"), EInputActionValueType::Boolean, true);
	Pick3Action = MakeAction(TEXT("IA_Pick3"), EInputActionValueType::Boolean, true);

	// Swizzle moves a 1D key onto the Y axis (forward); Negate flips it.
	auto Map = [this](UInputAction* Action, const FKey& Key, bool bNegate = false, bool bSwizzle = false, bool bDeadZone = false)
	{
		FEnhancedActionKeyMapping& M = Mapping->MapKey(Action, Key);
		if (bSwizzle) M.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		if (bNegate) M.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		if (bDeadZone) M.Modifiers.Add(NewObject<UInputModifierDeadZone>(this));
	};

	Map(MoveAction, EKeys::W, false, true);
	Map(MoveAction, EKeys::S, true, true);
	Map(MoveAction, EKeys::D);
	Map(MoveAction, EKeys::A, true);
	Map(MoveAction, EKeys::Up, false, true);
	Map(MoveAction, EKeys::Down, true, true);
	Map(MoveAction, EKeys::Right);
	Map(MoveAction, EKeys::Left, true);
	Map(MoveAction, EKeys::Gamepad_DPad_Up, false, true);
	Map(MoveAction, EKeys::Gamepad_DPad_Down, true, true);
	Map(MoveAction, EKeys::Gamepad_DPad_Right);
	Map(MoveAction, EKeys::Gamepad_DPad_Left, true);
	Map(MoveAction, EKeys::Gamepad_Left2D, false, false, true);

	Map(AimStickAction, EKeys::Gamepad_Right2D, false, false, true);

	Map(FireAction, EKeys::LeftMouseButton);
	Map(FireAction, EKeys::SpaceBar);
	Map(FireAction, EKeys::Gamepad_RightTrigger);
	Map(FireAction, EKeys::Gamepad_FaceButton_Bottom);

	Map(PauseAction, EKeys::Escape);
	Map(PauseAction, EKeys::P);
	Map(PauseAction, EKeys::Gamepad_Special_Right);

	Map(ConfirmAction, EKeys::Enter);
	Map(Pick1Action, EKeys::One);
	Map(Pick2Action, EKeys::Two);
	Map(Pick3Action, EKeys::Three);
}

AArenaPlayerCharacter* AArenaPlayerController::ArenaPawn() const
{
	return Cast<AArenaPlayerCharacter>(GetPawn());
}

bool AArenaPlayerController::MouseToGround(FVector& OutPoint, float PlaneZ) const
{
	FVector Origin, Direction;
	if (!DeprojectMousePositionToWorld(Origin, Direction)) return false;
	if (FMath::IsNearlyZero(Direction.Z)) return false;
	const FPlane Plane(FVector(0.f, 0.f, PlaneZ), FVector::UpVector);
	OutPoint = FMath::LinePlaneIntersection(Origin, Origin + Direction * 100000.f, Plane);
	return true;
}

void AArenaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	AArenaPlayerCharacter* P = ArenaPawn();
	if (P == nullptr) return;

	float DX = 0.f, DY = 0.f;
	GetInputMouseDelta(DX, DY);
	const bool bMoved = FMath::Abs(DX) + FMath::Abs(DY) > 0.01f;

	FVector Point;
	if (MouseToGround(Point, P->GetActorLocation().Z))
	{
		P->SetMouseAim(Point, bMoved);
	}
}

void AArenaPlayerController::OnMove(const FInputActionValue& Value)
{
	if (AArenaPlayerCharacter* P = ArenaPawn()) P->SetMoveInput(Value.Get<FVector2D>());
}

void AArenaPlayerController::OnMoveEnd(const FInputActionValue& Value)
{
	if (AArenaPlayerCharacter* P = ArenaPawn()) P->SetMoveInput(FVector2D::ZeroVector);
}

void AArenaPlayerController::OnAimStick(const FInputActionValue& Value)
{
	if (AArenaPlayerCharacter* P = ArenaPawn()) P->SetAimStick(Value.Get<FVector2D>());
}

void AArenaPlayerController::OnAimStickEnd(const FInputActionValue& Value)
{
	if (AArenaPlayerCharacter* P = ArenaPawn()) P->SetAimStick(FVector2D::ZeroVector);
}

void AArenaPlayerController::OnFireStart(const FInputActionValue& Value)
{
	if (AArenaPlayerCharacter* P = ArenaPawn()) P->SetFireHeld(true);
}

void AArenaPlayerController::OnFireEnd(const FInputActionValue& Value)
{
	if (AArenaPlayerCharacter* P = ArenaPawn()) P->SetFireHeld(false);
}

void AArenaPlayerController::OnPause(const FInputActionValue& Value)
{
	if (AArenaGameMode* Gm = AArenaGameMode::Get(this)) Gm->TogglePause();
}

void AArenaPlayerController::OnConfirm(const FInputActionValue& Value)
{
	AArenaGameMode* Gm = AArenaGameMode::Get(this);
	if (Gm == nullptr) return;
	if (Gm->State == EArenaState::Menu || Gm->State == EArenaState::GameOver)
	{
		Gm->StartRun();
	}
}

void AArenaPlayerController::OnPick1(const FInputActionValue& Value) { Pick(0); }
void AArenaPlayerController::OnPick2(const FInputActionValue& Value) { Pick(1); }
void AArenaPlayerController::OnPick3(const FInputActionValue& Value) { Pick(2); }

void AArenaPlayerController::Pick(int32 Index)
{
	AArenaGameMode* Gm = AArenaGameMode::Get(this);
	if (Gm == nullptr || Gm->State != EArenaState::Upgrading || Gm->Upgrades == nullptr) return;
	Gm->Upgrades->Choose(Index);
}
