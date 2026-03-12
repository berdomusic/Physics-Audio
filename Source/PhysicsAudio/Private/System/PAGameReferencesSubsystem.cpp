// Fill out your copyright notice in the Description page of Project Settings.


#include "System/PAGameReferencesSubsystem.h"
#include "System/PAGameReferencesSettings.h"

void UPAGameReferencesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	const UPAGameReferencesSettings& settings = UPAGameReferencesSettings::Get();

	if (UPAGameReferencesSettings::Get().MaterialSwitchesSoft.bKeepLoaded)
		MaterialSwitchDataAsset = UPAGameReferencesSettings::Get().MaterialSwitchesSoft.DataAsset.LoadSynchronous();
	if (settings.RTPCs_Soft.bKeepLoaded)
		RTPCsDataAsset = settings.RTPCs_Soft.DataAsset.LoadSynchronous();	
	if (settings.StopContinuousSoundsDataAssetSoft.bKeepLoaded)
		StopContinuousSoundsDataAsset = settings.StopContinuousSoundsDataAssetSoft.DataAsset.LoadSynchronous();
}

void UPAGameReferencesSubsystem::Deinitialize()
{
	MaterialSwitchDataAsset = nullptr;
	RTPCsDataAsset = nullptr;
	StopContinuousSoundsDataAsset = nullptr;	
	Super::Deinitialize();
}

UPAGameReferencesSubsystem& UPAGameReferencesSubsystem::Get()
{
	UPAGameReferencesSubsystem* const subsystem = GEngine->GetEngineSubsystem<UPAGameReferencesSubsystem>();
	check(subsystem);
	return *subsystem;
}

UDataAsset* UPAGameReferencesSubsystem::GetMaterialSwitchDataAsset() const
{
	if(!MaterialSwitchDataAsset)
	{
		MaterialSwitchDataAsset = UPAGameReferencesSettings::Get().MaterialSwitchesSoft.DataAsset.LoadSynchronous();
	}
	return MaterialSwitchDataAsset;
}

UDataAsset* UPAGameReferencesSubsystem::GetRTPCsDataAsset() const
{
	if(!RTPCsDataAsset)
	{
		RTPCsDataAsset = UPAGameReferencesSettings::Get().RTPCs_Soft.DataAsset.LoadSynchronous();
	}
	return RTPCsDataAsset;
}

UDataAsset* UPAGameReferencesSubsystem::GetStopContinousSoundsDataAsset() const
{
	if(!StopContinuousSoundsDataAsset)
	{
		StopContinuousSoundsDataAsset = UPAGameReferencesSettings::Get().StopContinuousSoundsDataAssetSoft.DataAsset.LoadSynchronous();
	}
	return StopContinuousSoundsDataAsset;
}
