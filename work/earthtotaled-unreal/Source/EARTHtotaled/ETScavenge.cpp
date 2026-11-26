#include "ETGame.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

void AETWorld::LoadExpansion()
{
    FString Text; TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Data/expansion.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root) return;
    for(auto V:Root->GetArrayField(TEXT("weapons"))) {
        auto O=V->AsObject(); FETWeapon W;
        W.Name=O->GetStringField(TEXT("name")); W.Mag=O->GetIntegerField(TEXT("mag")); W.Reserve=O->GetIntegerField(TEXT("reserve"));
        W.Damage=O->GetNumberField(TEXT("damage")); W.Rate=O->GetNumberField(TEXT("rate")); W.Reload=O->GetNumberField(TEXT("reload"));
        W.Spread=O->GetNumberField(TEXT("spread")); W.Pellets=O->GetIntegerField(TEXT("pellets")); W.Range=O->GetNumberField(TEXT("range"))*100; Weapons.Add(W);
    }
    for(auto V:Root->GetArrayField(TEXT("items"))) {
        auto O=V->AsObject(); FETSupply S; S.Id=O->GetStringField(TEXT("id")); S.Name=O->GetStringField(TEXT("name"));
        S.Description=O->GetStringField(TEXT("description")); S.Usable=O->GetBoolField(TEXT("usable")); Supplies.Add(S);
    }
    for(auto V:Root->GetArrayField(TEXT("recipes"))) {
        auto O=V->AsObject(); FETRecipe R; R.Id=O->GetStringField(TEXT("id")); R.Name=O->GetStringField(TEXT("name")); R.Output=O->GetStringField(TEXT("output"));
        R.Scrap=O->GetIntegerField(TEXT("scrap")); R.Count=O->GetIntegerField(TEXT("count"));
        for(auto It:O->GetObjectField(TEXT("ingredients"))->Values) R.Ingredients.Add(FString(*It.Key),int32(It.Value->AsNumber())); Recipes.Add(R);
    }
    for(auto It:Root->GetObjectField(TEXT("lootTables"))->Values) {
        TArray<FETLootEntry> Entries;
        for(auto V:It.Value->AsArray()) {auto O=V->AsObject(); FETLootEntry E; E.Id=O->GetStringField(TEXT("id")); E.Min=O->GetIntegerField(TEXT("min")); E.Max=O->GetIntegerField(TEXT("max")); Entries.Add(E);}
        LootTables.Add(FString(*It.Key),Entries);
    }
    // Keep old saves and weapon indices. New inventory fields default to empty.
    State->ViewDistance=FMath::Clamp(State->ViewDistance,350,800);
    for(const TCHAR* Name:{TEXT("Reload"),TEXT("Empty"),TEXT("Pickup"),TEXT("Drink"),TEXT("Craft"),TEXT("Heal"),TEXT("StepForest"),TEXT("StepCity"),TEXT("Hit"),TEXT("Robot"),TEXT("Animal"),TEXT("Switch"),TEXT("ForestDay"),TEXT("ForestNight"),TEXT("CityWind")}) {
        FString Id=Name; Sounds.Add(Id,LoadObject<USoundBase>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/S_")+Id+TEXT(".S_")+Id)));
    }
    for(const FString& Id:TArray<FString>{TEXT("BorerPulse"),TEXT("OreChime"),TEXT("JetThrust"),TEXT("ShardChirp"),TEXT("BroodCall"),TEXT("FieldLatch")}) {
        Sounds.Add(Id,LoadObject<USoundBase>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/S_")+Id+TEXT(".S_")+Id)));
    }
    LoadDialogue();
}
void AETWorld::Sound(const FString& Name,float Volume,float Pitch)
{
    if(auto S=Sounds.FindRef(Name)) UGameplayStatics::PlaySound2D(this,S,Volume,Pitch);
}
void AETWorld::Loot(AETCharacter* P,const FString& Id,const FString& Table)
{
    if(!P||State->Consumed.Contains(Id)) return;
    const auto* Entries=LootTables.Find(Table); if(!Entries||Entries->IsEmpty()) return;
    FRandomStream R(GetTypeHash(Id)^uint32(State->Seed));
    int32 First=R.RandRange(0,Entries->Num()-1), Second=(First+R.RandRange(1,Entries->Num()-1))%Entries->Num();
    FString Summary;
    for(int32 Index:{First,Second}) {
        const auto& E=(*Entries)[Index]; int32 N=R.RandRange(E.Min,E.Max); FString Name=E.Id;
        if(E.Id==TEXT("water")) {State->Water+=N; Name=TEXT("sealed water");}
        else if(E.Id==TEXT("medkit")) {State->Meds+=N; Name=TEXT("medkit");}
        else if(E.Id==TEXT("buildkit")) {State->BuildStock+=N; Name=TEXT("building kits");}
        else {State->Inventory.FindOrAdd(E.Id)+=N; for(auto S:Supplies) if(S.Id==E.Id) Name=S.Name;}
        if(!Summary.IsEmpty()) Summary+=TEXT(" / "); Summary+=FString::Printf(TEXT("%d %s"),N,*Name);
    }
    const int32 Scrap=FMath::RoundToInt(R.RandRange(4,9)*P->LootRate()); State->Scrap+=Scrap;
    if(Table==TEXT("armory")&&R.RandRange(0,7)==0) {
        const int32 Gun=R.RandRange(6,9); if(!State->Unlocked.Contains(Gun)) {State->Unlocked.Add(Gun); Summary+=TEXT(" / FOUND ")+Weapons[Gun].Name;}
    }
    P->AddXP(20); Say(Summary+FString::Printf(TEXT(" / %d scrap"),Scrap)); State->Consumed.Add(Id);
}
void AETCharacter::Inventory()
{
    if(!Panel.IsEmpty()&&Panel!=TEXT("inventory")) return;
    if(Panel==TEXT("inventory")) {ClosePanel(); return;}
    OpenPanel(TEXT("inventory"),TEXT("PACK / SUPPLIES & ARSENAL"),TEXT("Select a supply to use it. Craft with recovered materials at any workbench."));
}
void AETCharacter::UseSupply(const FString& Id)
{
    auto S=World->State; if(S->Inventory.FindRef(Id)<=0) return;
    if(Id==TEXT("food")||Id==TEXT("bandage")) {
        if(S->Health>=MaxHealth()) {PanelBody=TEXT("Health is already full."); return;}
        S->Health=FMath::Min(MaxHealth(),S->Health+(Id==TEXT("food")?15:25));
    } else if(Id==TEXT("armorplate")) {
        if(S->Armor>=S->ArmorLimit()) {PanelBody=TEXT("Your suit is already fully repaired."); return;} S->Armor=FMath::Min(S->ArmorLimit(),S->Armor+35);
    } else if(Id==TEXT("stim")) S->StimTime=180;
    else return;
    --S->Inventory[Id]; World->Sound(TEXT("Heal")); PanelBody=TEXT("Supply used. Your pack is saved."); World->Save();
}
void AETCharacter::Craft(const FString& Id)
{
    if(Panel!=TEXT("bench")) return; auto S=World->State;
    for(const auto& R:World->Recipes) if(R.Id==Id) {
        bool Enough=S->Scrap>=R.Scrap; for(auto It:R.Ingredients) Enough&=S->Inventory.FindRef(It.Key)>=It.Value;
        if(!Enough) {PanelBody=TEXT("Missing materials. Search toolboxes, clinics, kitchens and electronics stores."); return;}
        for(auto It:R.Ingredients) S->Inventory[It.Key]-=It.Value; S->Scrap-=R.Scrap;
        if(R.Output==TEXT("buildkit")) S->BuildStock+=R.Count;
        else if(R.Output==TEXT("medkit")) S->Meds+=R.Count;
        else S->Inventory.FindOrAdd(R.Output)+=R.Count;
        PanelBody=TEXT("Crafted: ")+R.Name; World->Sound(TEXT("Craft")); World->Save(); return;
    }
}
void AETCharacter::Weapon7(){SelectWeapon(6);} void AETCharacter::Weapon8(){SelectWeapon(7);}
void AETCharacter::Weapon9(){SelectWeapon(8);} void AETCharacter::Weapon10(){SelectWeapon(9);}
void AETCharacter::NextWeapon(){CycleWeapon(1);} void AETCharacter::PreviousWeapon(){CycleWeapon(-1);}
void AETCharacter::CycleWeapon(int32 Direction)
{
    if(!World||!Panel.IsEmpty()) return;
    if(Building) {BuildPiece=(BuildPiece+Direction+AETConstruction::Count)%AETConstruction::Count; RefreshBuild(); return;}
    for(int32 N=1;N<=World->Weapons.Num();++N) {
        int32 I=(World->State->Weapon+N*Direction+World->Weapons.Num())%World->Weapons.Num();
        if(World->State->Unlocked.Contains(I)) {SelectWeapon(I); return;}
    }
}
