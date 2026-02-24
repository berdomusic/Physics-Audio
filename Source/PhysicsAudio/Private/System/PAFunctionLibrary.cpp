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
