// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "System/PhysicsAudioStructs.h"
#include "Components/WidgetComponent.h"
#include "PAInteractableActor.generated.h"


UCLASS()
class PHYSICSAUDIO_API APAInteractableActor : public AActor, public ILookAtInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APAInteractableActor();	

protected:
	
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void UpdateWidgetText();
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	UWidgetComponent* WidgetComponent;
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	bool bCanBePickedUp = true;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	FName ItemName;
		
	virtual void OnLookAtStarted_Implementation() override;
	virtual void OnLookAtFinished_Implementation() override;

};
