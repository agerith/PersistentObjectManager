// @author Julien Olivier

#include "PersistentObjectManager.h"
#include "SaveableActorInterface.h"

#include <IImageWrapper.h>
#include <IImageWrapperModule.h>
#include <HighResScreenshot.h>
#include "Engine/World.h"

#define SAVE_EXTENSION_FILE ".sav"
#define CAPTURE_EXTENSION_FILE ".png"
#define GAME_ID "Exemple"

UPersistentObjectManager::UPersistentObjectManager() : Super() {}

void UPersistentObjectManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPersistentObjectManager::Deinitialize()
{
	Super::Deinitialize();
}

bool UPersistentObjectManager::SaveGame(FName SlotName, int32 SlotId, bool bScreenshot, const int32 Width, const int32 Height)
{
	//Save Actor
	TArray<FActorSaveData> SavedActors;

	m_bIsOnSaving = true;

	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsWithInterface(GWorld, USaveableActorInterface::StaticClass(), Actors);

	for (AActor* Actor : Actors)
	{
		//Saving each actor to record
		if (Actor == nullptr)
		{
			continue;
		}
		FActorSaveData ActorRecord;
		ActorRecord.ActorName = FName(*Actor->GetName());
		ActorRecord.ActorClass = Actor->GetClass()->GetPathName();
		ActorRecord.ActorTransform = Actor->GetTransform();

		FMemoryWriter MemoryWriter(ActorRecord.ActorData, true);
		FSaveGameArchive Ar(MemoryWriter);
		Actor->Serialize(Ar);

		SavedActors.Add(ActorRecord);
		ISaveableActorInterface::Execute_ActorSaveDataSaved(Actor);
	}

	//Save GameInstance
	UGameInstance* gameInstance = Cast<UGameInstance>(GWorld->GetGameInstance());
	FGameInstanceSaveData GameInstanceRecord;
	if (gameInstance)
	{
		GameInstanceRecord.GameInstanceName = FName(*gameInstance->GetName());
		GameInstanceRecord.GameInstanceClass = gameInstance->GetClass()->GetPathName();

		FMemoryWriter MemoryWriter(GameInstanceRecord.GameInstanceData, true);
		FSaveGameArchive Ar(MemoryWriter);
		gameInstance->Serialize(Ar);

		ISaveableActorInterface::Execute_ActorSaveDataSaved(gameInstance);
	}

	//Save File
	FSaveGameData SaveGameData;

	SaveGameData.GameID = GAME_ID;
	SaveGameData.Timestamp = FDateTime::Now();
	SaveGameData.SavedActors = SavedActors;
	SaveGameData.SavedGameInstance = GameInstanceRecord;

	FBufferArchive BinaryData;

	BinaryData << SaveGameData;

	if (BinaryData.Num() <= 0)
	{
		m_bIsOnSaving = false;
		return false;
	}

	FString pathSave = SavedDir() + SlotName.ToString() + FString::FromInt(SlotId) + FString(TEXT(SAVE_EXTENSION_FILE));

	if (SaveGameExist(SlotName, SlotId))
	{
		DeleteSaveGame(SlotName, SlotId);
	}

	if (FFileHelper::SaveArrayToFile(BinaryData, *pathSave))
	{
		UE_LOG(LogTemp, Warning, TEXT("Save Success!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Save Failed!"));
		m_bIsOnSaving = false;
		return false;
	}

	BinaryData.FlushCache();
	BinaryData.Empty();

	if (bScreenshot)
	{
		CaptureThumbnail(SlotName, SlotId, Width, Height);
	}

	m_bIsOnSaving = false;
	return true;
}

bool UPersistentObjectManager::LoadGame(FName SlotName, int32 SlotId)
{
	TArray<uint8> BinaryData;

	FString pathSave = SavedDir() + SlotName.ToString() + FString::FromInt(SlotId) + FString(TEXT(SAVE_EXTENSION_FILE));

	if (!FFileHelper::LoadFileToArray(BinaryData, *pathSave))
	{
		UE_LOG(LogTemp, Warning, TEXT("Load Failed!"));
		return false;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Load Succeeded!"));
		m_bIsOnLoading = true;
	}

	if (BinaryData.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Loaded file empty!"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Save File Size: %d"), BinaryData.Num());

	FMemoryReader FromBinary = FMemoryReader(BinaryData, true);
	FromBinary.Seek(0);

	FSaveGameData SaveGameData;
	FromBinary << SaveGameData;

	UE_LOG(LogTemp, Warning, TEXT("Loaded data with Timestamp: %s"), *SaveGameData.Timestamp.ToString());

	FromBinary.FlushCache();
	BinaryData.Empty();
	FromBinary.Close();

	//Load Actors

	for (FActorSaveData ActorRecord : SaveGameData.SavedActors)
	{
		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsWithInterface(GWorld, USaveableActorInterface::StaticClass(), Actors);

		for (AActor* Actor : Actors)
		{
			//Saving each actor to record
			if (Actor == nullptr)
			{
				continue;
			}

			FActorSpawnParameters SpawnParams;
			//Actor in Scene
			if (*Actor->GetName() == ActorRecord.ActorName)
			{
				FMemoryReader MemoryReader(ActorRecord.ActorData, true);
				FSaveGameArchive Ar(MemoryReader);
				Actor->Serialize(Ar);
				Actor->SetActorTransform(ActorRecord.ActorTransform);
				ISaveableActorInterface::Execute_ActorSaveDataLoaded(Actor);
			}
		}
	}

	//Load Game Instance
	UGameInstance* gameInstance = Cast<UGameInstance>(GWorld->GetGameInstance());
	if (gameInstance)
	{
		if (gameInstance->GetClass()->GetPathName() == SaveGameData.SavedGameInstance.GameInstanceClass)
		{
			FMemoryReader MemoryReader(SaveGameData.SavedGameInstance.GameInstanceData, true);
			FSaveGameArchive Ar(MemoryReader);
			gameInstance->Serialize(Ar);
			ISaveableActorInterface::Execute_ActorSaveDataLoaded(gameInstance);
		}
	}

	m_bIsOnLoading = false;
	return true;
}

