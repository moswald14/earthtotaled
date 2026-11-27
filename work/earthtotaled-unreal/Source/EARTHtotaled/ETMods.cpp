#include "ETGame.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace {
bool Slug(const FString& S)
{
    if(S.IsEmpty()||S.Len()>32||S[0]<'a'||S[0]>'z') return false;
    for(TCHAR C:S) if(!(C>='a'&&C<='z')&&!(C>='0'&&C<='9')&&C!='-') return false;
    return true;
}
bool DisplayName(const FString& S)
{
    if(S.TrimStartAndEnd().IsEmpty()||S.Len()>48) return false;
    for(TCHAR C:S) if(C<32||C==127) return false;
    return true;
}
bool Number(const FJsonObject& O,const TCHAR* Key,int32 Lo,int32 Hi,int32& Out)
{
    double N=0; if(!O.HasTypedField<EJson::Number>(Key)||!O.TryGetNumberField(Key,N)||!(N>=Lo&&N<=Hi)||FMath::FloorToDouble(N)!=N) return false;
    Out=int32(N);return true;
}
}
FString AETWorld::ModFolder()
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Mods"));
}
bool AETWorld::ParseMod(const FString& Json,FETModPack& Pack,FString& Error)
{
    Pack={}; Error=TEXT("Expected a mod JSON object under 64 KB.");
    TSharedPtr<FJsonObject> O;
    if(Json.IsEmpty()||Json.Len()>65536||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),O)||!O) return false;
    int32 Version=0; const TArray<TSharedPtr<FJsonValue>>* Finds=nullptr;
    Error=TEXT("Use schema 1, a lowercase ID, a short display name and one to eight finds.");
    if(O->Values.Num()!=4||!O->HasTypedField<EJson::String>(TEXT("id"))||!O->HasTypedField<EJson::String>(TEXT("name"))||!Number(*O,TEXT("schema"),1,1,Version)||!O->TryGetStringField(TEXT("id"),Pack.Id)||!Slug(Pack.Id)||
        !O->TryGetStringField(TEXT("name"),Pack.Name)||!DisplayName(Pack.Name)||!O->TryGetArrayField(TEXT("finds"),Finds)||!Finds||Finds->Num()<1||Finds->Num()>8) return false;
    const TSet<FString> Sources={TEXT("supertech"),TEXT("bug"),TEXT("zombie"),TEXT("wildlife"),TEXT("cache")};
    const TSet<FString> Uses={TEXT("wrappers"),TEXT("wood"),TEXT("stone"),TEXT("scrap"),TEXT("medkit"),TEXT("water")};
    TSet<FString> Ids;
    for(const auto& V:*Finds) {
        const TSharedPtr<FJsonObject>* Obj=nullptr;FETModFind F;FString Id;
        Error=FString::Printf(TEXT("Find %d needs a unique ID, name, source, reward, chance 1-100 and yield 1-25."),Pack.Finds.Num()+1);
        if(!V||!V->TryGetObject(Obj)||!Obj||!*Obj) return false;
        const auto& A=**Obj;
        if(A.Values.Num()!=6||!A.HasTypedField<EJson::String>(TEXT("id"))||!A.HasTypedField<EJson::String>(TEXT("name"))||!A.HasTypedField<EJson::String>(TEXT("source"))||!A.HasTypedField<EJson::String>(TEXT("use"))||!A.TryGetStringField(TEXT("id"),Id)||!Slug(Id)||Ids.Contains(Id)||
            !A.TryGetStringField(TEXT("name"),F.Name)||!DisplayName(F.Name)||!A.TryGetStringField(TEXT("source"),F.Source)||!Sources.Contains(F.Source)||
            !A.TryGetStringField(TEXT("use"),F.Use)||!Uses.Contains(F.Use)||!Number(A,TEXT("chance"),1,100,F.Chance)||!Number(A,TEXT("yield"),1,25,F.Yield)) return false;
        if(F.Source==TEXT("supertech")&&F.Use==TEXT("water")) {Error=TEXT("Supertech machines cannot supply water.");return false;}
        Ids.Add(Id); F.Key=TEXT("mod.")+Pack.Id+TEXT(".")+Id;Pack.Finds.Add(F);
    }
    Error.Empty();return true;
}
void AETWorld::LoadMods()
{
    Mods.Reset(); ModErrors.Reset();auto& FS=IFileManager::Get();
    const auto Dir=ModFolder();FS.MakeDirectory(*Dir,true);
    TArray<FString> Files; FS.FindFiles(Files,*(Dir/TEXT("*.earthmod.json")),true,false);Files.Sort();
    if(Files.Num()>64) {ModErrors.Add(TEXT("Only the first 64 files can load."));Files.SetNum(64);}
    TSet<FString> Seen;
    for(const auto& File:Files) {
        FETModPack Pack; FString Raw,Issue;const auto Path=Dir/File;
        if(FS.FileSize(*Path)>65536||!FFileHelper::LoadFileToString(Raw,*Path)||!ParseMod(Raw,Pack,Issue)||Seen.Contains(Pack.Id)) {
            ModErrors.Add(File+TEXT(" / ")+(Issue.IsEmpty()?TEXT("too large, unreadable or duplicate ID"):Issue));continue;
        }
        Seen.Add(Pack.Id);Pack.Enabled=!FS.FileExists(*(Dir/(Pack.Id+TEXT(".disabled"))));Mods.Add(MoveTemp(Pack));
    }
}
bool AETWorld::InstallMod(const FString& Raw,FString& Message)
{
    FETModPack Pack;if(!ParseMod(Raw,Pack,Message)) return false;
    LoadMods();const auto Dir=ModFolder(),Path=Dir/(Pack.Id+TEXT(".earthmod.json"));auto& FS=IFileManager::Get();
    TArray<FString> Files; FS.FindFiles(Files,*(Dir/TEXT("*.earthmod.json")),true,false);
    if(Files.Num()>=64||Mods.ContainsByPredicate([&](const FETModPack& P){return P.Id==Pack.Id;})||FS.FileExists(*Path)) {Message=TEXT("ID already installed or workshop full. Choose a new ID or edit the original file.");return false;}
    FTCHARToUTF8 Bytes(*Raw);if(Bytes.Length()>65536) {Message=TEXT("Mod exceeds 64 KB.");return false;}
    const FString Temp=Path+FGuid::NewGuid().ToString()+TEXT(".tmp");
    if(!FFileHelper::SaveStringToFile(Raw,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)||!FS.Move(*Path,*Temp,false)) {
        FS.Delete(*Temp); Message=TEXT("Could not save the mod to your Mods folder.");return false;
    }
    LoadMods();Message=Pack.Name+TEXT(" installed. Its finds are now in the world.");return true;
}
const FETModFind* AETWorld::ModFind(const FString& Key) const
{
    if(!Key.StartsWith(TEXT("mod."))) return nullptr;
    for(const auto& P:Mods) for(const auto& F:P.Finds) if(F.Key==Key) return &F;
    return nullptr;
}
void AETWorld::ModCache(const FString& Id,FString& Summary)
{
    for(const auto& P:Mods) if(P.Enabled) for(const auto& F:P.Finds) if(F.Source==TEXT("cache")) {
        FRandomStream Roll(GetTypeHash(Id+F.Key)^State->Seed);
        if(Roll.RandRange(1,100)<=F.Chance) {++State->Inventory.FindOrAdd(F.Key);Summary+=TEXT(" / ")+F.Name;}
    }
}
