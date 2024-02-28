// Copyright Epic Games, Inc. All Rights Reserved.

#include "BasicallyZeldaCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ABasicallyZeldaCharacter

ABasicallyZeldaCharacter::ABasicallyZeldaCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ABasicallyZeldaCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ABasicallyZeldaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ABasicallyZeldaCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABasicallyZeldaCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABasicallyZeldaCharacter::Look);

		// Shooting
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Triggered, this, &ABasicallyZeldaCharacter::ShootBow);

		// Exploding Gamer
		EnhancedInputComponent->BindAction(ExplodeAction, ETriggerEvent::Triggered, this, &ABasicallyZeldaCharacter::ExplodeAttack);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ABasicallyZeldaCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ABasicallyZeldaCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}



void ABasicallyZeldaCharacter::Shoot()
{
	FHitResult* Hit = new FHitResult();

	// get start position
	FVector Start = GetActorLocation();

	// get end position
	FVector End = Start + GetFollowCamera()->GetForwardVector() * ShotRange;

	// avoid shooting self
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	// "shoot"
	bool bHit = GetWorld()->LineTraceSingleByChannel(*Hit, Start, End, ECC_Visibility, Params);
	DrawDebugLine(GetWorld(), Start, Hit->TraceEnd, FColor(255, 0, 0), false, 5.f);

	// richochet shot
	if (bHit)
	{
		RicochetShot(Hit);
	}
}

void ABasicallyZeldaCharacter::RicochetShot(const FHitResult* Hit)
{
	FHitResult* RicochetHit = new FHitResult();

	if (Hit != nullptr)
	{
		// get new (ricochet) direction
		FVector InitialDirection = (Hit->TraceStart - Hit->TraceEnd).GetSafeNormal();
		FVector NewDirection = (2 * FVector::DotProduct(InitialDirection, Hit->ImpactNormal) * Hit->ImpactNormal - InitialDirection).GetSafeNormal(); // vo=2*(vi.n)*n-vi and forgot unreal engine has GetReflectionVector()

		// find remaining travel distance
		double RemainingDistance = (Hit->Location - Hit->TraceEnd).Size();

		// get start position
		FVector Start = Hit->Location;

		// get end position
		FVector End = Start + NewDirection * RemainingDistance;

		// avoid shooting self
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);

		// "shoot" ricochet
		GetWorld()->LineTraceSingleByChannel(*RicochetHit, Start, End, ECC_Visibility, Params);
		DrawDebugLine(GetWorld(), Start, End, FColor(0, 255, 0), false, 5.f);
	}
}

static FVector* FibonacciSphereDistribution(int i, int NumOfSpikes)
{
	float k = i + 0.5f;

	float phi = std::acos(1.0f - 2.0f * k / NumOfSpikes);
	float theta = PI * (1 + std::sqrt(5.0f)) * k;

	float x = std::cos(theta) * std::sin(phi);
	float y = std::sin(theta) * std::sin(phi);
	float z = std::cos(phi);

	return new FVector(x, y, z);
}

void ABasicallyZeldaCharacter::ExplodeAttack()
{
	// generate # of spikes
	int NumOfSpikes = FMath::RandRange(MinimumSpikes, MaximumSpikes);

	// place spikes
	TArray<FVector*> SpikeLocations = PlaceSpikes(NumOfSpikes);

	// spawn spikes
	TArray<FHitResult*> SpikeHits = SpawnSpikes(SpikeLocations);
	
	// get 3 closest hits
	TArray<FHitResult*> ThreeClosestHits = GetThreeRandomHits(SpikeHits);
	
	// launch player towards normal of the hit points
	if (ThreeClosestHits.Num() >= 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Launching"));

		// make plane from 3 closest hits
		FPlane Plane(ThreeClosestHits[0]->Location, ThreeClosestHits[1]->Location, ThreeClosestHits[2]->Location);
		
		// get launch direction
		FVector LaunchDirection = Plane.GetSafeNormal();

		// make launch vector
		FVector LaunchVelocity = LaunchDirection * LaunchForce;

		ACharacter::LaunchCharacter(LaunchVelocity, false, true);
	}


	UE_LOG(LogTemp, Warning, TEXT("exploded"));
}

TArray<FVector*> ABasicallyZeldaCharacter::PlaceSpikes(int NumOfSpikes)
{
	TArray<FVector*> SpikeLocations;
	SpikeLocations.SetNum(NumOfSpikes);
	
	// evenly distribute spikes
	for (int i = 0; i < NumOfSpikes; ++i)
	{
		SpikeLocations[i] = FibonacciSphereDistribution(i, NumOfSpikes); // fancy math i didn't invent so not including (super cool though). used for evenly distributing points in the shape of a sphere
	}

	return SpikeLocations;
}

TArray<FHitResult*> ABasicallyZeldaCharacter::SpawnSpikes(TArray<FVector*> SpikeLocations)
{
	TArray<FHitResult*> SpikeHits;

	// simulate "spikes" with lines outward from the player
	for (FVector* SpikeLocation : SpikeLocations)
	{
		FHitResult* SpikeHit = new FHitResult();

		if (SpikeLocation != nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *SpikeLocation->ToString());

			// get spike direction (facing outward)
			FVector SpikeDirection = (GetActorLocation() - (GetActorLocation() + *SpikeLocation)).GetSafeNormal();

			// get start position
			FVector Start = GetActorLocation();

			// get end position
			FVector End = Start + SpikeDirection * SpikeLength;

			// avoid self
			FCollisionQueryParams Params;
			Params.AddIgnoredActor(this);

			// activate spike
			bool bHit = GetWorld()->LineTraceSingleByChannel(*SpikeHit, Start, End, ECC_Visibility, Params);
			DrawDebugLine(GetWorld(), Start, End, FColor(0, 0, 0), false, 5.f);

			if (bHit)
			{
				DrawDebugSphere(GetWorld(), SpikeHit->Location, 50.f, 10, FColor(255, 0, 0), false, 5.f);

				// add spike to return vector
				SpikeHits.Add(SpikeHit);
			}
		}
	}

	return SpikeHits;
}

TArray<FHitResult*> ABasicallyZeldaCharacter::GetThreeRandomHits(TArray<FHitResult*> SpikeHits)
{
	if (SpikeHits.Num() >= 3)
	{
		// store 3 random hits
		TArray<FHitResult*> RandomHits;

		// generate indices for choosing hits
		TSet<int> Indices;

		while (Indices.Num() < 3)
		{
			// generate random index
			int i = FMath::RandRange(0, SpikeHits.Num() - 1);

			// add without duplicates
			if (Indices.Contains(i) == false)
			{
				Indices.Add(i);
			}
		}

		// store respective spike hits using indices
		for (auto& i : Indices)
		{
			RandomHits.Add(SpikeHits[i]);
			DrawDebugSphere(GetWorld(), SpikeHits[i]->Location, 30.f, 10, FColor(0, 0, 255), false, 5.f);
		}

		//*** DEBUG ***//
		DrawDebugLine(GetWorld(), RandomHits[0]->TraceEnd, RandomHits[1]->TraceEnd, FColor(255, 255, 255), false, 5.f);
		DrawDebugLine(GetWorld(), RandomHits[0]->TraceEnd, RandomHits[2]->TraceEnd, FColor(255, 255, 255), false, 5.f);
		DrawDebugLine(GetWorld(), RandomHits[1]->TraceEnd, RandomHits[2]->TraceEnd, FColor(255, 255, 255), false, 5.f);

		return RandomHits;
	}

	return TArray<FHitResult*>();
}
