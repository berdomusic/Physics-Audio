// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/PAPhysicsActor.h"

#include "Kismet/GameplayStatics.h"
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
	RetriggerDeactivationTimer();
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

void APAPhysicsActor::OnPhysicsComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
                                            UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!ShouldDeactivatePhysicsAudio())
		RetriggerDeactivationTimer();
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
		StaticMeshComponent->OnComponentHit.AddUniqueDynamic(this, &APAPhysicsActor::OnPhysicsComponentHit);		
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
	if (staticMeshAsset != nullptr)
		SphereCollision->SetSphereRadius(staticMeshAsset->GetBounds().SphereRadius * 1.1f);
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
	RetriggerDeactivationTimer();
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
	RetriggerDeactivationTimer();
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
	if (OtherActor->IsA(APawn::StaticClass()))
		return true;
	if (OtherComp->GetPhysicsLinearVelocity().SizeSquared() > PhysicsAudioSettings::PHYSICS_AUDIO_MIN_VELOCITY_SQUARED)
		return true;		
	return false;
}

bool APAPhysicsActor::ShouldDeactivatePhysicsAudio() const
{
	if (!OverlappedActors.IsEmpty())
		for (const AActor* const overlappedActor : OverlappedActors)
			if (IsValid(overlappedActor) && overlappedActor->IsA(APawn::StaticClass()))
				return false;
	if (StaticMeshComponent->GetPhysicsLinearVelocity().SizeSquared() > PhysicsAudioSettings::PHYSICS_AUDIO_MIN_VELOCITY_SQUARED)
		return false;
	return true;
}

void APAPhysicsActor::DeactivatePhysicsAudio()
{
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
