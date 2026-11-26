#include "ETSaveDisk.h"
#include "ETGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

bool ETSaveDisk::Commit(const FString& Temp,const FString& Dest)
{
#if PLATFORM_WINDOWS
    const FString From=FPaths::ConvertRelativePathToFull(Temp),To=FPaths::ConvertRelativePathToFull(Dest);
    if(::MoveFileExW(*From,*To,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) return true;
#else
    if(IFileManager::Get().Move(*Dest,*Temp,true,true)) return true;
#endif
    IFileManager::Get().Delete(*Temp,false); return false;
}
bool ETSaveDisk::Write(UETSave* Profile,const FString& Slot,bool Backup)
{
    TArray<uint8> Data;
    if(!Profile||!UGameplayStatics::SaveGameToMemory(Profile,Data)) return false;
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("SaveGames");
    IFileManager::Get().MakeDirectory(*Dir,true);
    auto WriteBytes=[&](const FString& Name,const TArray<uint8>& Bytes) {
        const FString Dest=Dir/(Name+TEXT(".sav")),Temp=Dest+TEXT(".")+FGuid::NewGuid().ToString()+TEXT(".tmp");
        if(FFileHelper::SaveArrayToFile(Bytes,*Temp)) return Commit(Temp,Dest);
        IFileManager::Get().Delete(*Temp,false); return false;
    };
    if(Backup&&UGameplayStatics::DoesSaveGameExist(Slot,0)) {
        TArray<uint8> Previous;
        if(!UGameplayStatics::LoadDataFromSlot(Previous,Slot,0)||!WriteBytes(Slot+TEXT("-previous"),Previous)) return false;
    }
    return WriteBytes(Slot,Data);
}

