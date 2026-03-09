// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/PAInteractableActor.h"

#include "Blueprint/UserWidget.h"

// Sets default values
APAInteractableActor::APAInteractableActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetupAttachment(RootComponent);
}

void APAInteractableActor::OnLookAtStarted_Implementation()
{
	ILookAtInterface::OnLookAtStarted_Implementation();
	if (WidgetComponent && WidgetComponent->GetWidget())
		WidgetComponent->SetVisibility(true);
}

void APAInteractableActor::OnLookAtFinished_Implementation()
{
	ILookAtInterface::OnLookAtFinished_Implementation();
	if (WidgetComponent && WidgetComponent->GetWidget())
		WidgetComponent->SetVisibility(false);
}
