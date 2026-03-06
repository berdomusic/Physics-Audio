// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/PAPhysicsActor.h"

#include "Kismet/GameplayStatics.h"
#include "PhysicsAudio/PhysicsAudioProjectile.h"
#include "Subsystems/PAPhysicsAudioSubsystem.h"
#include "System/PAFunctionLibrary.h"
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
	SphereCollision->SetCollisionProfileName(TEXT("OverlapPhysicsActors"));
	
	HealthComponent = CreateDefaultSubobject<UPAHealthComponent>(TEXT("HealthComponent"));
}

void APAPhysicsActor::OnHitByProjectile_Implementation(AActor* ProjectileActor, const FHitResult& Hit, const FVector& InProjectileImpulse)
{	
	StaticMeshComponent->AddImpulseAtLocation(InProjectileImpulse, ProjectileActor->GetActorLocation());	
	const TArray<USceneComponent*>& attachedChildren = StaticMeshComponent->GetAttachChildren();
	
	for (USceneComponent* child : attachedChildren)
		if (child && child->GetClass()->ImplementsInterface(UProjectileInterface::StaticClass()))
			Execute_OnHitByProjectile(child, ProjectileActor, Hit, InProjectileImpulse);
}

void APAPhysicsActor::OnDamageDealt_Implementation(AActor* Dealer, const FHitResult& Hit, const FVector& InImpulse)
{
	Execute_OnDamageDealt(HealthComponent, Dealer, Hit, InImpulse);
}

void APAPhysicsActor::Init()
{
	if (!bAllowPhysicsSounds)
	{
		bAllowPhysicsSounds = UPAFunctionLibrary::ResolveAudioHandle(PhysicsAudioHandle, PhysicsAudioProperties) 
			&& UPAFunctionLibrary::IsAudioHandleNotEmpty(PhysicsAudioProperties);
	}
	if (!bAllowDestructionSounds)
	{
		bAllowDestructionSounds = 
			!DestructionMeshes.IsEmpty()
			&& UPAFunctionLibrary::ResolveAudioHandle(DestructionAudioHandle, DestructionAudioProperties) 
			&& UPAFunctionLibrary::IsAudioHandleNotEmpty(DestructionAudioProperties);
	}	
	
	if (bAllowPhysicsSounds)
	{
		SphereCollision->OnComponentBeginOverlap.AddUniqueDynamic(this, &APAPhysicsActor::OnActivationBeginOverlap);
		SphereCollision->OnComponentEndOverlap.AddUniqueDynamic(this, &APAPhysicsActor::OnActivationEndOverlap);
	}
	if (bAllowDestructionSounds)
	{
		HealthComponent->SetCanBeDamaged(true);
		HealthComponent->OnDeath.AddUniqueDynamic(this, &APAPhysicsActor::OnDeath);
	}		
}

void APAPhysicsActor::GetDestructibleMeshes()
{
	DestructionMeshes.Empty();	
	
	TArray<USceneComponent*> destructionComponents;
	StaticMeshComponent->GetChildrenComponents(false, destructionComponents);
	
	if (destructionComponents.IsEmpty())
		return;
	for (USceneComponent* component : destructionComponents)
		if (UStaticMeshComponent* staticMeshComp = Cast<UStaticMeshComponent>(component))
			DestructionMeshes.Add(staticMeshComp);
}

void APAPhysicsActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);	
	GetDestructibleMeshes();
	UStaticMesh* staticMeshAsset = StaticMeshComponent->GetStaticMesh();
	// Need a little bigger radius to give time for queue to attach physical audio
	if (staticMeshAsset != nullptr)
		SphereCollision->SetSphereRadius(staticMeshAsset->GetBounds().SphereRadius + 100.f);		
}

void APAPhysicsActor::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(GetWorld()))
		return;
	// ignore begin play overlaps
	//GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &APAPhysicsActor::Init));
	GetWorld()->GetTimerManager().SetTimer(
		InitTimerHandle,
		this,
		&APAPhysicsActor::Init,
		1.f,
		false
	);
}

void APAPhysicsActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivatePhysicsAudio();
	StaticMeshComponent->OnComponentHit.RemoveAll(this);		
	SphereCollision->OnComponentBeginOverlap.RemoveAll(this);
	SphereCollision->OnComponentEndOverlap.RemoveAll(this);
	HealthComponent->OnDeath.RemoveAll(this);
	Super::EndPlay(EndPlayReason);
}

