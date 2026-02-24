// Fill out your copyright notice in the Description page of Project Settings.


#include "System/PAGameReferencesSubsystem.h"
#include "System/PAGameReferencesSettings.h"

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