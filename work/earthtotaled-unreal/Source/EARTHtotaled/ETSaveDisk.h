#pragma once
#include "CoreMinimal.h"
class UETSave;
namespace ETSaveDisk
{
    bool Commit(const FString& Temporary,const FString& Destination);
    bool Write(UETSave* Profile,const FString& Slot,bool Backup=false);
}

