// Fill out your copyright notice in the Description page of Project Settings.

#include "Shapeshift.h"
#include "Tank.h"


void FTankInput::Sanitize()
{
	MovementInput = RawMovementInput.ClampAxes(-1.0f, 1.0f);
	MovementInput = MovementInput.GetSafeNormal();
	RawMovementInput.Set(0.0f, 0.0f);
}

void FTankInput::MoveForward(float AxisValue)
{
	RawMovementInput.Y += AxisValue;
}

void FTankInput::MoveRight(float AxisValue)
{
	RawMovementInput.X += AxisValue;
}

void FTankInput::Fire1(bool bPressed)
{
	bFire1 = bPressed;
}

// Sets default values
ATank::ATank()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MoveForce = 500.0f;
	TorqueForce = 5000.0f;
	MoveForceUpOffset = -10.0f;
	MoveForceForwardOffset = 40.0f;
	MaxSpeed = 100.0f;
	MaxTurnSpeed = 100.0f;
	JumpForce = 5000.0f;
	CompressionDistance = 50.0f;
	IdleConpression = 0.125f;
	WheelOffset = -20.0f;

	RootComponent = TankMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TankMesh"));
	TankMesh->SetSimulatePhysics(true);
	TankMesh->WakeRigidBody();

	TurretMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TurretMesh"));
	TurretMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TurretMesh->SetupAttachment(TankMesh);

	BarrelMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrelMesh->SetupAttachment(TurretMesh);

	LeftTreadslMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftTreadsMesh"));
	LeftTreadslMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftTreadslMesh->SetupAttachment(TankMesh);

	RightTreadsMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightTreadsMesh"));
	RightTreadsMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightTreadsMesh->SetupAttachment(TankMesh);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->TargetArmLength = 500.0f;
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	SpringArm->SetRelativeRotation(FRotator(-30.0f, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	WheelsContainer = CreateDefaultSubobject<USceneComponent>(TEXT("WheelsContainer"));
	WheelsContainer->SetupAttachment(TankMesh);
	WheelsContainer->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	Wheels = TArray<FTankWheel>();

	AddWheel(FVector(80.0f, 75.0f, WheelOffset), TEXT("Wheel1"));
	AddWheel(FVector(-80.0f, 75.0f, WheelOffset), TEXT("Wheel2"));
	AddWheel(FVector(80.0f, -75.0f, WheelOffset), TEXT("Wheel3"));
	AddWheel(FVector(-80.0f, -75.0f, WheelOffset), TEXT("Wheel4"));
}

void ATank::AddWheel(FVector Location, FName name)
{
	USceneComponent* Wheel = CreateDefaultSubobject<USceneComponent>(name);
	Wheel->SetupAttachment(WheelsContainer);
	Wheel->SetRelativeLocation(Location);

	FTankWheel WheelStruct = FTankWheel();
	WheelStruct.WheelComponent = Wheel;
	WheelStruct.Compression = 0.0f;
	WheelStruct.HitNormalVector = TankMesh->GetUpVector();
	WheelStruct.HitForwardVector = TankMesh->GetForwardVector();

	Wheels.Add(WheelStruct);
}

// Called when the game starts or when spawned
void ATank::BeginPlay()
{
	Super::BeginPlay();
	GroundForwardDirection = TankMesh->GetForwardVector();

	TankMesh->SetCenterOfMass((TankMesh->GetUpVector() * -100));
}

// Called every frame
void ATank::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	UWorld* World = GetWorld();

	if (!World) { return; }

	SpringArm->SetWorldLocation(TankMesh->GetComponentLocation() + (FVector::UpVector * 150.0f));

	TankInput.Sanitize();

	UpdateWheelCompression();

	SetTreadPosition(true);
	SetTreadPosition(false);

	SetTreadRotation(true);
	SetTreadRotation(false);

	UpdateTurretRotation();
	UpdateBarrelRotation();

	UpdateFire();

	if (Grounded)
	{
		if (TankInput.bJumpPressed)
		{
			TankInput.bJumpPressed = false;
			TankMesh->AddImpulse(TankMesh->GetMass() * TankMesh->GetUpVector() * JumpForce);
		}

		// Setup Left/Right damping
		float currentRightForce = FVector::DotProduct(TankMesh->GetRightVector(), TankMesh->GetPhysicsLinearVelocity()) * TankMesh->GetMass();

		FVector RightForceLocation = TankMesh->GetCenterOfMass();
		RightForceLocation += TankMesh->GetUpVector() * -5;
		TankMesh->AddForceAtLocation(currentRightForce * TankMesh->GetRightVector() * -4.0f, RightForceLocation);

		if (!TankInput.MovementInput.IsNearlyZero())
		{
			FVector ForceLocation = TankMesh->GetCenterOfMass();
			ForceLocation += TankMesh->GetUpVector() * MoveForceUpOffset;
			ForceLocation += TankMesh->GetForwardVector() * MoveForceForwardOffset;

			float AngleDelta = 0.0f;

			FVector Direction = (Camera->GetForwardVector() * TankInput.MovementInput.Y) + (Camera->GetRightVector() * TankInput.MovementInput.X);
			Direction = Direction.GetSafeNormal();
			AngleDelta = Direction.Rotation().Yaw - TankMesh->GetComponentRotation().Yaw;

			if (AngleDelta > 180.0f) { AngleDelta = AngleDelta - 360.0f; }
			else if (AngleDelta < -180.0f) { AngleDelta = AngleDelta + 360.0f; }

			float AdjustedDeltaYaw = AngleDelta;

			//bool bReverse = TankInput.MovementInput.Y < -0.1f && FMath::Abs(TankInput.MovementInput.X) < 0.5f;
			bool bReverse = AngleDelta > 90.0f || AngleDelta < -90.0f;
			if (bReverse)
			{
				if (AdjustedDeltaYaw < 0) { AdjustedDeltaYaw += 180.0f; }
				else if (AdjustedDeltaYaw > 0) { AdjustedDeltaYaw -= 180.0f; }
			}

			float CurrentVelocity = FVector::DotProduct(TankMesh->GetPhysicsLinearVelocity(), TankMesh->GetForwardVector());
			if (FMath::Abs(CurrentVelocity) < MaxSpeed)
			{
				FVector ForwardForce = TankMesh->GetMass() * GroundForwardDirection * MoveForce * TankInput.MovementInput.Size();
				ForwardForce = ForwardForce * (bReverse ? -1.0f : 1.0f);
				TankMesh->AddForceAtLocation(ForwardForce, ForceLocation);
			}

			float CurrentTurnVelocity = FVector::DotProduct(TankMesh->GetPhysicsAngularVelocity(), TankMesh->GetUpVector());

			if (FMath::Abs(CurrentTurnVelocity) < MaxTurnSpeed && TankInput.MovementInput.Size() > 0.1)
			{
				FVector Torque = TankMesh->GetMass() * TankMesh->GetUpVector() * TorqueForce * -1.0f;

				Torque = Torque * (AdjustedDeltaYaw < 0 ? 1.0f : -1.0f);

				if (FMath::Abs(AdjustedDeltaYaw) < 30)
				{
					Torque = Torque * (FMath::Abs(AdjustedDeltaYaw) / 30);
					TankMesh->SetAngularDamping(3.0f);
				}
				else
				{
					TankMesh->SetAngularDamping(0.0f);
				}

				TankMesh->AddTorque(Torque);
			}
			else
			{
				TankMesh->SetAngularDamping(3.0f);
			}
		}
		else
		{
			// Setup Forward damping damping
			float currentForwardForce = FVector::DotProduct(TankMesh->GetForwardVector(), TankMesh->GetPhysicsLinearVelocity()) * TankMesh->GetMass();
			TankMesh->AddForce(currentForwardForce * TankMesh->GetForwardVector() * -4.0f);

			// Add angular damping
			TankMesh->SetAngularDamping(3.0f);
		}
	}
	else
	{
		TankMesh->SetAngularDamping(0.0f);
	}
}

