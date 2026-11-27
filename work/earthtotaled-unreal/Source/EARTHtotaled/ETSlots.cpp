#include "ETGame.h"
#include "ETLandfall.h"
#include "ETSaveDisk.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/FileManager.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Async/Async.h"
#include "HAL/PlatformApplicationMisc.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <windows.h>
#include <shobjidl.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace
{
    FString PickFile(bool Write,int32 Slot,const FString& TestFolder)
    {
#if PLATFORM_WINDOWS
        HWND Owner=nullptr;
        if(GEngine&&GEngine->GameViewport) if(auto W=GEngine->GameViewport->GetWindow();W&&W->GetNativeWindow()) Owner=(HWND)W->GetNativeWindow()->GetOSWindowHandle();
        auto Result=Async(EAsyncExecution::Thread,[Owner,Write,Slot,TestFolder](){ 
            FString Path;
            if(FAILED(CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED))) return Path;
            IFileDialog* Dialog=nullptr;
            if(SUCCEEDED(CoCreateInstance(Write?CLSID_FileSaveDialog:CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&Dialog)))) {
                const COMDLG_FILTERSPEC Filter[]={ {L"EARTHtotaled survivor",L"*.etsave"} };
                FILEOPENDIALOGOPTIONS Opts=0; Dialog->GetOptions(&Opts);
                Dialog->SetOptions(Opts|FOS_FORCEFILESYSTEM|FOS_NOCHANGEDIR|(Write?FOS_OVERWRITEPROMPT:FOS_FILEMUSTEXIST));
                Dialog->SetFileTypes(1,Filter); Dialog->SetDefaultExtension(L"etsave");
                Dialog->SetTitle(Write?L"Export EARTHtotaled survivor":L"Import EARTHtotaled survivor");
                if(!TestFolder.IsEmpty()) {
                    IShellItem* Folder=nullptr;
                    if(SUCCEEDED(SHCreateItemFromParsingName(*TestFolder,nullptr,IID_PPV_ARGS(&Folder)))) {Dialog->SetFolder(Folder); Folder->Release();}
                    Dialog->SetFileName(L"Transfer survivor.etsave");
                } else if(Write) Dialog->SetFileName(*FString::Printf(TEXT("EARTHtotaled-slot-%02d.etsave"),Slot));
                if(SUCCEEDED(Dialog->Show(Owner))) {
                    IShellItem* Item=nullptr;
                    if(SUCCEEDED(Dialog->GetResult(&Item))) {PWSTR Value=nullptr;
                        if(SUCCEEDED(Item->GetDisplayName(SIGDN_FILESYSPATH,&Value))) {Path=Value; CoTaskMemFree(Value);} Item->Release();
                    }
                }
                Dialog->Release();
            }
            CoUninitialize(); return Path;
        });
        while(!Result.IsReady()) {FPlatformApplicationMisc::PumpMessages(true); FPlatformProcess::Sleep(.012f);}
        return Result.Get();
#else
        return FString();
