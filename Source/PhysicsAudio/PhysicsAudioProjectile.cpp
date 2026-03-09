// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsAudioProjectile.h"

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "System/PAFunctionLibrary.h"
#include "System/PhysicsAudioStructs.h"

APhysicsAudioProjectile::APhysicsAudioProjectile() 
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &APhysicsAudioProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;
	
	AudioComponent = CreateDefaultSubobject<UPAPhysicsAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
}

void APhysicsAudioProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	FVector impulse = GetVelocity() * 100.f;
	
	if (OtherActor != nullptr && OtherActor != this && OtherComp != nullptr)
	{		
		if (OtherActor->GetClass()->ImplementsInterface(UProjectileInterface::StaticClass()))
			IProjectileInterface::Execute_OnHitByProjectile(OtherActor, this, Hit, impulse);
		else if (OtherComp->IsSimulatingPhysics())
			OtherComp->AddImpulseAtLocation(impulse, GetActorLocation());
		
		if (OtherActor->GetClass()->ImplementsInterface(UDamageInterface::StaticClass()))
			IDamageInterface::Execute_OnDamageDealt(OtherActor, this, Hit, impulse);		
	}
	if (IsValid(AudioComponent) && IsValid(StaticMeshComponent)) // Call manually to avoid enabling simulation on static mesh
		AudioComponent->OnComponentHit(StaticMeshComponent, OtherActor, OtherComp, impulse, Hit);
}

UStaticMeshComponent* APhysicsAudioProjectile::GetStaticMeshComponent() const
{
	const TArray<USceneComponent*>& attachedChildren = RootComponent->GetAttachChildren();
	for (USceneComponent* child : attachedChildren)
	{
		UStaticMeshComponent* staticMeshComp = Cast<UStaticMeshComponent>(child);
		if (staticMeshComp)
			return staticMeshComp;
	}
	return nullptr;
}

void APhysicsAudioProjectile::BeginPlay()
{
	Super::BeginPlay();
	StaticMeshComponent = GetStaticMeshComponent();
	if (IsValid(AudioComponent) && IsValid(StaticMeshComponent))
		if (UPAFunctionLibrary::ResolveAudioHandle(PhysicsAudioHandle, ProjectileAudioProperties))
			AudioComponent->OnAttachedToNonSimulatingComponent(StaticMeshComponent, ProjectileAudioProperties);
}