// Fill out your copyright notice in the Description page of Project Settings.


#include "System/PAFunctionLibrary.h"

#include "System/PAGameReferencesSubsystem.h"
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

UPAPhysicsRTPCs* UPAFunctionLibrary::GetRTPC_Assets()
{	
	if (UDataAsset* dataAsset = UPAGameReferencesSubsystem::Get().GetRTPCsDataAsset())
		if (UPAPhysicsRTPCs* rtpcAssets = Cast<UPAPhysicsRTPCs>(dataAsset))
			return rtpcAssets;
	checkNoEntry()
	return nullptr;
}

bool UPAFunctionLibrary::IsAudioHandleNotEmpty(const FPAPhysicsActorAudioHandle& InHandle)
{
	// Returns true if any sound asset is assigned
	return !InHandle.ImpactSound.IsNull() ||
		!InHandle.SlideSound.IsNull() ||
		!InHandle.ProjectileSound.IsNull() ||
		!InHandle.DestructionSound.IsNull();
}

bool UPAFunctionLibrary::ResolveAudioHandle(const FDataTableRowHandle& InRowHandle, FPAPhysicsActorAudioHandle& OutHandle)
{
	if (!InRowHandle.DataTable)
		return false;

	const FPAPhysicsActorAudioHandle* row =
		InRowHandle.DataTable->FindRow<FPAPhysicsActorAudioHandle>(
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
