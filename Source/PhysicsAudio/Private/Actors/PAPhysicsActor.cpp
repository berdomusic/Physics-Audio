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
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	RootComponent = StaticMeshComponent;
	StaticMeshComponent->SetCollisionProfileName("PhysicsActor");
	StaticMeshComponent->SetSimulatePhysics(true);
	StaticMeshComponent->SetNotifyRigidBodyCollision(true);	
	
	ActivationSphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationSphereCollision"));
	ActivationSphereCollision->SetupAttachment(RootComponent);
	ActivationSphereCollision->SetCollisionProfileName(TEXT("OverlapPhysicsActors"));
	ActivationSphereCollision->SetSphereRadius(ActivationSphereRadius);
	
	HealthComponent = CreateDefaultSubobject<UPAHealthComponent>(TEXT("HealthComponent"));
	
	WidgetComponent->SetupAttachment(StaticMeshComponent);
	WidgetComponent->SetVisibility(false);
	WidgetComponent->SetDrawSize(FVector2D(75, 25));
	WidgetComponent->SetRelativeLocation(FVector(0, 0, 100));
	
	bCanBePickedUp = true;
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

void APAPhysicsActor::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                            FVector NormalImpulse, const FHitResult& Hit)
{
	if (NormalImpulse.Size() < 50000.f)
		return;
	OnDamageDealt_Implementation(OtherActor, Hit, NormalImpulse * FMath::RandRange(.5f, .75f));	
	
	const TArray<USceneComponent*>& attachedChildren = StaticMeshComponent->GetAttachChildren();
	
	for (USceneComponent* child : attachedChildren)
		if (child && child->GetClass()->ImplementsInterface(UPhysicsAudioInterface::StaticClass()))
			Execute_OnPhysicsActorHit(child, NormalImpulse, Hit);
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
		ActivationSphereCollision->OnComponentBeginOverlap.AddUniqueDynamic(this, &APAPhysicsActor::OnActivationBeginOverlap);
		ActivationSphereCollision->OnComponentEndOverlap.AddUniqueDynamic(this, &APAPhysicsActor::OnActivationEndOverlap);
	}
	if (bAllowDestructionSounds)
	{
		StaticMeshComponent->OnComponentHit.AddUniqueDynamic(this, &APAPhysicsActor::OnHit);
		HealthComponent->SetCanBeDamaged(true);
		HealthComponent->OnDeath.AddUniqueDynamic(this, &APAPhysicsActor::OnDeath);
		SetupHealthWidget();
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
}

void APAPhysicsActor::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(GetWorld()))
		return;
	
	UStaticMesh* staticMeshAsset = StaticMeshComponent->GetStaticMesh();	
	if (!staticMeshAsset)
	{
		Destroy();
		return;
	}
	
	// Need a little bigger radius to give time for queue to attach physical audio
	ActivationSphereRadius = staticMeshAsset->GetBounds().SphereRadius + 200.f;
	ActivationSphereCollision->SetSphereRadius(ActivationSphereRadius);	
	
	// Ignore begin play overlaps
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
	ActivationSphereCollision->OnComponentBeginOverlap.RemoveAll(this);
	ActivationSphereCollision->OnComponentEndOverlap.RemoveAll(this);
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
	UWorld* world = GetWorld();
	if (!IsValid(world))
		return;
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(world);
	if (!IsValid(subsystem))
		return;
	subsystem->ReturnPhysicsAudioObjectToPool(StaticMeshComponent, nullptr, true);
	const float velocity = FMath::Max(StaticMeshComponent->GetPhysicsLinearVelocity().Length(), 1000.f);
	StaticMeshComponent->SetHiddenInGame(true);
	StaticMeshComponent->SetSimulatePhysics(false);
	StaticMeshComponent->OnComponentHit.RemoveAll(this);

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
		newPhysicsActor->StaticMeshComponent->PrimaryComponentTick.SetTickFunctionEnable(true);
		newPhysicsActor->PhysicsAudioHandle = DestructionAudioHandle;
		//newPhysicsActor->StaticMeshComponent->SetMassOverrideInKg(NAME_None, DestructionAudioProperties.ObjectMassOverride);//StaticMeshComponent->GetMass() * 1.5f);
		UGameplayStatics::FinishSpawningActor(newPhysicsActor, spawnTransform);
		// Run after construction script because parent sets name from class name
		newPhysicsActor->ItemName = FName(*(ItemName.ToString() + "_Debris"));
		newPhysicsActor->UpdateWidgetText();
		newPhysicsActor->bCanBePickedUp = false;
		
		subsystem->TryAddPhysicsAudioToPrimitive(newPhysicsActor->StaticMeshComponent, PhysicsAudioProperties);		
		newPhysicsActor->StaticMeshComponent->AddRadialImpulse(
			Hit.Location,
			100.f,
			velocity,
			RIF_Linear,
			true
			);
		newPhysicsActor->SetLifeTime(5.f);
	}
	Destroy();
}

