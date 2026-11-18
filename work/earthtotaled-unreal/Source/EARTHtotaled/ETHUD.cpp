#include "ETGame.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"

namespace {const FLinearColor Bone(.89f,.88f,.78f), Mint(.61f,.82f,.63f), Muted(.52f,.57f,.52f), Amber(.93f,.65f,.34f);}
void AETHUD::Text(const FString& V,float X,float Y,FLinearColor C,float Scale) {DrawText(V,C,X,Y,GEngine->GetSmallFont(),Scale,false);}
void AETHUD::Wrapped(const FString& Value,float X,float Y,float Width,float Scale)
{
    TArray<FString> Paragraphs; Value.ParseIntoArray(Paragraphs,TEXT("\n"),false);
    for(auto Paragraph:Paragraphs)
    {
        TArray<FString> Words; Paragraph.ParseIntoArrayWS(Words); FString Line;
        for(auto Word:Words)
        {
            FString Candidate=Line.IsEmpty()?Word:Line+TEXT(" ")+Word; float TW,TH; GetTextSize(Candidate,TW,TH,GEngine->GetSmallFont(),Scale);
            if(TW>Width&&!Line.IsEmpty()) {Text(Line,X,Y,Bone,Scale); Y+=21*Scale; Line=Word;} else Line=Candidate;
        }
        Text(Line,X,Y,Bone,Scale); Y+=25*Scale;
    }
}
void AETHUD::Button(const FString& Id,const FString& Label,float X,float Y,float W)
{
    DrawRect(FLinearColor(.14f,.20f,.16f,.98f),X,Y,W,34); Text(Label,X+10,Y+8,Bone,1.05f);
    AddHitBox(FVector2D(X,Y),FVector2D(W,34),FName(*Id),true);
}
void AETHUD::DialogueSubtitle()
{
    auto P=Cast<AETCharacter>(GetOwningPawn()); if(!P||!P->World||!P->World->HasSubtitle()) return;
    auto W=P->World; const float SW=Canvas->SizeX,SH=Canvas->SizeY;
    const float X=SW*.13f,Y=SH*.765f,Width=SW*.74f;
    DrawRect(FLinearColor(.008f,.012f,.01f,.96f),X-14,Y-8,Width+28,FMath::Min(112.f,SH*.105f));
    Text(W->SubtitleSpeaker,X,Y,Mint,SH<800?.86f:1.05f);
    Wrapped(W->SubtitleText,X,Y+(SH<800?18:23),Width,SH<800?.88f:1.1f);
}
void AETHUD::DrawHUD()
{
    Super::DrawHUD(); auto P=Cast<AETCharacter>(GetOwningPawn()); if(!P||!P->World) return;
    auto W=P->World; auto S=W->State; const float SW=Canvas->SizeX,SH=Canvas->SizeY;
    // Dark backing keeps survival information readable against bright sky and at night.
    DrawRect(FLinearColor(.012f,.019f,.015f,.68f),20,17,520,116);
    DrawRect(FLinearColor(.012f,.019f,.015f,.68f),SW-425,20,410,106);
    DrawRect(FLinearColor(.012f,.019f,.015f,.72f),20,SH-104,560,92);
    DrawRect(FLinearColor(.012f,.019f,.015f,.72f),SW-315,SH-103,300,90);
    Text(TEXT("EARTHtotaled"),32,24,Bone,1.8f); Text(TEXT("THE LAST SIGNAL"),34,52,Muted,.9f);
    Text(W->ChapterNames.IsValidIndex(S->Cores)?W->ChapterNames[S->Cores]:TEXT("KEEP GOING"),32,83,Mint,1.1f);
    Text(W->Objectives.IsValidIndex(S->Cores)?W->Objectives[S->Cores]:TEXT("Explore the wilderness"),32,105,Bone,1.1f);
    const float Hour=W->Hour(); const int32 Minutes=FMath::FloorToInt(Hour*60);
    Text(FString::Printf(TEXT("DAY %d / %02d:%02d / %s"),1+FMath::FloorToInt((16.5f+S->Elapsed/120.f)/24),Minutes/60,Minutes%60,Hour<6||Hour>=19?TEXT("NIGHT"):TEXT("DAYLIGHT")),SW-410,30,Mint,1.1f);
    Text(W->PlaceName(P->GetActorLocation()),SW-410,57,Muted,.85f);
    Text(W->WeatherName()+(W->Sheltered?TEXT(" / UNDER COVER"):TEXT("")),SW-410,79,Mint,.9f);
    Text(TEXT("TAB / PACK   F / LIGHT   K / SKILLS   J / JOURNAL"),SW-410,103,Muted,.8f);
    const auto Location=P->GetActorLocation(); const auto City=W->CityCenter(Location.X,Location.Y);
    Text(FString::Printf(TEXT("CITY %dm   /   SHELTER %dm"),FMath::RoundToInt(FVector::Dist2D(Location,City)/100),FMath::RoundToInt(Location.Size2D()/100)),SW*.42f,28,Amber,.9f);
    if(S->StimTime>0) Text(FString::Printf(TEXT("ENDURANCE / %ds"),FMath::CeilToInt(S->StimTime)),SW*.44f,48,Mint,.9f);
    if(W->Review) {DrawRect(FLinearColor(.02f,.03f,.02f,.9f),SW*.33f,66,SW*.34f,26); Text(TEXT("TEST SESSION / PROGRESS IS NOT SAVED"),SW*.35f,72,Amber,.9f);}
    Text(FString::Printf(TEXT("HEALTH %d  /  SUIT %d"),FMath::CeilToInt(S->Health),FMath::CeilToInt(S->Armor)),32,SH-94,Bone,1.15f);
    Text(FString::Printf(TEXT("WATER %d%%    %d bottles [H]"),FMath::CeilToInt(S->Hydration),S->Water),32,SH-65,S->Hydration<25?Amber:Mint,1.2f);
    Text(FString::Printf(TEXT("WRAPPERS %d / SCRAP %d / MEDS %d [Q] / LEVEL %d"),S->Gold,S->Scrap,S->Meds,S->Level),32,SH-36,Muted,1);
    if(W->Weapons.IsValidIndex(S->Weapon))
    {
        Text(W->Weapons[S->Weapon].Name,SW-300,SH-90,Bone,1.1f);
        Text(P->ReloadTime>0?TEXT("RELOADING"):FString::Printf(TEXT("%02d  /  %d"),S->Ammo[S->Weapon],S->Reserve[S->Weapon]),SW-300,SH-62,Mint,1.8f);
    }
    if(P->Panel.IsEmpty())
    {
        DrawLine(SW/2-6,SH/2,SW/2+6,SH/2,Mint,1); DrawLine(SW/2,SH/2-6,SW/2,SH/2+6,Mint,1);
        if(P->HitTime>0) {DrawLine(SW/2-10,SH/2-10,SW/2-4,SH/2-4,Amber,2); DrawLine(SW/2+10,SH/2+10,SW/2+4,SH/2+4,Amber,2); DrawLine(SW/2+10,SH/2-10,SW/2+4,SH/2-4,Amber,2); DrawLine(SW/2-10,SH/2+10,SW/2-4,SH/2+4,Amber,2);}
        if(P->Focus) Text(TEXT("[E] ")+P->Focus->Label,SW/2-180,SH/2+45,Bone,1.15f);
        if(W->NoticeTime>0) Text(W->Notice,SW*.2f,SH-145,Mint,1.1f); DialogueSubtitle(); return;
    }
    DrawRect(FLinearColor(.015f,.025f,.02f,.97f),SW*.035f,SH*.12f,SW*.93f,SH*.73f);
    const float X=SW*.07f,Y=SH*.16f, Width=SW*.86f;
    Text(P->PanelTitle,X,Y,Mint,1.6f);
    if(P->Panel==TEXT("skills"))
    {
        Text(FString::Printf(TEXT("%d POINTS / LEVEL %d / %d XP"),S->Points,S->Level,S->XP),X,Y+32,Amber,1.1f);
        const float Scale=FMath::Min(Width/700.f,(SH*.49f)/390.f); const FVector2D Offset(X+30,Y+65);
        for(auto Node:W->Perks) for(auto N:Node.Neighbors) for(auto Other:W->Perks) if(Other.Id==N)
        {
            FVector2D A=Offset+Node.Position*Scale, B=Offset+Other.Position*Scale;
            DrawLine(A.X,A.Y,B.X,B.Y,P->Has(Node.Id)&&P->Has(Other.Id)?Mint:Muted.CopyWithNewOpacity(.45f),2);
        }
        for(auto Node:W->Perks)
        {
            auto Pos=Offset+Node.Position*Scale; bool Owned=P->Has(Node.Id),Connected=false; for(auto N:Node.Neighbors) Connected|=P->Has(N);
            DrawRect(Owned?Mint:Connected?FLinearColor(.36f,.29f,.17f):FLinearColor(.12f,.15f,.13f),Pos.X-10,Pos.Y-10,20,20);
            Text(Node.Name,Pos.X-35,Pos.Y+15,Owned?Mint:Bone,.8f);
            Text(Owned?TEXT("LEARNED"):FString::Printf(TEXT("%d points"),Node.Cost),Pos.X-25,Pos.Y+29,Muted,.7f);
            AddHitBox(FVector2D(Pos.X-30,Pos.Y-12),FVector2D(72,47),FName(*(TEXT("perk:")+Node.Id)),true);
        }
        Wrapped(P->PanelBody,X,SH*.75f,Width,1);
    }
    else if(P->Panel==TEXT("inventory"))
    {
        Text(P->PanelBody,X,Y+38,Muted,SH<800?.8f:1.f);
        Text(TEXT("SUPPLIES / select to use"),X,Y+74,Mint,1.1f);
        const float Row=FMath::Min(41.f,SH*.049f), Right=X+Width*.52f;
        int32 N=0;
        for(const auto& Supply:W->Supplies) {
            float RY=Y+105+N++*Row;
            FString Label=FString::Printf(TEXT("%2d  %s"),S->Inventory.FindRef(Supply.Id),*Supply.Name);
            if(Supply.Usable) Button(TEXT("use:")+Supply.Id,Label,X,RY,Width*.44f);
            else Text(Label,X+10,RY+8,Bone,1.05f);
        }
        Text(TEXT("ARSENAL / select to equip"),Right,Y+74,Mint,1.1f);
        for(int32 I=0;I<W->Weapons.Num();++I) {
            float RY=Y+105+I*Row; bool Owned=S->Unlocked.Contains(I);
            FString Label=FString::Printf(TEXT("%s%s"),S->Weapon==I?TEXT("> "):TEXT(""),*W->Weapons[I].Name);
            if(Owned) Button(FString::Printf(TEXT("equip:%d"),I),Label,Right,RY,Width*.44f);
            else Text(Label+TEXT(" / undiscovered"),Right+10,RY+8,Muted,.9f);
        }
    }
    else if(P->Panel==TEXT("bench"))
    {
        Wrapped(P->PanelBody,X,Y+44,Width,1);
        float RY=Y+115;
        for(const auto& R:W->Recipes) {
            FString Cost; if(R.Scrap) Cost=FString::Printf(TEXT("%d/%d scrap   "),S->Scrap,R.Scrap);
            for(auto It:R.Ingredients) {
                FString Name=It.Key; for(auto Supply:W->Supplies) if(Supply.Id==It.Key) Name=Supply.Name;
                Cost+=FString::Printf(TEXT("%d/%d %s   "),S->Inventory.FindRef(It.Key),It.Value,*Name);
            }
            Button(TEXT("craft:")+R.Id,R.Name,X,RY,Width*.29f);
            Text(Cost,X+Width*.31f,RY+10,Muted,SH<800?.8f:1.f); RY+=45;
        }
        Button(TEXT("scrap-repair"),TEXT("Quick suit repair / 10 scrap"),X,RY+6,Width*.40f);
    }
    else
    {
        Wrapped(P->PanelBody,X,Y+48,Width,SH<800?.9f:1.1f);
        float BY=SH*.65f;
        if(P->Panel==TEXT("contact"))
        {
            Button(TEXT("quest"),TEXT("Accept / turn in quest"),X,BY,250);
            if(P->Contact==TEXT("SALT-7"))
            {
                Button(TEXT("ammo"),TEXT("Ammo / 20 wrappers"),X+270,BY,210); Button(TEXT("repair"),TEXT("Suit / 25 wrappers"),X+500,BY,210);
                Button(TEXT("lmg"),TEXT("Bastion-70 / 90 wrappers"),X,BY+44,250);
            }
            else {Button(TEXT("ration"),TEXT("Emergency water ration"),X+270,BY,250); Button(TEXT("supply-run"),TEXT("Supply run / 12 scrap"),X+540,BY,250); if(P->Contact==TEXT("Nora")&&!S->Welcome) Button(TEXT("welcome"),TEXT("Welcome / 2 water"),X,BY+44,250);}
        }
        if(P->Panel==TEXT("pause")) {
            Button(TEXT("view"),FString::Printf(TEXT("View distance: %dm / change"),S->ViewDistance),X,BY,360);
            Button(TEXT("render"),FString::Printf(TEXT("Render detail: %d%% / change"),S->RenderScale),X+380,BY,360);
            Text(FString::Printf(TEXT("%d x %d internal picture / higher detail costs performance"),FMath::RoundToInt(SW*S->RenderScale/100),FMath::RoundToInt(SH*S->RenderScale/100)),X,BY+47,Muted,.95f);
            Button(TEXT("music-volume"),FString::Printf(TEXT("Music: %d%% / change"),S->MusicVolume),X,BY+80,360);
            Button(TEXT("weather-volume"),FString::Printf(TEXT("Weather sound: %d%% / change"),S->WeatherVolume),X+380,BY+80,360);
            const TCHAR* Tracks[]={TEXT("Embers in the Canopy"),TEXT("Concrete Echoes"),TEXT("The Last Frequency")};
            Text(FString(TEXT("SCORE / "))+Tracks[W->MusicFor(P->GetActorLocation())],X,BY+128,Muted,1);
        }
        if(P->Panel==TEXT("ending")) {Button(TEXT("garden"),TEXT("Wake irrigation"),X,BY,260); Button(TEXT("signal"),TEXT("Transmit warning"),X+280,BY,260);}
        if(P->Panel==TEXT("death")) Button(TEXT("reload-save"),TEXT("Continue last save"),X,BY,280);
    }
    if(P->Panel!=TEXT("death")) Button(TEXT("resume"),TEXT("RETURN TO THE WORLD"),X,SH*.88f,310);
    if(P->Panel==TEXT("pause")) Button(TEXT("quit"),TEXT("Save and exit"),X+330,SH*.88f,210);
    if(P->Panel!=TEXT("pause")) DialogueSubtitle();
}
void AETHUD::NotifyHitBoxClick(FName Name) {Super::NotifyHitBoxClick(Name); if(auto P=Cast<AETCharacter>(GetOwningPawn())) P->Action(Name.ToString());}