void ATank::UpdateFire()
{
	if (TankInput.bFire1)
	{
		TankInput.bFire1 = false;

		FVector LineStart = BarrelMesh->GetComponentLocation();
		FVector LineEnd = BarrelMesh->GetComponentLocation() + (BarrelMesh->GetComponentRotation().Vector() * 10000.0f);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, LineStart, LineEnd, ECollisionChannel::ECC_Visibility))
		{
			float SphereRadius = 200.0f;
			float BlastForce = 2000.0f;
			DrawDebugLine(GetWorld(), LineStart, Hit.Location, FColor::Green, false, 0.1f, 0, 5.0f);
			DrawDebugSphere(GetWorld(), Hit.Location, SphereRadius, 64, FColor::Green, false, 0.1f);

			UStaticMeshComponent* HitComp = Cast<UStaticMeshComponent>(Hit.GetComponent());
			if (HitComp)
			{
				UE_LOG(LogTemp, Warning, TEXT("Direct Hit!"));
				FVector ForceDirection = Hit.Location - LineStart;
				HitComp->AddImpulseAtLocation(ForceDirection.GetSafeNormal() * BlastForce, Hit.Location);
			}

			TArray<FOverlapResult> HitResults;
			if (GetWorld()->OverlapMultiByChannel(
				HitResults, 
				Hit.Location, 
				FQuat(), 
				ECollisionChannel::ECC_Visibility, 
				FCollisionShape::MakeSphere(SphereRadius)
			))
			{
				for (FOverlapResult HitObject : HitResults)
				{
					if (UStaticMeshComponent* FoundComp = Cast<UStaticMeshComponent>(HitObject.GetComponent()))
					{
						FHitResult DistanceHit;
						if (GetWorld()->LineTraceSingleByChannel(DistanceHit, Hit.Location, HitObject.Actor->GetActorLocation(), ECollisionChannel::ECC_Visibility))
						{
							if (FoundComp == HitComp)
							{
								UE_LOG(LogTemp, Warning, TEXT("Skipping Direct Hit"));
							}
							else
							{
								FVector ForceDirection = HitObject.Actor->GetActorLocation() - Hit.Location;
								UE_LOG(LogTemp, Warning, TEXT("Distance: %f"), DistanceHit.Distance);
								UE_LOG(LogTemp, Warning, TEXT("Multiple: %f"), DistanceHit.Distance / SphereRadius);
								float DistanceMultiple = 1 - FMath::Clamp(DistanceHit.Distance / SphereRadius, 0.0f, 1.0f);
								//DistanceMultiple = FMath::Pow(DistanceMultiple, 2);
								float ForceMagnitude = BlastForce * DistanceMultiple;

								FVector RandomVarience = FVector(FMath::FRand(), FMath::FRand(), FMath::FRand()) * 10.0f;
								FoundComp->AddImpulseAtLocation(ForceDirection.GetSafeNormal() * ForceMagnitude, FoundComp->GetCenterOfMass() + RandomVarience);
							}
						}
					}
				}
			}
		}
		else
		{
			DrawDebugLine(GetWorld(), LineStart, LineEnd, FColor::Green, false, 0.1f, 0, 5.0f);
		}
	}
}

