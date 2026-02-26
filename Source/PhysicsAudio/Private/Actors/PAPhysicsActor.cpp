// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/PAPhysicsActor.h"

#include "GameFramework/Character.h"
#include "Subsystems/PAPhysicsAudioSubsystem.h"
#include "System/PhysicsAudioSettings.h"

class UPAPhysicsAudioComponent;
// Sets default values
APAPhysicsActor::APAPhysicsActor()
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMeshComponent;
	StaticMeshComponent->SetCollisionProfileName("PhysicsActor");
	StaticMeshComponent->SetSimulatePhysics(true);
	StaticMeshComponent->SetNotifyRigidBodyCollision(true);	
	
	SphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationSphereCollision"));
	SphereCollision->SetupAttachment(RootComponent);
	SphereCollision->SetSphereRadius(250.f);
	SphereCollision->SetCollisionProfileName(TEXT("OverlapPhysicsActors"));	
}

void APAPhysicsActor::OnHitByProjectile_Implementation(AActor* HitActor, UPrimitiveComponent* HitComp,
	AActor* ProjectileActor, UPrimitiveComponent* ProjectileComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const TArray<USceneComponent*>& attachedChildren = StaticMeshComponent->GetAttachChildren();
	
	for (USceneComponent* child : attachedChildren)
		if (child && child->GetClass()->ImplementsInterface(UPhysicsAudioInterface::StaticClass()))
			Execute_OnHitByProjectile(child, HitActor, HitComp, ProjectileActor, ProjectileComp, NormalImpulse, Hit);			
}

void APAPhysicsActor::OnPhysicsComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                            UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	RetriggerDeactivationTimer();
}

void APAPhysicsActor::Init()
{
	if (bAllowPhysicsSounds)
	{
		StaticMeshComponent->OnComponentHit.AddUniqueDynamic(this, &APAPhysicsActor::OnPhysicsComponentHit);		
		SphereCollision->OnComponentBeginOverlap.AddUniqueDynamic(this, &APAPhysicsActor::OnActivationBeginOverlap);
		SphereCollision->OnComponentEndOverlap.AddUniqueDynamic(this, &APAPhysicsActor::OnActivationEndOverlap);
	}
}

void APAPhysicsActor::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(GetWorld()))
		return;
	// ignore begin play overlaps
	GetWorld()->GetTimerManager().SetTimer(
		InitTimerHandle,
		this,
		&APAPhysicsActor::Init,
		.2f,
		false
	);
}

void APAPhysicsActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivatePhysicsAudio();
	StaticMeshComponent->OnComponentHit.RemoveAll(this);
		
	SphereCollision->OnComponentBeginOverlap.RemoveAll(this);
	SphereCollision->OnComponentEndOverlap.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}

void APAPhysicsActor::OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor))
		return;
	if (!IsValid(OtherComp))
		return;
	if (OtherComp->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(PhysicsAudioSettings::PHYSICS_AUDIO_ACTIVATION_VELOCITY))
		return;
	OverlappedActors.AddUnique(OtherActor);
	
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
	if (!IsValid(subsystem))
		return;
	subsystem->TryAddPhysicsAudioToPrimitive(StaticMeshComponent, PhysicsAudioProperties);
	
	RetriggerDeactivationTimer();
}

void APAPhysicsActor::RetriggerDeactivationTimer()
{
	GetWorld()->GetTimerManager().SetTimer(
		DeactivationTimerHandle,
		this,
		&APAPhysicsActor::DeactivatePhysicsAudio,
		PhysicsAudioSettings::PHYSICS_AUDIO_DEACTIVATION_DELAY,
		false
	);
}

void APAPhysicsActor::OnActivationEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{	
	for (int i = 0; i < OverlappedActors.Num(); i++)
		if (OverlappedActors[i] == OtherActor)
		{
			OverlappedActors.RemoveAt(i);
			break;
		}			
	
	if (ShouldDeactivatePhysicsAudio())
		RetriggerDeactivationTimer();
}

bool APAPhysicsActor::ShouldDeactivatePhysicsAudio() const
{
	if (!OverlappedActors.IsEmpty())
	{
		for (const AActor* const overlappedActor : OverlappedActors)
			if (IsValid(overlappedActor) && overlappedActor->IsA(ACharacter::StaticClass()))
				return false; //if there are controllable characters nearby we want to preserve physics audio
	}
	else if (StaticMeshComponent->GetPhysicsLinearVelocity().SizeSquared() > FMath::Square(PhysicsAudioSettings::PHYSICS_AUDIO_ACTIVATION_VELOCITY))
		return false;	
	return true;
}

void APAPhysicsActor::DeactivatePhysicsAudio()
{
	if (!bAllowPhysicsSounds)
		return;
	if (ShouldDeactivatePhysicsAudio())
	{
		UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
		if (!IsValid(subsystem))
			return;
		subsystem->ReturnPhysicsAudioComponentToPool(StaticMeshComponent);
		return;
	}
	RetriggerDeactivationTimer();
}