#endif
    }
}
FString AETWorld::SlotName(int32 Number) const
{
    return FString::Printf(TEXT("%s-%02d"),(SlotQA||SelfTest||FrontierQA||Review)?TEXT("EARTHtotaled-QA-Slot"):TEXT("EARTHtotaled-Unreal"),FMath::Clamp(Number,1,10));
}
void AETWorld::RememberSlot(int32 N)
{
    if(Review||FrontierQA||BorerRecoveryQA) return;
    GConfig->SetInt(SlotQA?TEXT("OutpostQA"):TEXT("Survivors"),TEXT("ActiveSlot"),N,GGameUserSettingsIni);
    GConfig->Flush(false,GGameUserSettingsIni);
}
void AETCharacter::TravelSlot(int32 Number,bool Start,bool Shelter)
{
    World->RememberSlot(Number);
    Panel=TEXT("travel"); PendingImport=nullptr;
    UGameplayStatics::SetGamePaused(this,false);
    FString Options=FString::Printf(TEXT("ETSlot=%d%s"),FMath::Clamp(Number,1,10),Start?TEXT("?ETContinue"):TEXT(""));
    if(Shelter) Options+=TEXT("?ETRecoverShelter");
    UGameplayStatics::OpenLevel(this,FName(*UGameplayStatics::GetCurrentLevelName(this,true)),true,Options);
}
FString AETCharacter::SlotSummary(int32 N) const
{
    const auto Key=World->SlotName(N);
    if(!UGameplayStatics::DoesSaveGameExist(Key,0)) return TEXT("Empty");
    auto* S=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(Key,0));
    if(!S||S->Schema!=1) return TEXT("Unreadable");
    return FString::Printf(TEXT("Level %d   /   %d min   /   %d builds"),S->Level,FMath::FloorToInt(S->Elapsed/60),S->Builds.Num());
}
bool AETCharacter::StoreSlot(UETSave* Profile,int32 N)
{
    if(!Profile||N<1||N>10) return false;
    return ETSaveDisk::Write(Profile,World->SlotName(N),true);
}
void AETCharacter::Slots()
{
    if(Panel!=TEXT("slots")) {
        SlotOrigin=Panel; SelectedSlot=World->ActiveSlot; SlotMessage.Empty();
        if(Panel==TEXT("pause")&&!World->Save()&&!World->Review) {PanelBody=TEXT("Saving the current run failed."); return;}
    }
    PendingImport=nullptr; PendingSlotAction.Empty();
    SlotLabels.Reset(); for(int32 I=1;I<=10;++I) SlotLabels.Add(SlotSummary(I));
    OpenPanel(TEXT("slots"),TEXT("Survivor saves"),TEXT(""));
}
void AETCharacter::SlotAction(const FString& Id)
{
    if(Id==TEXT("slot-back")) {
        PendingImport=nullptr; PendingSlotAction.Empty();
        if(SlotOrigin==TEXT("pause")) {Panel.Empty(); Pause();} else Title();
        return;
    }
    if(Id==TEXT("slot-cancel")) {PendingImport=nullptr; PendingSlotAction.Empty(); SlotMessage=TEXT("Cancelled."); return;}
    if(!PendingSlotAction.IsEmpty()&&Id!=TEXT("slot-confirm")) return;
    if(Id.StartsWith(TEXT("pick-"))) {SelectedSlot=FMath::Clamp(FCString::Atoi(*Id.Mid(5)),1,10); SlotMessage.Empty(); return;}
    const auto Key=World->SlotName(SelectedSlot);
    FString TestFolder;
    if(World->SlotQA) {
        TestFolder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()); FPaths::CollapseRelativeDirectories(TestFolder);
        FPaths::MakePlatformFilename(TestFolder);
    }
    auto Pick=[&](bool Write){
        FString Path=PickFile(Write,SelectedSlot,TestFolder);
        if(!TestFolder.IsEmpty()&&!Path.IsEmpty()&&!FPaths::IsSamePath(Path,FPaths::Combine(TestFolder,TEXT("Transfer survivor.etsave")))) {
            SlotMessage=TEXT("Test transfer must remain in its disposable save directory."); Path.Empty();
        }
        return Path;
    };
    const bool Occupied=UGameplayStatics::DoesSaveGameExist(Key,0);
    if(Id==TEXT("slot-play")) {
        if(Occupied&&SlotLabels[SelectedSlot-1]!=TEXT("Unreadable")) TravelSlot(SelectedSlot,true);
        else SlotMessage=TEXT("Choose an occupied, readable slot.");
        return;
    }
    if(Id==TEXT("slot-export")) {
        if(!Occupied) {SlotMessage=TEXT("Choose an occupied slot to export."); return;}
        auto* Data=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(Key,0));
        const FString Path=Pick(true);
        if(!Path.IsEmpty()) ExportProfile(Data,Path,SlotMessage);
        return;
    }
    FString Op=Id;
    if(Id==TEXT("slot-confirm")) Op=PendingSlotAction;
    if(Op==TEXT("slot-import")&&Id!=TEXT("slot-confirm")) {
        const FString Path=Pick(false);
        if(Path.IsEmpty()) return;
        PendingImport=ReadProfile(Path,SlotMessage);
        if(!PendingImport) return;
    }
    if(Op!=TEXT("slot-new")&&Op!=TEXT("slot-copy")&&Op!=TEXT("slot-import")) return;
    if(Op==TEXT("slot-copy")&&(SlotOrigin!=TEXT("pause")||SelectedSlot==World->ActiveSlot)) {SlotMessage=TEXT("During a run, choose a different slot to save a copy."); return;}
    if(Occupied&&Id!=TEXT("slot-confirm")) {
        PendingSlotAction=Op;
        SlotMessage=FString::Printf(TEXT("Replace slot %02d? Its current survivor will be backed up."),SelectedSlot); return;
    }
    UETSave* Source=Op==TEXT("slot-copy")?World->State:PendingImport;
    if(Op==TEXT("slot-new")) {
        Source=NewObject<UETSave>(this); Source->ArrivalStage=0; Source->RetroDisplay=false; Source->ModernDay=true; Source->ClockOffset=ETDay::At(9); Source->Seed=FMath::RandRange(1,MAX_int32-1); Source->LandscapeEdition=1; Source->BuildStock=0;
        auto* Old=World->State; Source->MusicVolume=Old->MusicVolume; Source->WeatherVolume=Old->WeatherVolume;
        Source->RetroDisplay=Old->RetroDisplay; Source->RenderScale=Old->RenderScale; Source->HasRingDisplay=true; Source->ViewDistance=Old->ViewDistance;
    }
    if(!StoreSlot(Source,SelectedSlot)) {SlotMessage=TEXT("Could not write the slot. Check available storage."); PendingSlotAction.Empty(); PendingImport=nullptr; return;}
    PendingSlotAction.Empty(); PendingImport=nullptr;
    if(Op==TEXT("slot-new")||SelectedSlot==World->ActiveSlot) {TravelSlot(SelectedSlot,Op==TEXT("slot-new")||SlotOrigin==TEXT("pause")); return;}
    SlotLabels[SelectedSlot-1]=SlotSummary(SelectedSlot); SlotMessage=FString::Printf(TEXT("Saved to slot %02d. Select Play to switch to it."),SelectedSlot);
}

