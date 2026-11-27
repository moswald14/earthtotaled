#include "ETGame.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace {
FString Kit() {return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("ModWorkshop"));}
}
void AETCharacter::Workshop()
{
    if(Panel!=TEXT("title")&&Panel!=TEXT("pause")) return;
    WorkshopOrigin=Panel; WorkshopNote.Empty(); ProposedMod.Empty(); ModPage=0;
    World->LoadMods();RefreshWorkshop();
}
void AETCharacter::RefreshWorkshop()
{
    auto& Packs=World->Mods;ModPage=FMath::Clamp(ModPage,0,FMath::Max(0,Packs.Num()-1));
    FString Details=TEXT("Create a loot pack with your own ChatGPT or another AI. Preview its contents here before installing.");
    if(Packs.IsValidIndex(ModPage)) {
        const auto& Pack=Packs[ModPage];
        Details+=FString::Printf(TEXT("\n\n%d / %d    %s    /    %s"),ModPage+1,Packs.Num(),*Pack.Name,Pack.Enabled?TEXT("enabled"):TEXT("disabled"));
        for(const auto& F:Pack.Finds) Details+=FString::Printf(TEXT("\n%s  /  %s %d%%  /  %d %s  /  owned %d"),*F.Name,*F.Source,F.Chance,F.Yield,*F.Use,World->State->Inventory.FindRef(F.Key));
    } else Details+=TEXT("\n\nNo packs installed. Try the starter pack to add a new find to scavenging caches.");
    if(World->ModErrors.Num()) Details+=TEXT("\nFile skipped: ")+World->ModErrors[0];
    OpenPanel(TEXT("mods"),TEXT("MOD WORKSHOP"),Details);
}
void AETCharacter::WorkshopAction(const FString& Id)
{
    if(Id==TEXT("back")) {ClosePanel();return;}
    auto& Packs=World->Mods;auto& FS=IFileManager::Get();const auto Dir=World->ModFolder();
    if(Id==TEXT("discard")) {ProposedMod.Empty();WorkshopNote.Empty();RefreshWorkshop();return;}
    if(Id==TEXT("kit")) {FPlatformProcess::ExploreFolder(*Kit());return;}
    if(Id==TEXT("folder")) {FPlatformProcess::ExploreFolder(*Dir);return;}
    if(Id==TEXT("chatgpt")) {
        const auto Path=Kit()/TEXT("Create a mod with ChatGPT.cmd");
        if(!FS.FileExists(*Path)) WorkshopNote=TEXT("Author kit missing. Open the GitHub guide for another way to create a pack.");
        else FPlatformProcess::LaunchFileInDefaultExternalApplication(*Path);
    } else if(Id==TEXT("browser-ai")) {
        FString Brief;FFileHelper::LoadFileToString(Brief,*(Kit()/TEXT("AI-BRIEF.txt")));
        FPlatformApplicationMisc::ClipboardCopy(*Brief);FPlatformProcess::LaunchURL(TEXT("https://chatgpt.com/"),nullptr,nullptr);
        WorkshopNote=TEXT("Paste the copied instructions into your chat. Copy its JSON back here.");
    } else if(Id==TEXT("guide")) {FPlatformProcess::LaunchURL(TEXT("https://github.com/moswald14/earthtotaled/blob/main/work/earthtotaled-unreal/Content/ModWorkshop/README.md"),nullptr,nullptr);return;}
    else if(Id==TEXT("preview")||Id==TEXT("starter")) {
        FString Raw;if(Id==TEXT("starter")) FFileHelper::LoadFileToString(Raw,*(Kit()/TEXT("starter.earthmod.json")));
        else FPlatformApplicationMisc::ClipboardPaste(Raw);
        Raw.TrimStartAndEndInline();
        if(!Raw.IsEmpty()&&Raw[0]==TCHAR(0xFEFF)) {Raw.RightChopInline(1);Raw.TrimStartAndEndInline();}
        const FString Fence=TEXT("```");int32 LineEnd=INDEX_NONE;
        if(Raw.StartsWith(Fence)&&Raw.EndsWith(Fence)&&Raw.FindChar('\n',LineEnd)) {
            Raw=Raw.Mid(LineEnd+1,Raw.Len()-LineEnd-1-Fence.Len()).TrimStartAndEnd();
        }
        FETModPack Reading; ProposedMod.Empty();
        if(World->ParseMod(Raw,Reading,WorkshopNote)) {
            ProposedMod=Raw;WorkshopNote=FString::Printf(TEXT("PREVIEW  /  %s (%s)"),*Reading.Name,*Reading.Id);
            for(const auto& F:Reading.Finds) WorkshopNote+=FString::Printf(TEXT("\n%s: %d%% %s, redeem for %d %s"),*F.Name,F.Chance,*F.Source,F.Yield,*F.Use);
        }
    } else if(Id==TEXT("install")&&!ProposedMod.IsEmpty()) {
        if(World->InstallMod(ProposedMod,WorkshopNote)) {FETModPack P;FString E;World->ParseMod(ProposedMod,P,E);ModPage=Packs.IndexOfByPredicate([&](const FETModPack& M){return M.Id==P.Id;});ProposedMod.Empty();}
    } else if(Id==TEXT("refresh")) {World->LoadMods();WorkshopNote=TEXT("Reread the Mods folder.");}
    else if(Packs.IsValidIndex(ModPage)) {
        const auto& P=Packs[ModPage];const auto Marker=Dir/(P.Id+TEXT(".disabled"));
        if(Id==TEXT("previous")||Id==TEXT("next")) {ModPage=(ModPage+Packs.Num()+(Id==TEXT("next")?1:-1))%Packs.Num();WorkshopNote.Empty();ProposedMod.Empty();}
        else if(Id==TEXT("toggle")) {
            const bool OK=P.Enabled?FFileHelper::SaveStringToFile(TEXT("disabled"),*Marker):FS.Delete(*Marker);
            WorkshopNote=OK?TEXT("Pack setting saved. Existing finds stay in the survivor pack."):TEXT("Could not change the pack setting."); World->LoadMods();
        } else if(Id==TEXT("redeem")&&WorkshopOrigin==TEXT("pause")) {
            auto& S=*World->State;int Redeemed=0;
            for(const auto& F:P.Finds) {const int N=S.Inventory.FindRef(F.Key);if(N<=0) continue;
                const int Count=int(FMath::Min<int64>(int64(N)*F.Yield,10000000));S.Inventory.Remove(F.Key);Redeemed+=N;
                if(F.Use==TEXT("wrappers")) S.Gold+=Count;else if(F.Use==TEXT("water")) S.Water+=Count;
                else if(F.Use==TEXT("medkit")) S.Meds+=Count;else if(F.Use==TEXT("scrap")) S.Scrap+=Count;else S.Inventory.FindOrAdd(F.Use)+=Count;
            }
            if(Redeemed) World->Save();
            WorkshopNote=FString::Printf(TEXT("Redeemed %d finds from this pack."),Redeemed);
        }
    }
    RefreshWorkshop();
}