void ATank::UpdateWheelCompression()
{
	UWorld* World = GetWorld();

	float ForcePerWheel = (980 * TankMesh->GetMass()) / WheelsContainer->GetAttachChildren().Num();
	ForcePerWheel /= IdleConpression;

	int32 NumWheelsGrounded = 0;
	TArray<FTankWheel> NewWheels = TArray<FTankWheel>();
	for (FTankWheel WheelStruct : Wheels)
	{
		FHitResult hit;

		FVector WheelLocation = WheelStruct.WheelComponent->GetComponentLocation();
		FVector LineEnd = WheelLocation + (TankMesh->GetUpVector() * -CompressionDistance);

		//DrawDebugLine(World, WheelLocation, LineEnd, FColor::Red);
		if (World->LineTraceSingleByChannel(hit, WheelLocation, LineEnd, ECollisionChannel::ECC_Visibility))
		{
			NumWheelsGrounded++;

			WheelStruct.Compression = 1 - (hit.Distance / CompressionDistance);
			FVector SurfaceImpactRight = FVector::CrossProduct(hit.Normal, TankMesh->GetForwardVector());
			GroundForwardDirection = FVector::CrossProduct(SurfaceImpactRight, hit.Normal).GetSafeNormal();

			float ForceMagnitude = ForcePerWheel * FMath::Pow(WheelStruct.Compression, 2);
			//float currentForce = FVector::DotProduct(TankMesh->GetUpVector(), TankMesh->GetPhysicsLinearVelocityAtPoint(WheelLocation)) * TankMesh->GetMass();
			float currentForce = FVector::DotProduct(FVector::UpVector, TankMesh->GetPhysicsLinearVelocityAtPoint(WheelLocation)) * TankMesh->GetMass();

			float ForceDelta = ForceMagnitude - currentForce;
			//FVector Force = TankMesh->GetUpVector() * ForceDelta;
			FVector Force = FVector::UpVector * ForceDelta;
			TankMesh->AddForceAtLocation(Force, WheelLocation);
		}
		else
		{
			WheelStruct.Compression = 0;
		}

		Grounded = NumWheelsGrounded >= 3;

		NewWheels.Add(FTankWheel(WheelStruct));
	}

	Wheels.Empty();
	Wheels.Append(NewWheels);
}

