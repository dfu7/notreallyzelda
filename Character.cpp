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

// awesome math not invented by me
static FVector* FibonacciSphereDistribution(int i, int NumOfSpikes) { ... }

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
	// intialize return vector
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