void APAPhysicsActor::OnActivationBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor))
		return;
	if (!IsValid(OtherComp))
		return;	
	if (!ShouldActivatePhysicsAudio(OtherActor, OtherComp))
		return;	
	
	OverlappedActors.AddUnique(OtherActor);	
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
	if (!IsValid(subsystem))
		return;
	subsystem->TryAddPhysicsAudioToPrimitive(StaticMeshComponent, PhysicsAudioProperties);
	TriggerDeactivationTimer();
}

void APAPhysicsActor::OnActivationEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OverlappedActors.Contains(OtherActor))
		for (int i = 0; i < OverlappedActors.Num(); i++)
			if (OverlappedActors[i] == OtherActor)
			{
				OverlappedActors.RemoveAt(i);
				break;
			}	
}

void APAPhysicsActor::OnDeath(AActor* Dealer, const FHitResult& Hit, const FVector& Impulse)
{
	TEST_Destroy();
	
	UWorld* world = GetWorld();
	if (!IsValid(world))
		return;
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(world);
	if (!IsValid(subsystem))
		return;
	subsystem->ReturnPhysicsAudioComponentToPool(StaticMeshComponent);
	StaticMeshComponent->SetHiddenInGame(true);
	
	for (UStaticMeshComponent* meshComp : DestructionMeshes)
	{
		if (!IsValid(meshComp))
			continue;
		UStaticMesh* StaticMeshAsset = meshComp->GetStaticMesh();
		if (!StaticMeshAsset)
			continue;
		FTransform spawnTransform = meshComp->GetComponentTransform();
		APAPhysicsActor* newPhysicsActor = world->SpawnActorDeferred<APAPhysicsActor>(GetClass(), spawnTransform);
		if (!IsValid(newPhysicsActor))
			continue;
		newPhysicsActor->StaticMeshComponent->SetStaticMesh(StaticMeshAsset);
		newPhysicsActor->StaticMeshComponent->SetHiddenInGame(false);
		newPhysicsActor->StaticMeshComponent->SetSimulatePhysics(true);
		newPhysicsActor->StaticMeshComponent->SetMassOverrideInKg(NAME_None, StaticMeshComponent->GetMass() * 1.5f);
		newPhysicsActor->StaticMeshComponent->PrimaryComponentTick.SetTickFunctionEnable(true);
		newPhysicsActor->PhysicsAudioHandle = DestructionAudioHandle;
		UGameplayStatics::FinishSpawningActor(newPhysicsActor, spawnTransform);
		
		subsystem->TryAddPhysicsAudioToPrimitive(newPhysicsActor->StaticMeshComponent, PhysicsAudioProperties);		
		newPhysicsActor->StaticMeshComponent->AddRadialImpulse(Hit.Location, 100.f, 1000.f, RIF_Linear, true );
	}
	Destroy();
}

bool APAPhysicsActor::ShouldActivatePhysicsAudio(const AActor* OtherActor, UPrimitiveComponent* OtherComp) const
{
	if (!UPAFunctionLibrary::IsAudioHandleNotEmpty(PhysicsAudioProperties))
		return false; 
	if (IsPhysicsTriggerActor(OtherActor))
		return true;
	if (OtherComp->GetPhysicsLinearVelocity().SizeSquared() > PhysicsAudioSettings::PHYSICS_AUDIO_MIN_VELOCITY_SQUARED)
		return true;
	return false;
}

bool APAPhysicsActor::ShouldDeactivatePhysicsAudio()
{
	if (!OverlappedActors.IsEmpty())
		for (AActor* overlappedActor : OverlappedActors)
			if (IsPhysicsTriggerActor(overlappedActor))
				return false;
	if (StaticMeshComponent->GetPhysicsLinearVelocity().SizeSquared() > PhysicsAudioSettings::PHYSICS_AUDIO_MIN_VELOCITY_SQUARED)
		return false;
	return true;
}

bool APAPhysicsActor::IsPhysicsTriggerActor(const AActor* InActor)
{
	if (IsValid(InActor))
		if (InActor->IsA(APawn::StaticClass()) 
			|| InActor->IsA(APhysicsAudioProjectile::StaticClass())
			)
		return true;
	return false;
}

void APAPhysicsActor::DeactivatePhysicsAudio()
{
	if (!ShouldDeactivatePhysicsAudio())
		return;
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
	if (!IsValid(subsystem))
		return;
	subsystem->ReturnPhysicsAudioComponentToPool(StaticMeshComponent);
}

void APAPhysicsActor::TriggerDeactivationTimer()
{
	GetWorld()->GetTimerManager().SetTimer(
		DeactivationTimerHandle,
		this,
		&APAPhysicsActor::DeactivatePhysicsAudio,
		PhysicsAudioSettings::PHYSICS_AUDIO_DEACTIVATION_DELAY,
		true
	);
}