void  ATank::UpdateTurretRotation()
{
	float DeltaRotation = Camera->GetForwardVector().Rotation().Yaw - TurretMesh->GetComponentRotation().Yaw;
	if (DeltaRotation > 180.0f) { DeltaRotation = DeltaRotation - 360.0f; }
	else if (DeltaRotation < -180.0f) { DeltaRotation = DeltaRotation + 360.0f; }
	float MaxTurretRotateAmount = 100.0f * GetWorld()->GetDeltaSeconds();

	float NewTurretYaw = TurretMesh->GetRelativeTransform().Rotator().Yaw;
	if (FMath::Abs(DeltaRotation) > MaxTurretRotateAmount)
	{
		NewTurretYaw += MaxTurretRotateAmount * FMath::Sign(DeltaRotation);
	}
	else
	{
		NewTurretYaw += DeltaRotation;
	}

	TurretMesh->SetRelativeRotation(FRotator(0.0f, NewTurretYaw, 0.0f));
}

void  ATank::UpdateBarrelRotation()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FVector WorldPosition;
		FVector WorldDirection;
		int32 ViewPortSizeX, ViewPortSizeY;
		PC->GetViewportSize(ViewPortSizeX, ViewPortSizeY);
		FVector2D ScreenLocation = FVector2D(ViewPortSizeX * 0.5f, ViewPortSizeY * 0.5f);
		UGameplayStatics::DeprojectScreenToWorld(PC, ScreenLocation, WorldPosition, WorldDirection);

		FHitResult Hit;
		FVector LineEnd = WorldPosition + (Camera->GetForwardVector() * 10000.0f);
		FVector HitPosition = LineEnd;
		if (GetWorld()->LineTraceSingleByChannel(Hit, WorldPosition, LineEnd, ECollisionChannel::ECC_Visibility))
		{
			HitPosition = Hit.Location;
		}

		//DrawDebugLine(World, BarrelMesh->GetComponentLocation(), HitPosition, FColor::Green, false, -1.0f, 0, 10.0f);
		FVector BarrelDirection = (HitPosition - BarrelMesh->GetComponentLocation()).GetSafeNormal();

		float BarrelDeltaRotation = BarrelDirection.Rotation().Pitch - BarrelMesh->GetComponentRotation().Pitch;
		if (BarrelDeltaRotation > 180.0f) { BarrelDeltaRotation = BarrelDeltaRotation - 360.0f; }
		else if (BarrelDeltaRotation < -180.0f) { BarrelDeltaRotation = BarrelDeltaRotation + 360.0f; }

		float MaxRotatePerFrame = 45.0f * GetWorld()->GetDeltaSeconds();

		float NewBarrelPitch = BarrelMesh->GetRelativeTransform().Rotator().Pitch;
		if (FMath::Abs(BarrelDeltaRotation) > MaxRotatePerFrame)
		{
			NewBarrelPitch += MaxRotatePerFrame * FMath::Sign(BarrelDeltaRotation);
		}
		else
		{
			NewBarrelPitch += BarrelDeltaRotation;
		}

		NewBarrelPitch = FMath::Clamp(NewBarrelPitch, -10.0f, 50.0f);

		BarrelMesh->SetRelativeRotation(FRotator(NewBarrelPitch, 0.0f, 0.0f));
	}
}

TArray<FTankWheel> ATank::GetWheels(bool Right)
{
	TArray<FTankWheel> WheelList = TArray<FTankWheel>();
	for (FTankWheel Wheel : Wheels)
	{
		float WheelYPos = Wheel.WheelComponent->GetRelativeTransform().GetLocation().Y;

		if (Right && WheelYPos > 0)
		{
			WheelList.Add(Wheel);
		}
		else if (!Right && WheelYPos < 0)
		{
			WheelList.Add(Wheel);
		}
	}
	return WheelList;
}

float ATank::GetAverageWheelCompression(bool Right)
{
	TArray<FTankWheel> WheelList = GetWheels(Right);
	float AverageCompression = 0;
	for (FTankWheel Wheel : WheelList)
	{
		AverageCompression += Wheel.Compression;
	}
	AverageCompression /= WheelList.Num();
	return AverageCompression;
}

