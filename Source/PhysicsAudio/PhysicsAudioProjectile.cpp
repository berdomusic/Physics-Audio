// Copyright Epic Games, Inc. All Rights Reserved.

#include "PhysicsAudioProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
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
	// Only add impulse and destroy projectile if we hit a physics
	if ((OtherActor != nullptr) && (OtherActor != this) && (OtherComp != nullptr) && OtherComp->IsSimulatingPhysics())
	{
		FVector impulse = GetVelocity() * 100.f;
		if (OtherActor->GetClass()->ImplementsInterface(UProjectileInterface::StaticClass()))
			IProjectileInterface::Execute_OnHitByProjectile(OtherActor, this, Hit, impulse);
		else
			OtherComp->AddImpulseAtLocation(impulse, GetActorLocation());
		
		if (OtherActor->GetClass()->ImplementsInterface(UDamageInterface::StaticClass()))
			IDamageInterface::Execute_OnDamageDealt(OtherActor, this, Hit, impulse);		
	}
	if (IsValid(AudioComponent))
		AudioComponent->OnComponentHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}

void APhysicsAudioProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(AudioComponent))
		AudioComponent->OnAttachedToPhysicsComponent(CollisionComp, ProjectileAudioProperties, 100.f);
}
