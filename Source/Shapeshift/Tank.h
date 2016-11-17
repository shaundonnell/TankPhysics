// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Pawn.h"
#include "Tank.generated.h"

USTRUCT()
struct FTankWheel
{
	GENERATED_BODY()

	USceneComponent* WheelComponent;
	float Compression;
	FVector HitNormalVector;
	FVector HitForwardVector;
};

// This struct covers all possible input schemes
USTRUCT()
struct FTankInput
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Input")
	FVector2D MovementInput;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Input")
	uint32 bFire1 : 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Input")
	uint32 bFire2 : 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Input")
	uint32 bJumpPressed : 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Input")
	uint32 bJumpReleased : 1;

	void Sanitize();
	void MoveForward(float AxisValue);
	void MoveRight(float AxisValue);
	void Fire1(bool bPressed);
	void Fire2(bool bPressed);

private:
	FVector2D RawMovementInput;
};

UCLASS()
class SHAPESHIFT_API ATank : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATank();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	UStaticMeshComponent* TankMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	UStaticMeshComponent* TurretMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	UStaticMeshComponent* BarrelMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	UStaticMeshComponent* LeftTreadslMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	UStaticMeshComponent* RightTreadsMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	USceneComponent* WheelsContainer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank")
	UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float MoveForce;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float MoveForceUpOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float MoveForceForwardOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float TorqueForce;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float MaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float MaxTurnSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tank")
	float JumpForce;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tank Input")
	FTankInput TankInput;

private:

	FVector GroundForwardDirection;
	bool Grounded;

	float WheelOffset;
	float CompressionDistance;
	float IdleConpression;

	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);
	void MoveForward(float axisValue);
	void MoveRight(float axisValue);
	void BumpPressed();
	void JumpPressed();
	void JumpReleased();
	void Fire1();

	void UpdateFire();

	void AddWheel(FVector Location, FName name);

	TArray<FTankWheel> Wheels;

	void UpdateWheelCompression();
	void UpdateTurretRotation();
	void UpdateBarrelRotation();

	TArray<FTankWheel> GetWheels(bool Right);
	float GetAverageWheelCompression(bool Right);
	void SetTreadPosition(bool Right);
	void SetTreadRotation(bool Right);
};
