// @author Julien Olivier

#pragma once

#include "SaveableActorInterface.generated.h"

UINTERFACE(BlueprintType)
class USaveableActorInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ISaveableActorInterface
{
	GENERATED_IINTERFACE_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Saveable Actor")
	void ActorSaveDataLoaded();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Saveable Actor")
	void ActorSaveDataSaved();
};