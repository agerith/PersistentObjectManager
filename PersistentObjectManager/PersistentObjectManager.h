// @author Julien Olivier

#pragma once

#include <CoreMinimal.h>
#include <Engine/GameInstance.h>
#include "UObject/NameTypes.h"

#include "PersistentObjectManager.generated.h"

/* We need to set ArIsSaveGame and use the ObjectAndNameAsString Proxy */
struct FSaveGameArchive : public FObjectAndNameAsStringProxyArchive
{
	FSaveGameArchive(FArchive& InInnerArchive)
		: FObjectAndNameAsStringProxyArchive(InInnerArchive, true)
	{
		ArIsSaveGame = true;
	}
};

USTRUCT()
struct FActorSaveData
{
	GENERATED_USTRUCT_BODY()

	FString ActorClass;
	FName ActorName;
	FTransform ActorTransform;
	TArray<uint8> ActorData;

	friend FArchive& operator<<(FArchive& Ar, FActorSaveData& ActorData)
	{
		Ar << ActorData.ActorClass;
		Ar << ActorData.ActorName;
		Ar << ActorData.ActorTransform;
		Ar << ActorData.ActorData;
		return Ar;
	}
};

USTRUCT()
struct FGameInstanceSaveData
{
	GENERATED_USTRUCT_BODY()

	FString GameInstanceClass;
	FName GameInstanceName;
	TArray<uint8> GameInstanceData;

	friend FArchive& operator<<(FArchive& Ar, FGameInstanceSaveData& GameInstanceData)
	{
		Ar << GameInstanceData.GameInstanceClass;
		Ar << GameInstanceData.GameInstanceName;
		Ar << GameInstanceData.GameInstanceData;
		return Ar;
	}
};

USTRUCT()
struct FSaveGameData
{
	GENERATED_USTRUCT_BODY()

	FName GameID;
	FDateTime Timestamp;
	TArray<FActorSaveData> SavedActors;
	FGameInstanceSaveData SavedGameInstance;

	friend FArchive& operator<<(FArchive& Ar, FSaveGameData& GameData)
	{
		Ar << GameData.GameID;
		Ar << GameData.Timestamp;
		Ar << GameData.SavedActors;
		Ar << GameData.SavedGameInstance;
		return Ar;
	}
};

/**
 *
 */
UCLASS(ClassGroup = PersistentObjectManager, meta = (DisplayName = "PersistentObjectManager"))
class EXEMPLE_API UPersistentObjectManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()


/************************************************************************/
/* METHODS											     			    */
/************************************************************************/
public:
	UPersistentObjectManager();

	/** Begin USubsystem */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** End USubsystem */
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PersistentObjectManager")
	bool m_bIsOnLoading = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PersistentObjectManager")
	bool m_bIsOnSaving = false;

public:
	UFUNCTION(Category = "PersistentObjectManager", BlueprintCallable)
	static bool SaveGame(FName SlotName, int32 SlotId, bool bScreenshot = true, const int32 Width = 360, const int32 Height = 360);
	UFUNCTION(Category = "PersistentObjectManager", BlueprintCallable)
	static bool LoadGame(FName SlotName, int32 SlotId);
	UFUNCTION(Category = "PersistentObjectManager", BlueprintCallable)
	static bool SaveGameExist(FName SlotName, int32 SlotId);
	UFUNCTION(Category = "PersistentObjectManager", BlueprintCallable)
	static bool DeleteSaveGame(FName SlotName, int32 SlotId);

	static FString SavedDir();
	/** Returns this slot's thumbnail if any */
	UFUNCTION(Category = "PersistentObjectManager", BlueprintCallable)
	static UTexture2D* GetThumbnail(FName SlotName, int32 SlotId);

	/** Captures a thumbnail for the current slot */
	static bool CaptureThumbnail(FName SlotName, int32 SlotId, const int32 Width = 360, const int32 Height = 360);
};