void ATank::SetTreadPosition(bool Right)
{
	float MaxMovePerFrame = 150.0f * GetWorld()->DeltaTimeSeconds;

	UStaticMeshComponent* TreadsMesh = Right ? RightTreadsMesh : LeftTreadslMesh;
	float AvergeCompression = 1 - FMath::Pow(GetAverageWheelCompression(Right), 2);

	FVector NewLocation = TreadsMesh->GetRelativeTransform().GetLocation();
	float MaxWheelZ = WheelOffset - CompressionDistance;
	float LocationToMove = FMath::Clamp<float>(AvergeCompression * MaxWheelZ, MaxWheelZ, WheelOffset);
	float DeltaMove = LocationToMove - NewLocation.Z;
	NewLocation.Z += MaxMovePerFrame * FMath::Sign(DeltaMove);
	if (FMath::Abs(DeltaMove) < MaxMovePerFrame) { NewLocation.Z = LocationToMove; }
	TreadsMesh->SetRelativeLocation(NewLocation);
}

void ATank::SetTreadRotation(bool Right)
{
	float MaxRotatePerFrame = 90.0f * GetWorld()->DeltaTimeSeconds;

	UStaticMeshComponent* TreadsMesh = Right ? RightTreadsMesh : LeftTreadslMesh;

	TArray<FTankWheel> WheelList = GetWheels(Right);
	FVector Wheel0Location = WheelList[0].WheelComponent->GetRelativeTransform().GetLocation();
	FVector Wheel1Location = WheelList[1].WheelComponent->GetRelativeTransform().GetLocation();
	bool Wheel0IsFront = WheelList[0].WheelComponent->GetRelativeTransform().GetLocation().X > 0;
	FVector FrontComponentLocation = Wheel0IsFront ? Wheel0Location : Wheel1Location;
	FVector BackComponentLocation = Wheel0IsFront ? Wheel1Location : Wheel0Location;
	float FrontCompression = Wheel0IsFront ? WheelList[0].Compression : WheelList[1].Compression;
	float BackCompression = Wheel0IsFront ? WheelList[1].Compression : WheelList[0].Compression;
	float MaxWheelZ = WheelOffset - CompressionDistance;
	float FrontLocation = FMath::Clamp<float>((1 - FMath::Pow(FrontCompression, 2)) * MaxWheelZ, MaxWheelZ, WheelOffset);
	float BackLocation = FMath::Clamp<float>((1 - FMath::Pow(BackCompression, 2)) * MaxWheelZ, MaxWheelZ, WheelOffset);
	FVector FrontLocationVector = FrontComponentLocation + (TankMesh->GetUpVector() * FrontLocation);
	FVector BackLocationVector = BackComponentLocation + (TankMesh->GetUpVector() * BackLocation);

	FVector Direction = (FrontLocationVector - BackLocationVector).GetSafeNormal();

	FRotator CurrentRotation = TreadsMesh->GetRelativeTransform().Rotator();
	float RotationDelta = Direction.Rotation().Pitch - CurrentRotation.Pitch;
	CurrentRotation.Pitch += MaxRotatePerFrame * FMath::Sign(RotationDelta);

	if (FMath::Abs(RotationDelta) < MaxRotatePerFrame) 
	{ 
		CurrentRotation.Pitch = Direction.Rotation().Pitch;
	}

	TreadsMesh->SetRelativeRotation(CurrentRotation);
}

// Called to bind functionality to input
void ATank::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);

	InputComponent->BindAction("Bump", EInputEvent::IE_Pressed, this, &ATank::BumpPressed);
	InputComponent->BindAction("Jump", EInputEvent::IE_Pressed, this, &ATank::JumpPressed);

	InputComponent->BindAction("Fire1", EInputEvent::IE_Pressed, this, &ATank::Fire1);

	InputComponent->BindAxis("Forward", this, &ATank::MoveForward);
	InputComponent->BindAxis("Right", this, &ATank::MoveRight);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookRight", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &ATank::TurnAtRate);
	InputComponent->BindAxis("LookUpRate", this, &ATank::LookUpAtRate);
}

void ATank::MoveForward(float AxisValue) { TankInput.MoveForward(AxisValue); }
void ATank::MoveRight(float AxisValue) { TankInput.MoveRight(AxisValue); }
void ATank::Fire1() { TankInput.Fire1(true); }
void ATank::JumpPressed() { TankInput.bJumpPressed = true; }

void ATank::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * 45.0f * GetWorld()->GetDeltaSeconds());
}

void ATank::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * 45.0f * GetWorld()->GetDeltaSeconds());
}

void ATank::BumpPressed()
{
	float RandomX = (FMath::FRand() * 150) - 75;
	float RandomY = (FMath::FRand() * 100) - 50;
	FVector RandomVector = TankMesh->GetComponentLocation() + FVector(RandomX, RandomY, 0.0f);
	FVector Force = FVector::UpVector * TankMesh->GetMass() * (JumpForce / 2);
	TankMesh->AddImpulseAtLocation(Force, RandomVector);
}


