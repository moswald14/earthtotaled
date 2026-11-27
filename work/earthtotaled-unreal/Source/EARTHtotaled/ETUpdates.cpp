#include "ETGame.h"
#include "ETLandfall.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"

namespace {
    bool ReadVersion(const FString& Tag,FIntVector& Number) {
        if(!Tag.StartsWith(TEXT("v"))) return false;
        TArray<FString> Parts;Tag.Mid(1).ParseIntoArray(Parts,TEXT("."),false); if(Parts.Num()!=3) return false;
        for(int I=0;I<3;++I) if(!LexTryParseString(Number[I],*Parts[I])||Number[I]<0||FString::FromInt(Number[I])!=Parts[I]) return false;
        return true;
    }
    bool Newer(const FString& A,const FString& B) {
        FIntVector L,R;if(!ReadVersion(A,L)||!ReadVersion(B,R)) return false;
        return L.X!=R.X?L.X>R.X:L.Y!=R.Y?L.Y>R.Y:L.Z>R.Z;
    }
}
void AETCharacter::ShowVersions()
{
    FString Body=FString(TEXT("Installed   "))+ETLandfall::Version+TEXT("\nSource   github.com/moswald14/earthtotaled\n\n");
    if(ReleaseTags.IsValidIndex(ReleaseIndex)) {
        const auto Tag=ReleaseTags[ReleaseIndex];Body+=FString::Printf(TEXT("Selected   %s     (%d of %d)\n\n"),*Tag,ReleaseIndex+1,ReleaseTags.Num());
        Body+=ConfirmVersion?TEXT("Save this survivor, close the game and install this version? Setup backs up your slots and starts the selected game. Older versions play on a separate copy of your saves."):
            TEXT("Choose a release below. You can stay on this version and keep playing.");
    }
    if(!UpdateNote.IsEmpty()) Body+=TEXT("\n")+UpdateNote;
    OpenPanel(TEXT("updates"),ConfirmVersion?TEXT("Install this release?"):TEXT("GAME UPDATES"),Body);
}
void AETCharacter::CheckVersions(bool Automatic)
{
    if(Automatic&&!FCString::Strifind(FCommandLine::Get(),TEXT("-UserDir="))) {
        const auto Managed=FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"))/TEXT("Programs/EARTHtotaled");
        const auto Executable=FPaths::ConvertRelativePathToFull(FPlatformProcess::ExecutablePath());
#if PLATFORM_WINDOWS
        // A legacy Setup may just have installed this game and copied its old version manager to Start.
        const FString CurrentDir=Managed/TEXT("Versions")/ETLandfall::Version;
        const FString Shipping=CurrentDir/TEXT("EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe");
        const FString Setup=CurrentDir/TEXT("EARTHtotaled/Content/UpdateTools/EARTHtotaled-Setup.exe");
        static bool CheckedLauncher=false;
        if(!CheckedLauncher&&FPaths::IsSamePath(Executable,Shipping)) {
            CheckedLauncher=true;
            if(FPlatformMisc::GetFileVersion(Setup)>FPlatformMisc::GetFileVersion(Managed/TEXT("EARTHtotaled.exe"))) {
                auto Repair=FPlatformProcess::CreateProc(*Setup,TEXT("--refresh-launcher"),true,true,true,nullptr,0,nullptr,nullptr);
                if(Repair.IsValid())FPlatformProcess::CloseProc(Repair);
            }
        }
#endif
        TArray<FString> Origins;FFileHelper::LoadFileToStringArray(Origins,*(Managed/TEXT("portable-origins.txt")));
        if(Origins.ContainsByPredicate([&](const FString& Origin){return FPaths::IsSamePath(Origin,Executable);})&&FPaths::FileExists(Managed/TEXT("active.txt"))) {
            auto H=FPlatformProcess::CreateProc(*(Managed/TEXT("EARTHtotaled.exe")),TEXT(""),true,false,false,nullptr,0,nullptr,nullptr);
            if(H.IsValid()){FPlatformProcess::CloseProc(H);FPlatformMisc::RequestExit(false);return;}
        }
    }
    static double LastAutomatic=-1.e8;
    if(CheckingVersions||(Automatic&&FPlatformTime::Seconds()-LastAutomatic<14400)) return;
    if(Automatic) LastAutomatic=FPlatformTime::Seconds();
    CheckingVersions=true; ConfirmVersion=false; UpdateNote=TEXT("Checking public Windows releases...");
    if(!Automatic) ShowVersions();
    ReadReleasePage(1,Automatic);
}
void AETCharacter::ReadReleasePage(int Page,bool Automatic)
{
    // Real release-list pages start at one. The separate latest-release file has one row.
    constexpr int LatestReleaseFile=0;
    const bool LatestOnly=Page==LatestReleaseFile;
    auto Req=FHttpModule::Get().CreateRequest();
    Req->SetURL(LatestOnly?FString(TEXT("https://github.com/moswald14/earthtotaled/releases/latest/download/EARTHtotaled-Windows-update.json")):FString::Printf(TEXT("https://api.github.com/repos/moswald14/earthtotaled/releases?per_page=100&page=%d"),Page));
    Req->SetVerb(TEXT("GET"));Req->SetHeader(TEXT("User-Agent"),TEXT("EARTHtotaled-Windows"));Req->SetHeader(TEXT("Accept"),TEXT("application/vnd.github+json"));Req->SetTimeout(14);
    auto Handled=MakeShared<bool>(false);
    auto Complete=[this,Automatic,Page,LatestOnly,Handled](FHttpRequestPtr,FHttpResponsePtr Res,bool OK) {
        if(*Handled)return;
        *Handled=true;
        TArray<TSharedPtr<FJsonValue>> Rows;
        if(OK&&Res&&Res->GetResponseCode()==200&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Res->GetContentAsString()),Rows)) {
            if(Page<=1){ReleaseTags.Reset();ReleaseIndex=0;}
            for(auto& V:Rows) {
                const auto O=V->AsObject(); if(!O||O->GetBoolField(TEXT("draft"))||O->GetBoolField(TEXT("prerelease"))) continue;
                const auto Tag=O->GetStringField(TEXT("tag_name"));FIntVector Num;if(!ReadVersion(Tag,Num)) continue;
                const FString Zip=TEXT("EARTHtotaled-Unreal-Windows-")+Tag+TEXT(".zip");const TArray<TSharedPtr<FJsonValue>>* Assets=nullptr;
                if(!O->TryGetArrayField(TEXT("assets"),Assets)) continue;
                for(auto& A:*Assets) {auto File=A->AsObject();FString Digest,Name;
                    if(File&&File->TryGetStringField(TEXT("name"),Name)&&Name==Zip&&File->TryGetStringField(TEXT("digest"),Digest)&&Digest.StartsWith(TEXT("sha256:"))&&Digest.Len()==71) {ReleaseTags.AddUnique(Tag);break;}
                }
            }
            if(!LatestOnly&&Rows.Num()==100){ReadReleasePage(Page+1,Automatic);return;}
            ReleaseTags.Sort([](const FString& A,const FString& B){return Newer(A,B);});
            UpdateNote=ReleaseTags.IsEmpty()?TEXT("No installable Windows releases found."):LatestOnly?TEXT("Showing the latest Windows release."):TEXT("");
        } else if(Page==1) {
            ReadReleasePage(LatestReleaseFile,Automatic);
            return;
        } else UpdateNote=TEXT("Could not reach GitHub. You can return to play or try again later.");
        CheckingVersions=false;
        if(Panel==TEXT("updates")) ShowVersions();
        else if(Automatic&&Panel==TEXT("title")&&!ReleaseTags.IsEmpty()&&Newer(ReleaseTags[0],ETLandfall::Version)) {UpdateOrigin=Panel;ShowVersions();}
    };
    Req->OnProcessRequestComplete().BindWeakLambda(this,Complete);
    if(!Req->ProcessRequest()) Complete(Req,nullptr,false);
}
void AETCharacter::UpdateAction(const FString& Id)
{
    if(Id==TEXT("later")) {ConfirmVersion=false;Panel.Empty();if(UpdateOrigin==TEXT("pause")) Pause();else Title();return;}
    if(Id==TEXT("retry")) {CheckVersions(false);return;}
    if(!ReleaseTags.IsValidIndex(ReleaseIndex)) return;
    if(Id==TEXT("prev")||Id==TEXT("next")) {ReleaseIndex=FMath::Clamp(ReleaseIndex+(Id==TEXT("next")?1:-1),0,ReleaseTags.Num()-1);ConfirmVersion=false;}
    else if(Id==TEXT("choose")) ConfirmVersion=true;
    else if(Id==TEXT("install")&&ConfirmVersion) {
        if(UpdateOrigin==TEXT("pause")&&!World->Save()) {UpdateNote=TEXT("Saving failed. Your game is still open.");ShowVersions();return;}
        const auto Setup=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("UpdateTools/EARTHtotaled-Setup.exe"));
        const auto Args=FString::Printf(TEXT("--install %s --from %s --wait %u --origin \"%s\""),*ReleaseTags[ReleaseIndex],ETLandfall::Version,FPlatformProcess::GetCurrentProcessId(),*FPaths::ConvertRelativePathToFull(FPlatformProcess::ExecutablePath()));
        auto Handle=FPlatformProcess::CreateProc(*Setup,*Args,true,false,false,nullptr,0,nullptr,nullptr);
        if(Handle.IsValid()) {FPlatformProcess::CloseProc(Handle);FPlatformMisc::RequestExit(false);return;}
        UpdateNote=TEXT("Setup could not start. Run EARTHtotaled-Setup.exe from the GitHub release.");
    }
    ShowVersions();
}