bool UPersistentObjectManager::SaveGameExist(FName SlotName, int32 SlotId)
{
	bool bSaveGameExist = false;

	FString pathSave = SavedDir() + SlotName.ToString() + FString::FromInt(SlotId) + FString(TEXT(SAVE_EXTENSION_FILE));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (PlatformFile.FileExists(*pathSave))
	{
		bSaveGameExist = true;
	}
	else
	{
		bSaveGameExist = false;
	}

	return bSaveGameExist;
}

bool UPersistentObjectManager::DeleteSaveGame(FName SlotName, int32 SlotId)
{
	bool bSuccess = false;

	//Delete File
	FString pathSave = SavedDir() + SlotName.ToString() + FString::FromInt(SlotId) + FString(TEXT(SAVE_EXTENSION_FILE));
	if (FPaths::ValidatePath(pathSave) && FPaths::FileExists(pathSave))
	{
		IFileManager& FileManager = IFileManager::Get();
		if (FileManager.Delete(*pathSave))
		{
			UE_LOG(LogTemp, Warning, TEXT("Delete Succeeded!"));
			bSuccess = true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Delete Failed!"));
			bSuccess = false;
		}
	}

	//Delete Thumbnail
	FString pathThumbnail = SavedDir() + SlotName.ToString() + FString::FromInt(SlotId) + FString(TEXT(CAPTURE_EXTENSION_FILE));
	if (FPaths::ValidatePath(pathThumbnail) && FPaths::FileExists(pathThumbnail))
	{
		IFileManager& FileManager = IFileManager::Get();
		if (FileManager.Delete(*pathThumbnail))
		{
			UE_LOG(LogTemp, Warning, TEXT("Delete Succeeded!"));
			bSuccess = true;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Delete Failed!"));
			bSuccess = false;
		}
	}

	return bSuccess;
}

FString UPersistentObjectManager::SavedDir()
{
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()) + "SaveGames/";
}

UTexture2D* UPersistentObjectManager::GetThumbnail(FName SlotName, int32 SlotId)
{
	// Load thumbnail as Texture2D
	UTexture2D* Texture{ nullptr };
	TArray<uint8> RawFileData;
	FString pathThumbnail = SavedDir() + SlotName.ToString() + FString::FromInt(SlotId) + FString(TEXT(CAPTURE_EXTENSION_FILE));
	if (FFileHelper::LoadFileToArray(RawFileData, *pathThumbnail))
	{
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked < IImageWrapperModule >(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
		{
			const TArray<uint8>* UncompressedBGRA = nullptr;
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
			{
				Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
				void* TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
				Texture->PlatformData->Mips[0].BulkData.Unlock();
				Texture->UpdateResource();
			}
		}
	}
	return Texture;
}

bool UPersistentObjectManager::CaptureThumbnail(FName SlotName, int32 SlotId, const int32 Width /*= 360*/, const int32 Height /*= 360*/)
{
	if (!GEngine || !GEngine->GameViewport)
	{
		return false;
	}

	if (FViewport* Viewport = GEngine->GameViewport->Viewport)
	{
		FString pathThumbnail = SavedDir() + SlotName.ToString() + FString::FromInt(SlotId) + FString(TEXT(CAPTURE_EXTENSION_FILE));
		FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
		HighResScreenshotConfig.SetHDRCapture(false);
		//Set Screenshot path
		HighResScreenshotConfig.FilenameOverride = pathThumbnail;
		//Set Screenshot Resolution
		GScreenshotResolutionX = Width;
		GScreenshotResolutionY = Height;
		Viewport->TakeHighResScreenShot();
		return true;
	}
	return false;
}