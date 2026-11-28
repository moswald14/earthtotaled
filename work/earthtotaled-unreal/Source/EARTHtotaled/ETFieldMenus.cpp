#include "ETField.h"
#include "ETWake.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"

using Build=AETConstruction;
const TCHAR* ETField::Track(int N){
    static const TCHAR* const Tracks[]={TEXT("Saltline Swing"),TEXT("Pump House Special"),TEXT("Night Furrow")};
    return Tracks[FMath::Clamp(N,0,2)];
}
void AETCharacter::MusicPanel(){
    auto& S=*World->State;
    FString Info=FString::Printf(TEXT("%s / volume %d%%\n\n"),S.MusicMuted?TEXT("Muted"):TEXT("Music enabled"),S.MusicVolume);
    if(S.JazzSource) Info+=FString::Printf(TEXT("Doodle Jazz / %s\nOriginal local trio recordings. Choose a track below."),ETField::Track(S.JazzTrack));
    else Info+=TEXT("World score / the soundtrack changes with wilderness, city and night.");
    OpenPanel(TEXT("music"),TEXT("MUSIC"),Info);
}
void AETCharacter::FarmPanel(const FString& Note){
    auto& S=*World->State;auto* B=S.Builds.Find(DeviceId);if(!B||B->Piece<Build::Cistern){ClosePanel();return;}
    const auto& F=B->Farm;FString Info;
    if(B->Piece==Build::Cistern){
        Info=FString::Printf(TEXT("%s. Sealed water %d / 8. Each bottle takes 90 seconds of world time.\n\nCarve an inlet 4-6 m from this tank. Connect segments downhill toward prepared beds. Water keeps flowing when storage fills."),
            World->PumpLinked(*B)?TEXT("Pump connected"):TEXT("Offline / build a pump within 12 m"),F.Stock);
    }else if(B->Piece==Build::Pump){
        bool Linked=false;for(const auto& V:S.Builds)if(V.Value.Piece==Build::Cistern&&FVector::Dist(V.Value.Anchor,B->Anchor)<=1250){Linked=true;break;}
        Info=Linked?TEXT("Pump online. Collect sealed water at the nearby cistern."):TEXT("Add a cistern within 12 m on this allotment. Pump and cistern share a buried pipe.");
    }else if(B->Piece==Build::Clinic){
        Info=FString::Printf(TEXT("Medpacks %d / 4. Orders %d. Each order takes 75 seconds and uses 1 Mendleaf and 2 scrap.\n\nAvailable: %d Mendleaf, %d scrap. Collect here, or apply one finished pack directly."),F.Stock,F.Orders,S.FarmStores.FindRef(TEXT("leaves")),S.Scrap);
    }else{
        static const TCHAR* const StageNames[]={TEXT("prepared soil"),TEXT("sprout"),TEXT("leafing"),TEXT("flowering"),TEXT("ripe")};
        Info=FString::Printf(TEXT("%s / %s / %d%% grown. Moisture %.0f%%. Soil %d/3. %s\n\nSeeds: %d Cinderbean, %d Saltroot, %d Mendleaf. Cinderbean matures quickly and improves soil. Saltroot gives three rations and likes mineral soil. Mendleaf supplies the clinic. Dry plants wait for water."),
            ETField::Crop(F.Crop),StageNames[ETField::Stage(F)],FMath::RoundToInt(F.Growth*100),F.Moisture,F.Fertility,
            ETField::Mineral(FVector2D(B->Anchor),S.Seed)||F.Fertility>=3?TEXT("Mineral-rich."):TEXT("Ordinary soil."),
            S.FarmStores.FindRef(TEXT("Cinderbean")),S.FarmStores.FindRef(TEXT("Saltroot")),S.FarmStores.FindRef(TEXT("Mendleaf")));
    }
    if(B->Piece==Build::Bed&&F.Crop&&F.Moisture<=0)Info+=FString::Printf(TEXT("\nDormant. Water within %d minutes of play before the crop goes to seed."),FMath::CeilToInt((2*ETDay::Period-F.Time)/60.));
    if(!Note.IsEmpty())Info+=TEXT("\n\n")+Note;
    OpenPanel(TEXT("farm"),ETField::FarmName(B->Piece,F.Style),Info);
}
void AETCharacter::FieldAction(const FString& Id){
    auto& S=*World->State;
    if(Id==TEXT("field-music")&&Panel==TEXT("pause")){MusicPanel();return;}
    if(Panel==TEXT("music")){
        if(Id==TEXT("field-back")){Panel.Empty();Pause();return;}
        if(Id==TEXT("field-mute"))S.MusicMuted=!S.MusicMuted;
        else if(Id==TEXT("field-volume-up"))S.MusicVolume=FMath::Min(100,S.MusicVolume+10);
        else if(Id==TEXT("field-volume-down"))S.MusicVolume=FMath::Max(0,S.MusicVolume-10);
        else if(Id==TEXT("field-source")&&World->Music.IsValidIndex(5)&&World->Music[3]->Sound)S.JazzSource=!S.JazzSource;
        else if(S.JazzSource&&(Id==TEXT("field-next")||Id==TEXT("field-prev")))S.JazzTrack=(S.JazzTrack+(Id==TEXT("field-next")?1:2))%3;
        World->UpdateAtmosphereAudio(0,true);World->Save();MusicPanel();return;
    }
    if(Panel==TEXT("threatruns")){
        if(Id.StartsWith(TEXT("field-run"))&&!S.Run.Type){
            if(!StartThreatRun(FCString::Atoi(*Id.Right(1)))){ThreatPanel();PanelBody+=TEXT("\nNo clear staging ground nearby. Try another relay.");return;}
        }else if(Id==TEXT("field-abandon")&&S.Run.Type){
            const FString Prefix=FString::Printf(TEXT("run/%d/"),S.Run.Serial);
            for(auto It=S.Threats.CreateIterator();It;++It)if(It.Key().StartsWith(Prefix)){
                if(auto* A=World->LiveThreats.FindRef(It.Key());IsValid(A))A->Destroy();
                World->LiveThreats.Remove(It.Key());It.RemoveCurrent();
            }
            S.Run.Remaining.Empty();S.Run.Type=0;
        }else if(Id==TEXT("field-mark")&&S.Run.Type){
            const FVector2D At(S.Run.Rally);int N=S.MapPins.IndexOfByKey(At);
            if(N==INDEX_NONE&&S.MapPins.Num()<ETNav::MaxPins)N=S.MapPins.Add(At);
            if(N==INDEX_NONE){PanelBody+=TEXT("\nMap full. Remove a pin on the map to mark this run.");return;}
            S.TrackedPin=N;
        }
        World->Save();ThreatPanel();return;
    }
    if(Panel!=TEXT("farm"))return;
    auto* B=S.Builds.Find(DeviceId);if(!B){ClosePanel();return;}auto& F=B->Farm;FString Note;
    if(Id==TEXT("field-furrow")&&(B->Piece==Build::Pump||B->Piece==Build::Cistern)){
        ClosePanel();StopBuilding();if(S.Wake.Body==ETWake::Bug){Borer=true;Firing=false;}else if(!Borer)ToggleBorer();
        S.FurrowHead=true;S.SquareBorer=false;FurrowStart.Reset();
        World->Say(TEXT("Mark an inlet near the cistern, then tap a downstream endpoint. G stows."));World->Save();return;
    }
    if(B->Piece==Build::Cistern&&Id==TEXT("field-bottles")){
        if(S.Wake.Body==ETWake::Bug)S.FarmStores.FindOrAdd(TEXT("bottles"))+=F.Stock;else S.Water+=F.Stock;
        Note=FString::Printf(TEXT("Collected %d sealed bottles%s."),F.Stock,S.Wake.Body==ETWake::Bug?TEXT(" for irrigation"):TEXT(""));F.Stock=0;
    }else if(B->Piece==Build::Clinic){
        if(Id==TEXT("field-order")){
            if(F.Stock+F.Orders>=4)Note=TEXT("All four storage spaces are reserved.");
            else if(S.FarmStores.FindRef(TEXT("leaves"))<1||S.Scrap<2)Note=TEXT("An order needs one harvested Mendleaf and two scrap.");
            else{--S.FarmStores.FindOrAdd(TEXT("leaves"));S.Scrap-=2;++F.Orders;Note=TEXT("Reclamation cycle queued.");}
        }else if(Id==TEXT("field-collect")&&F.Stock&&S.Wake.Body!=ETWake::Bug){S.Meds+=F.Stock;F.Stock=0;Note=TEXT("Medpacks stored in your pack.");}
        else if(Id==TEXT("field-treat")&&F.Stock&&S.Health<MaxHealth()){--F.Stock;S.Health=FMath::Min(MaxHealth(),S.Health+55+(Has(TEXT("medic"))?25:0));Note=TEXT("Field dressing applied.");}
    }else if(B->Piece==Build::Bed){
        if(Id==TEXT("field-seeds")){
            const FString Key=TEXT("field.seedkit");
            if(!S.Consumed.Contains(Key)){
                for(int I=1;I<=3;++I)S.FarmStores.FindOrAdd(ETField::Crop(I))+=3;
                S.Consumed.Add(Key);Note=TEXT("Nine starter seeds stored for all your beds.");
            }else Note=TEXT("Harvest your crops for more seed. Buried service hatches also carry Saltroot.");
        }else if(Id.StartsWith(TEXT("field-plant"))&&!F.Crop){
            const int K=FCString::Atoi(*Id.Right(1));if(K>=1&&K<=3&&S.FarmStores.FindRef(ETField::Crop(K))>0){
                --S.FarmStores.FindOrAdd(ETField::Crop(K));F.Crop=K;F.Growth=0;F.Time=0;Note=TEXT("Seed planted.");
            }else Note=TEXT("Collect starter seeds or harvest a ripe bed.");
        }else if(Id==TEXT("field-water")&&F.Moisture<100){
            auto& Bottles=S.FarmStores.FindOrAdd(TEXT("bottles"));
            if(Bottles>0){--Bottles;F.Moisture=100;F.Time=0;}
            else if(S.Wake.Body!=ETWake::Bug&&S.Water>0){--S.Water;F.Moisture=100;F.Time=0;}
            else Note=TEXT("Collect a sealed bottle or bring a furrow within 4 m of this bed.");
        }else if(Id==TEXT("field-silt")&&S.FarmStores.FindRef(TEXT("silt"))>0&&F.Fertility<3){--S.FarmStores.FindOrAdd(TEXT("silt"));F.Fertility=3;}
        else if(Id==TEXT("field-harvest")&&ETField::Stage(F)==4){
            const int K=F.Crop,Yield=K==2?3:K==3?2:1;
            if(K==3) S.FarmStores.FindOrAdd(TEXT("leaves"))+=Yield;
            else if(S.Wake.Body!=ETWake::Bug)S.Inventory.FindOrAdd(TEXT("food"))+=Yield;
            else S.FarmStores.FindOrAdd(TEXT("rations"))+=Yield;
            int& Harvests=S.FarmStores.FindOrAdd(TEXT("harvests"));++Harvests;
            const int Seeds=1+(K==1||Harvests%3==0?1:0);S.FarmStores.FindOrAdd(ETField::Crop(K))+=Seeds;
            if(K==1)F.Fertility=FMath::Min(3,F.Fertility+1);
            F.Crop=0;F.Growth=0;F.Time=0;AddXP(12);
            Note=FString::Printf(TEXT("Harvested %d %s and %d seed%s."),Yield,K==3?TEXT("Mendleaf"):TEXT("ration(s)"),Seeds,Seeds==1?TEXT(""):TEXT("s"));
        }else if(Id==TEXT("field-feed")&&S.Wake.Body==ETWake::Bug&&S.FarmStores.FindRef(TEXT("rations"))>0){
            --S.FarmStores.FindOrAdd(TEXT("rations"));S.StimTime=FMath::Max(S.StimTime,90.f);Note=TEXT("Fed from the garden stores.");
        }
        if(auto* A=World->LiveBuilds.FindRef(DeviceId).Get())A->Configure(World,*B);
    }
    World->Save();FarmPanel(Note);
}
void AETHUD::DrawMedkitIcon(float X,float Y,float Size){
    static auto Icon=TWeakObjectPtr<UTexture2D>();
    if(!Icon.IsValid())Icon=LoadObject<UTexture2D>(nullptr,TEXT("/Game/EARTHtotaled/Art/T_FieldKit.T_FieldKit"));
    if(Icon.IsValid())DrawTexture(Icon.Get(),X,Y,Size,Size,0,0,1,1,FLinearColor::White);
}
void AETHUD::FieldButtons(AETCharacter& P,float X,float Y,float Width){
    const auto& S=*P.World->State;int N=0;const float W=FMath::Min(310.f,(Width-24)/3);
    auto Add=[&](const TCHAR* Id,const FString& Title){const int Slot=N++;Button(Id,Title,X+(Slot%3)*(W+10),Y+(Slot/3)*42,W);};
    if(P.Panel==TEXT("music")){
        Add(TEXT("field-mute"),S.MusicMuted?TEXT("Enable music"):TEXT("Mute music"));
        Add(TEXT("field-volume-down"),TEXT("Volume -10%"));Add(TEXT("field-volume-up"),TEXT("Volume +10%"));
        if(P.World->Music.IsValidIndex(5)&&P.World->Music[3]->Sound)Add(TEXT("field-source"),S.JazzSource?TEXT("Use world score"):TEXT("Tune to Doodle Jazz"));
        if(S.JazzSource){Add(TEXT("field-prev"),TEXT("Previous track"));Add(TEXT("field-next"),TEXT("Next track"));}
        Add(TEXT("field-back"),TEXT("Back to pause menu"));return;
    }
    if(P.Panel==TEXT("threatruns")){
        if(S.Run.Type){Add(TEXT("field-mark"),TEXT("Track run on map"));Add(TEXT("field-abandon"),TEXT("Abandon run"));}
        else for(int I=1;I<=3;++I)Add(*FString::Printf(TEXT("field-run%d"),I),ETField::RunName(I));
        return;
    }
    const auto* B=S.Builds.Find(P.DeviceId);if(P.Panel!=TEXT("farm")||!B)return;
    const auto& F=B->Farm;
    if(B->Piece==Build::Cistern)Add(TEXT("field-bottles"),TEXT("Collect sealed water"));
    if(B->Piece==Build::Pump||B->Piece==Build::Cistern)Add(TEXT("field-furrow"),TEXT("Shape irrigation furrows"));
    if(B->Piece==Build::Clinic){
        Add(TEXT("field-order"),TEXT("Queue medpack"));if(S.Wake.Body!=ETWake::Bug)Add(TEXT("field-collect"),TEXT("Collect medpacks"));
        Add(TEXT("field-treat"),TEXT("Apply one medpack"));DrawMedkitIcon(X+Width-70,Y-84,65);
    }
    if(B->Piece==Build::Bed){
        if(!S.Consumed.Contains(TEXT("field.seedkit")))Add(TEXT("field-seeds"),TEXT("Collect starter seeds"));
        if(!F.Crop)for(int I=1;I<=3;++I)Add(*FString::Printf(TEXT("field-plant%d"),I),FString(TEXT("Plant "))+ETField::Crop(I));
        if(ETField::Stage(F)==4)Add(TEXT("field-harvest"),TEXT("Harvest crop"));
        Add(TEXT("field-water"),TEXT("Water / 1 bottle"));
        if(S.FarmStores.FindRef(TEXT("silt"))>0)Add(TEXT("field-silt"),TEXT("Work in mineral silt"));
        if(S.Wake.Body==ETWake::Bug&&S.FarmStores.FindRef(TEXT("rations"))>0)Add(TEXT("field-feed"),TEXT("Eat a garden ration"));
    }
}
