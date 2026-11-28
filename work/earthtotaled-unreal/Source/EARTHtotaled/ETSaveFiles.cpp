#include "ETField.h"
#include "ETSaveDisk.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/FileManager.h"

namespace
{
    constexpr int32 Limit=32*1024*1024;
    FString Digest(const FString& S)
    {
        FTCHARToUTF8 Bytes(*S); FSHAHash Hash;
        FSHA1::HashBuffer(Bytes.Get(),Bytes.Length(),Hash.Hash); return Hash.ToString();
    }
    bool BoundedJson(const FString& S)
    {
        bool Quoted=false,Escape=false; int32 Depth=0,Members=0,Length=0;
        for(TCHAR C:S) {
            if(Quoted) {
                if(++Length>2048) return false;
                if(Escape) Escape=false;
                else if(C==TEXT('\\')) Escape=true;
                else if(C==TEXT('"')) Quoted=false;
            } else if(C==TEXT('"')) {Quoted=true; Length=0;}
            else if(C==TEXT('{')||C==TEXT('[')) {if(++Depth>12) return false;}
            else if(C==TEXT('}')||C==TEXT(']')) {if(--Depth<0) return false;}
            else if(C==TEXT(',')&&++Members>4000000) return false;
        }
        return !Quoted&&Depth==0;
    }
    bool Number(float F,float Min,float Max) {return FMath::IsFinite(F)&&F>=Min&&F<=Max;}
    bool Location(FVector V) {return !V.ContainsNaN()&&V.GetAbsMax()<2.e9;}
    bool Sensible(const UETSave& S,int32 Guns)
    {
        if(S.Furrows.Num()>4096||S.Run.Type<0||S.Run.Type>3||S.Run.Serial<0||S.Run.Serial>=MAX_int32||S.Run.Remaining.Num()>3||
           bool(S.Run.Type)!=!S.Run.Remaining.IsEmpty()||!Location(S.Run.Rally)||S.Streak<0||S.Streak>1000000||!FMath::IsFinite(S.LastHostileKill)||
           S.ScrapRemainder<0||S.ScrapRemainder>99||S.JazzTrack<0||S.JazzTrack>2)return false;
        for(const auto& K:S.Run.Remaining)if(!S.Threats.Contains(K)||!K.StartsWith(FString::Printf(TEXT("run/%d/"),S.Run.Serial)))return false;
        for(const auto& Pair:S.FarmStores)if(Pair.Value<0||Pair.Value>100000000)return false;
        for(const auto& Pair:S.SkillRanks)if(Pair.Value<1||Pair.Value>ETField::Cap(Pair.Key))return false;
        for(const auto& L:S.Furrows){const double D=FVector::Dist2D(L.Inlet,L.Outlet),Fall=L.Inlet.Z-L.Outlet.Z;
            if(!Location(L.Inlet)||!Location(L.Outlet)||D<294||D>1451||Fall<D*.004||Fall>D*.141)return false;}
        for(double Clock:{S.RingClock,S.EastRingClock,S.NorthLinkClock,S.SouthLinkClock})
            if(!FMath::IsFinite(Clock)||Clock<0||Clock>1000) return false;
        if(S.RailService<0||S.RailService>3||!Location(S.RingOffset)) return false;
        if(S.FrontLosses.Num()>4096||!Number(S.Shield,0,S.MaxShield())||!Number(S.ShieldDelay,0,10)||!FMath::IsFinite(S.ClockOffset)) return false;
        for(const auto& It:S.Towns) {const auto& T=It.Value;if(T.Wave<0||T.Wave>1000000||T.Dead<0||T.Dead>2047||T.Treasury<0||T.Defenses<0||T.Defenses>2||!Number(T.Upkeep,0,1.1e10)) return false;}
        for(const auto& It:S.TowerRefills) if(!Number(It.Value,0,1.1e10)) return false;
        for(const auto& Loss:S.FrontLosses) if(!Loss.Key.StartsWith(TEXT("front/"))||!Number(Loss.Value,0,1.1e10f)) return false;
        if(S.MapPins.Num()>ETNav::MaxPins||(S.TrackedPin!=INDEX_NONE&&!S.MapPins.IsValidIndex(S.TrackedPin))) return false;
        for(FVector2D Pin:S.MapPins) if(!Location(FVector(Pin,0))) return false;
        if(!S.Wake.Valid()||S.Schema!=1||!Location(S.Position)||!Location(S.WinchAnchor)||S.View.ContainsNaN()||
           S.ArmorTier<0||S.ArmorTier>3||!Number(S.Health,.00001f,1000)||!Number(S.Hydration,0,100)||
           !Number(S.Armor,0,S.ArmorLimit())||!Number(S.JetFuel,0,100)||!Number(S.Elapsed,0,1.e10f)||
           S.Weapon<0||S.Weapon>=Guns||S.Cores<0||S.Cores>4||S.Ammo.Num()>Guns||S.WeaponDamage.Num()>Guns||
           S.Level<1||S.Level>1000000||S.Water<0||S.Gold<0||S.Meds<0||S.Scrap<0||S.BuildStock<0||S.XP<0||S.Points<0||
           S.LandscapeEdition<0||S.LandscapeEdition>1||S.RaidStage<0||S.RaidStage>2||S.RaidSerial<0||S.Waves<0||
           !Number(S.RaidWait,-300,1.e7f)||!Number(S.RaidTime,-1,180)||!Number(S.NextPatrol,-1,1.e6f)||!Number(S.MaxDepth,0,1.e8f)||
           (S.RaidStage&&!S.Builds.Contains(S.RaidCore))||
           S.BoreCuts.Num()+S.SquareCuts.Num()>600000||S.SealedGalleries.Num()>600000||S.FallenRock.Num()>2000000||S.Builds.Num()>25000||S.Threats.Num()>4096) return false;
        if(S.ArrivalStage< -1||S.ArrivalStage>2||S.TrenchSides<0||S.TrenchSides>15||S.Relics.Num()>32768||(S.EquippedRelic!=INDEX_NONE&&!S.Relics.IsValidIndex(S.EquippedRelic))) return false;
        for(const auto& G:S.Relics) if(G.Seed<=0||G.Loaded<0||G.Loaded>5000||!Number(G.DamageDealt,0,1.e12f)) return false;
        for(int32 W:S.Unlocked) if(W<0||W>=Guns) return false;
        for(float XP:S.WeaponDamage) if(!Number(XP,0,1.e12f)) return false;
        for(int32 A:S.Ammo) if(A<0||A>5000) return false;
        for(auto& It:S.Inventory) if(It.Value<0||It.Value>100000000) return false;
        for(auto& It:S.DropPositions) if(!Location(It.Value)) return false;
        for(auto& It:S.Settled) if(!Location(It.Value)) return false;
        for(auto& It:S.AgentHealth) if(!Number(It.Value,0,1.e6f)) return false;
        for(auto& It:S.Builds) {
            auto& B=It.Value;
            const auto& F=B.Farm;
            if(F.Style<0||F.Style>2||F.Crop<0||F.Crop>3||!Number(F.Moisture,0,100)||!Number(F.Growth,0,1)||!Number(F.Time,0,B.Piece==AETConstruction::Bed?float(2*ETDay::Period):90.f)||F.Stock<0||F.Stock>8||F.Orders<0||F.Orders>4||F.Fertility<0||F.Fertility>3)return false;
            if(B.Piece==AETConstruction::Clinic&&F.Stock+F.Orders>4)return false;
            if(!It.Key.IsValid()||B.Piece<0||B.Piece>=AETConstruction::Count||B.Facing<0||B.Facing>3||B.MaterialTier<0||B.MaterialTier>2||!Location(B.Anchor)||
               !Number(B.Integrity,.0001f,10000)||!Number(B.Work,-60,180)||B.Iron<0||B.Iron>10000||B.GoldOre<0||B.GoldOre>10000) return false;
        }
        static const TSet<FString> Kinds={TEXT("robot"),TEXT("drone"),TEXT("runner"),TEXT("spitter"),TEXT("skimmer"),TEXT("bulwark"),TEXT("hive")};
        for(auto& It:S.Threats) if(!Kinds.Contains(It.Value.Kind)||!Number(It.Value.Health,.0001f,100000)||
            !Number(It.Value.AttackCooldown,-1,180)||!Location(It.Value.Position)) return false;
        for(const auto* Cuts:{&S.BoreCuts,&S.SquareCuts}) for(auto C:*Cuts)
            if(FMath::Abs(int64(C.X))>75000000||FMath::Abs(int64(C.Y))>75000000||FMath::Abs(int64(C.Z))>75000000) return false;
        for(auto C:S.FallenRock) if(FMath::Abs(int64(C.X))>20000000||FMath::Abs(int64(C.Y))>20000000||C.Z>=0||C.Z < -20000000) return false;
        return true;
    }
}
bool AETCharacter::ExportProfile(UETSave* Profile,const FString& Path,FString& Message)
{
    if(!Profile) {Message=TEXT("Choose an occupied slot to export."); return false;}
    if(!Sensible(*Profile,World->Weapons.Num())) {Message=TEXT("This world exceeds the portable save limits or includes unsupported data."); return false;}
    TSharedRef<FJsonObject> Data=MakeShared<FJsonObject>(); FString Body;
    if(!FJsonObjectConverter::UStructToJsonObject(UETSave::StaticClass(),Profile,Data,0,CPF_Transient)||
       !FJsonSerializer::Serialize(Data,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&Body))) {Message=TEXT("Could not encode survivor."); return false;}
    const FString Payload=TEXT("EARTHtotaled portable survivor 1\n")+Digest(Body)+TEXT("\n")+Body;
    if(FTCHARToUTF8(*Payload).Length()>Limit) {Message=TEXT("This survivor exceeds the 32 MB portable file limit."); return false;}
    if(!BoundedJson(Body)) {Message=TEXT("This world exceeds the portable save limits."); return false;}
    const FString Temp=Path+TEXT(".")+FGuid::NewGuid().ToString()+TEXT(".tmp");
    if(!FFileHelper::SaveStringToFile(Payload,*Temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)||
       !ETSaveDisk::Commit(Temp,Path)) {
        IFileManager::Get().Delete(*Temp,false); Message=TEXT("Export failed. Choose a writable folder."); return false;
    }
    Message=TEXT("Exported to ")+Path; return true;
}
UETSave* AETCharacter::ReadProfile(const FString& Path,FString& Message)
{
    Message=TEXT("Invalid, damaged, or unsupported EARTHtotaled save file.");
    const int64 Bytes=IFileManager::Get().FileSize(*Path);
    if(Bytes<=0||Bytes>Limit) return nullptr;
    FString File,Version,Rest,Checksum,Body;
    if(!FFileHelper::LoadFileToString(File,*Path)||!File.Split(TEXT("\n"),&Version,&Rest)||
       Version!=TEXT("EARTHtotaled portable survivor 1")||!Rest.Split(TEXT("\n"),&Checksum,&Body)||
       Checksum!=Digest(Body)||!BoundedJson(Body)) return nullptr;
    TSharedPtr<FJsonObject> Data; double Schema;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Body),Data)||!Data.IsValid()||
       !Data->TryGetNumberField(TEXT("schema"),Schema)||Schema!=1||!Data->HasField(TEXT("position"))||
       !Data->HasField(TEXT("builds"))||!Data->HasField(TEXT("inventory"))||!Data->HasField(TEXT("seed"))) return nullptr;
    // Decode only our known data class. Portable files never choose a UObject type.
    auto* Profile=NewObject<UETSave>(this);
    if(!FJsonObjectConverter::JsonObjectToUStruct(Data.ToSharedRef(),UETSave::StaticClass(),Profile,0,CPF_Transient)||
       !Sensible(*Profile,World->Weapons.Num())) return nullptr;
    Message=TEXT("Ready to import."); return Profile;
}

