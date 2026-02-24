// Fill out your copyright notice in the Description page of Project Settings.


#include "System/PAGameReferencesSettings.h"

const UPAGameReferencesSettings& UPAGameReferencesSettings::Get()
{
	return *CastChecked<UPAGameReferencesSettings>(StaticClass()->GetDefaultObject());
}

const UPAGameReferencesSettings* UPAGameReferencesSettings::K2_Get()
{
	return &Get();
}
