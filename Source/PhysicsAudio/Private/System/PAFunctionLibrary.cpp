// Fill out your copyright notice in the Description page of Project Settings.


#include "System/PAFunctionLibrary.h"

#include "Engine/AssetManager.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsAudio/PhysicsAudioCharacter.h"
#include "System/PAGameReferencesSubsystem.h"
#include "AkAudioEvent.h"
#include "System/PhysicsAudioStructs.h"

UAkSwitchValue* UPAFunctionLibrary::GetAkSwitchFromSurface(const TEnumAsByte<EPhysicalSurface> InSurface)
{
	UDataAsset* dataAsset = UPAGameReferencesSubsystem::Get().GetMaterialSwitchDataAsset();
	if (!dataAsset)
		return nullptr;
	
	UPAMaterialSwitchMap* switchMap = Cast<UPAMaterialSwitchMap>(dataAsset);
	if (!switchMap)
		return nullptr;
	
	if (const TSoftObjectPtr<UAkSwitchValue>* foundSwitch =	switchMap->MaterialSwitchMap.Find(InSurface))
		return foundSwitch->LoadSynchronous();
	
	if (!switchMap->MaterialSwitchMap.IsEmpty())
	{
		const TSoftObjectPtr<UAkSwitchValue>& defaultSwitch = switchMap->MaterialSwitchMap.CreateConstIterator().Value();
		return defaultSwitch.LoadSynchronous();
	}
	return nullptr;
}

UAkAudioEvent* UPAFunctionLibrary::GetStopContinousSoundEvent(const EPAEventType InEventType)
{
	if (!(InEventType == EPAEventType::Roll || InEventType == EPAEventType::Slide))
		return nullptr;
		if (UDataAsset* dataAsset = UPAGameReferencesSubsystem::Get().GetStopContinuousSoundsDataAsset())
			if (UPAStopContinuousSoundEvents* stopContinousEvents = Cast<UPAStopContinuousSoundEvents>(dataAsset))
				switch (InEventType)
				{
				case EPAEventType::Roll: 
					return stopContinousEvents->StopRoll;
				case EPAEventType::Slide: 
					return stopContinousEvents->StopSlide;
				default: 
					return nullptr;
				}				
	checkNoEntry()
	return nullptr;
}

bool UPAFunctionLibrary::IsAudioHandleNotEmpty(const FPAPhysicsActorAudioProperties& InHandle)
{
	// Returns true if any sound asset is assigned
	return 
		!InHandle.ImpactSound.AkEventSoft.IsNull() ||
		!InHandle.SlideSound.AkEventSoft.IsNull() ||
		!InHandle.RollSound.AkEventSoft.IsNull() ||
		!InHandle.ProjectileSound.AkEventSoft.IsNull() ||
		!InHandle.DestructionSound.AkEventSoft.IsNull();	
}

bool UPAFunctionLibrary::ResolveAudioHandle(const FDataTableRowHandle& InRowHandle, FPAPhysicsActorAudioProperties& OutHandle)
{
	if (!InRowHandle.DataTable)
		return false;

	const FPAPhysicsActorAudioProperties* row =
		InRowHandle.DataTable->FindRow<FPAPhysicsActorAudioProperties>(
			InRowHandle.RowName,
			TEXT("ResolveAudioHandle")
		);

	if (row)
	{
		OutHandle = *row;
		return true;
	}	
	return false;
}

TArray<TSoftObjectPtr<UAkAudioEvent>> UPAFunctionLibrary::GetAkEventsSoftFromHandle(
	const FPAPhysicsActorAudioProperties& InHandle)
{
	TArray<TSoftObjectPtr<UAkAudioEvent>> outArray;
	if (IsAudioHandleNotEmpty(InHandle))
	{
		outArray.Add(InHandle.ImpactSound.AkEventSoft);
		outArray.Add(InHandle.SlideSound.AkEventSoft);
		outArray.Add(InHandle.RollSound.AkEventSoft);
		outArray.Add(InHandle.ProjectileSound.AkEventSoft);
		outArray.Add(InHandle.DestructionSound.AkEventSoft);
	}
	return outArray;
}

void UPAFunctionLibrary::LoadEventsFromHandle(const FPAPhysicsActorAudioProperties& InHandle, FStreamableDelegate OnLoaded)
{
	if (!IsAudioHandleNotEmpty(InHandle))
	{
		if (OnLoaded.IsBound())
			OnLoaded.Execute();
		return;
	}
    
	TArray<FSoftObjectPath> assetsToLoadAsync;
	TArray<TSoftObjectPtr<UAkAudioEvent>> eventsArray = GetAkEventsSoftFromHandle(InHandle);
    
	for (const TSoftObjectPtr<UAkAudioEvent>& currentAudioEventSoft : eventsArray)
	{
		if (currentAudioEventSoft.IsNull())
			continue;
		if (IsValid(currentAudioEventSoft.Get()))
			continue;
		assetsToLoadAsync.Add(currentAudioEventSoft.ToSoftObjectPath());
	}
    
	if (!assetsToLoadAsync.IsEmpty())
	{
		FStreamableManager& streamable = UAssetManager::GetStreamableManager();
		streamable.RequestAsyncLoad(assetsToLoadAsync, OnLoaded);
	}
	else if (OnLoaded.IsBound())
		OnLoaded.Execute();
}

float UPAFunctionLibrary::GetCurrentPickupLengthModifier(const UObject* InWorldContext)
{
	if (UWorld* world = InWorldContext->GetWorld())
		if (APhysicsAudioCharacter* character = Cast<APhysicsAudioCharacter>(UGameplayStatics::GetPlayerCharacter(world, 0)))
			return character->PickupLengthModifier;	
	return 1.f;
}