bool APAPhysicsActor::ShouldActivatePhysicsAudio(AActor* OtherActor, UPrimitiveComponent* OtherComp) const
{
	if (!UPAFunctionLibrary::IsAudioHandleNotEmpty(PhysicsAudioProperties))
		return false;
	if (bIsPickedUp)
		return true;
	if (IsPhysicsTriggerActor(OtherActor))
		return true;
	if (OtherComp->GetPhysicsLinearVelocity().SizeSquared() > UPhysicsAudioSettings::GetMinVelocitySquared())
		return true;
	return false;
}

bool APAPhysicsActor::ShouldDeactivatePhysicsAudio()
{
	if (bIsPickedUp)
		return false;
	if (!OverlappedActors.IsEmpty())
		for (AActor* overlappedActor : OverlappedActors)
			if (IsPhysicsTriggerActor(overlappedActor))
				return false;
	if (StaticMeshComponent->GetPhysicsLinearVelocity().SizeSquared() > UPhysicsAudioSettings::GetMinVelocitySquared())
		return false;
	return true;
}

bool APAPhysicsActor::IsPhysicsTriggerActor(AActor* InActor)
{
	if (IsValid(InActor))
	{
		if (InActor->IsA(APawn::StaticClass()) 
			|| InActor->IsA(APhysicsAudioProjectile::StaticClass())
			)
			return true;
		if (InActor->IsA(StaticClass()))
			if (APAPhysicsActor* physicsActor = Cast<APAPhysicsActor>(InActor))
				if (physicsActor->bIsPickedUp)
					return true;
		UPrimitiveComponent* primitive = InActor->FindComponentByClass<UPrimitiveComponent>();
		if (IsValid(primitive))
			return primitive->GetPhysicsLinearVelocity().SizeSquared() > UPhysicsAudioSettings::GetMinVelocitySquared();
	}		
	return false;
}

void APAPhysicsActor::DeactivatePhysicsAudio()
{
	if (!ShouldDeactivatePhysicsAudio())
		return;
	UPAPhysicsAudioSubsystem* subsystem = UPAPhysicsAudioSubsystem::Get(GetWorld());
	if (!IsValid(subsystem))
		return;
	subsystem->ReturnPhysicsAudioObjectToPool(StaticMeshComponent, nullptr, false);
}

void APAPhysicsActor::TriggerDeactivationTimer()
{
	GetWorld()->GetTimerManager().SetTimer(
		DeactivationTimerHandle,
		this,
		&APAPhysicsActor::DeactivatePhysicsAudio,
		UPhysicsAudioSettings::GetDeactivationDelay(),
		true
	);
}

void APAPhysicsActor::OnPickup_Implementation(AActor* InInstigator)
{
	UStaticMesh* staticMeshAsset = StaticMeshComponent->GetStaticMesh();
	if (staticMeshAsset != nullptr)
		ActivationSphereCollision->SetSphereRadius(ActivationSphereRadius + 200.f, true);
	
	const TArray<USceneComponent*>& attachedChildren = StaticMeshComponent->GetAttachChildren();
	
	for (USceneComponent* child : attachedChildren)
		if (child && child->GetClass()->ImplementsInterface(ULookAtInterface::StaticClass()))
			Execute_OnPickup(child, InInstigator);
}

void APAPhysicsActor::OnDrop_Implementation(AActor* InInstigator)
{
	UStaticMesh* staticMeshAsset = StaticMeshComponent->GetStaticMesh();
	if (staticMeshAsset != nullptr)
		ActivationSphereCollision->SetSphereRadius(ActivationSphereRadius, true);	
	
	const TArray<USceneComponent*>& attachedChildren = StaticMeshComponent->GetAttachChildren();
	
	for (USceneComponent* child : attachedChildren)
		if (child && child->GetClass()->ImplementsInterface(ULookAtInterface::StaticClass()))
			Execute_OnDrop(child, InInstigator);
}
