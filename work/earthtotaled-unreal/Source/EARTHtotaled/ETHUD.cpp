#include "ETWakeVehicle.h"
#include "ETWake.h"
#include "ETGame.h"
#include "ETRingLine.h"
#include "ETLandfall.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"

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
    if(P->Panel==TEXT("wake")){DrawWake(P);return;}
    if(P->Panel==TEXT("console")) return;
    if(P->Panel==TEXT("arrival")||P->Panel==TEXT("updates")||P->Panel==TEXT("relics")) {DrawLandfall(P);return;}
    if(P->Panel==TEXT("mods")||P->Panel==TEXT("devtools")||P->Panel==TEXT("town")) {DrawReclamation(P);return;}
    if(P->Panel==TEXT("map")) {DrawAtlas(P); return;}
    if(P->Panel==TEXT("controls")) {DrawControls(P); return;}
    if(P->Panel==TEXT("slots")) {
        const float X=SW*.09f,Y=SH*.075f,Width=SW*.82f,Gap=12,Col=(Width-Gap)/2;
        DrawRect(FLinearColor(.018,.031,.029,.97),X-20,Y-15,Width+40,SH*.86f);
        Text(TEXT("SURVIVOR SAVES"),X,Y,Bone,1.8f);
        Text(FString::Printf(TEXT("Active %02d    /    Selected %02d"),W->ActiveSlot,P->SelectedSlot),X,Y+39,Mint,1.f);
        for(int32 I=0;I<P->SlotLabels.Num();++I) {
            const float BX=X+(I/5)*(Col+Gap),BY=Y+75+(I%5)*40;
            if(I+1==P->SelectedSlot) DrawRect(Mint,BX-5,BY,4,34);
            Button(FString::Printf(TEXT("pick-%d"),I+1),FString::Printf(TEXT("%02d    %s"),I+1,*P->SlotLabels[I]),BX,BY,Col);
        }
        const float AY=Y+282;
        Wrapped(P->SlotMessage,X,AY,Width,.94f);
        if(P->PendingSlotAction.IsEmpty()) {
            const float Third=(Width-2*Gap)/3,RY=AY+65;
            Button(TEXT("slot-play"),TEXT("Play selected"),X,RY,Third);
            Button(TEXT("slot-new"),TEXT("New survivor here"),X+Third+Gap,RY,Third);
            if(P->SlotOrigin==TEXT("pause")) Button(TEXT("slot-copy"),TEXT("Copy this run here"),X+2*(Third+Gap),RY,Third);
            Button(TEXT("slot-import"),TEXT("Import file"),X,RY+40,Third);
            Button(TEXT("slot-export"),TEXT("Export file"),X+Third+Gap,RY+40,Third);
            Button(TEXT("slot-back"),TEXT("Back"),X+2*(Third+Gap),RY+40,Third);
        } else {
            Button(TEXT("slot-confirm"),TEXT("Back up and replace"),X,AY+65,Col);
            Button(TEXT("slot-cancel"),TEXT("Cancel"),X+Col+Gap,AY+65,Col);
        }
        return;
    }
    if(P->Panel==TEXT("title")||P->Panel==TEXT("confirm-new")) {
        const float X=SW*.09f,Y=SH*.25f,Width=FMath::Min(SW*.76f,620.f);
        DrawRect(FLinearColor(.016,.024,.021,.32),0,0,SW,SH);
        DrawRect(FLinearColor(.021,.036,.029,.93),X-28,Y-30,Width+56,SH*.54f+46);
        DrawRect(Mint,X-28,Y-30,4,SH*.54f+46);
        const bool Title=P->Panel==TEXT("title");
        Text(Title?TEXT("EARTHtotaled"):P->PanelTitle,X,Y,Bone,Title?FMath::Clamp(SW/420.f,2.f,4.8f):1.8f);
        if(Title) Text(FString::Printf(TEXT("1.16.0     SURVIVOR %02d"),W->ActiveSlot),X,Y+SH*.10f,Mint,1.15f);
        if(!Title||P->PanelBody!=TEXT("FIELDWORKS")) Wrapped(P->PanelBody,X,Y+SH*.15f,Width,1.0f);
        float Row=Y+SH*.24f;
        if(Title) {
            if(W->HadSave) {Button(TEXT("continue"),TEXT("Continue survivor"),X,Row,Width*.72f); Row+=44;}
            Button(TEXT("new"),W->HadSave?TEXT("New survivor"):TEXT("Start at Shelter 09"),X,Row,Width*.72f);
            Button(TEXT("slots"),TEXT("Survivor saves  /  import and export"),X,Row+42,Width*.72f);
            Button(TEXT("controls"),TEXT("Field guide & controls   /   F2"),X,Row+84,Width*.72f);
            Button(TEXT("mods"),TEXT("Mod workshop"),X,Row+126,Width*.35f);
            Button(TEXT("updates"),TEXT("Game updates"),X+Width*.37f,Row+126,Width*.35f);
            Button(TEXT("quit"),TEXT("Exit to desktop"),X,Row+168,Width*.72f);
        } else {
            Button(TEXT("cancel"),TEXT("Keep my survivor"),X,Row,Width*.72f);
            Button(TEXT("confirm"),TEXT("Back up and start again"),X,Row+46,Width*.72f);
        }
        return;
    }
    if(P->Panel.IsEmpty()&&S->ArrivalStage==1) {
        int Side=ETLandfall::NextBank(S->TrenchSides);
        if(Side>=0) {const auto XY=ETLandfall::Stake(Side);const FVector Goal(XY,W->SurfaceHeight(XY.X,XY.Y)+155);
            const auto Pixel=Project(Goal,true);
            if(Pixel.Z>0&&Pixel.X>75&&Pixel.X<SW-80&&Pixel.Y>125&&Pixel.Y<SH-130)
                Text(FString::Printf(TEXT("◇ %s bank   %.0f m"),ETLandfall::Bank(Side),FVector::Dist(Goal,P->GetActorLocation())/100),Pixel.X-50,Pixel.Y,Amber,1.f);
        }
    }
    // Dark backing keeps survival information readable against bright sky and at night.
    DrawRect(FLinearColor(.012f,.019f,.015f,.68f),20,17,520,116);
    DrawRect(FLinearColor(.012f,.019f,.015f,.68f),SW-425,20,410,106);
    DrawRect(FLinearColor(.012f,.019f,.015f,.72f),20,SH-104,560,92);
    DrawRect(FLinearColor(.012f,.019f,.015f,.72f),SW-315,SH-103,300,90);
    Text(TEXT("EARTHtotaled"),32,24,Bone,1.8f); Text(TEXT("THE LAST SIGNAL"),34,52,Muted,.9f);
    const bool Campaign=S->Wake.Chapter!=ETWake::Missing||S->ArrivalStage!=1;
    Text(Campaign?(S->Wake.Chapter==ETWake::Home?TEXT("MEGAWAKE  /  KIT IS HOME"):TEXT("MEGAWAKE  /  FIND KIT")):W->StoryTitle(),32,83,Mint,1.1f);
    Text(Campaign?ETWake::Objective(S->Wake):W->StoryObjective(),32,105,Bone,1.01f);
    const float Hour=W->Hour(); const int32 Minutes=FMath::FloorToInt(Hour*60);
    Text(FString::Printf(TEXT("DAY %d   %02d:%02d   /   %s"),1+FMath::FloorToInt((S->ClockOffset+S->Elapsed)/ETDay::Period),Minutes/60,Minutes%60,W->IsNight()?TEXT("DAWN AT 06:00"):TEXT("NIGHTFALL AT 19:00")),SW-410,30,W->IsNight()?Amber:Mint,1.0f);
    Text(W->PlaceName(P->GetActorLocation()),SW-410,57,Muted,.85f);
    Text(W->WeatherName()+(W->Sheltered?TEXT(" / UNDER COVER"):TEXT("")),SW-410,79,Mint,.9f);
    Text(TEXT("M  MAP     F2  GUIDE     B  BUILD     G  BORER     ESC  MENU"),SW-410,103,Bone,.85f);
    if(P->SweepHeld) Text(P->SweepArmed?FString::Printf(TEXT("SALVAGE SWEEP  /  %.1f m"),2.5f+6.7f*P->SweepHold):TEXT("Crouch within 2.5 m of loose loot to start a sweep"),32,SH-159,Mint,.92f);
    if(S->StimTime>0) Text(FString::Printf(TEXT("ENDURANCE / %ds"),FMath::CeilToInt(S->StimTime)),32,SH-189,Mint,.85f);
    if(W->Review) Text(TEXT("TEST SESSION / PROGRESS IS NOT SAVED"),32,176,Amber,.85f);
    FString Vitals=FString::Printf(TEXT("HEALTH %d  /  SHIELD %d  /  SUIT %d"),FMath::CeilToInt(S->Health),FMath::CeilToInt(S->Shield),FMath::CeilToInt(S->Armor));
    if(S->Health<=25&&S->Health>0) Vitals+=S->Meds>0?TEXT("   CRITICAL / Q MEDKIT"):TEXT("   CRITICAL / TAKE COVER");
    Text(Vitals,32,SH-94,S->Health<=25?Amber:Bone,1.15f);
    if(S->Wake.Body)Text(S->Wake.Body==ETWake::Bug?TEXT("MOLT   /   WATER INDEPENDENT"):TEXT("OVERRIDE   /   CLOSED-CYCLE FRAME"),32,SH-65,Mint,1.12f);
    else Text(FString::Printf(TEXT("WATER %d%%    %d bottles [H]"),FMath::CeilToInt(S->Hydration),S->Water),32,SH-65,S->Hydration<25?Amber:Mint,1.2f);
    Text(S->Wake.Body==ETWake::Bug?TEXT("Survivor pack sealed   /   Tab to inspect"):FString::Printf(TEXT("WRAPPERS %d / SCRAP %d / MEDS %d [Q]"),S->Gold,S->Scrap,S->Meds),32,SH-36,Muted,1);
    DrawRect(FLinearColor(.012,.019,.015,.8),20,SH-135,204,29);
    Text(FString::Printf(TEXT("JET %d%%"),FMath::RoundToInt(S->JetFuel)),30,SH-130,P->Jetting?Amber:Mint,.85f);
    DrawRect(Muted.CopyWithNewOpacity(.4f),104,SH-121,105,4);
    DrawRect(P->Jetting?Amber:Mint,104,SH-121,1.05f*S->JetFuel,4);
    DrawRect(FLinearColor(.012,.019,.015,.83),230,SH-135,350,29);
    Text(FString::Printf(TEXT("LV %d    %d / %d XP     K  Skills (%d)"),S->Level,S->XP,S->NextLevelXP(),S->Points),240,SH-133,Mint,.9f);
    DrawRect(Muted.CopyWithNewOpacity(.55f),240,SH-113,327,3);
    DrawRect(Amber,240,SH-113,327.f*FMath::Clamp(float(S->XP)/S->NextLevelXP(),0.f,1.f),3);
    if(P->LevelNoticeTime>0||P->XPNoticeTime>0) {
        const FString Earned=P->LevelNoticeTime>0?FString::Printf(TEXT("LEVEL %d    /    K  Spend skill points"),S->Level):FString::Printf(TEXT("+%d survivor XP"),P->LastXPGain);
        Text(Earned,240,SH-161,Amber,1.05f);
    }
    if(P->SuitFlinch>0&&P->Panel.IsEmpty()) {
        const auto Edge=FLinearColor(.85f,.20f,.10f,.6f*P->SuitFlinch/.24f);
        DrawRect(Edge,0,0,SW,5); DrawRect(Edge,0,SH-5,SW,5);
        DrawRect(Edge,0,0,5,SH); DrawRect(Edge,SW-5,0,5,SH);
    }
    if(P->GetCharacterMovement()->IsFalling()&&!P->Climbing)
        Text(!P->AirJumpSpent?TEXT("SPACE  /  FREE AIR JUMP"):S->JetFuel<=0?TEXT("LAND TO REFUEL"):TEXT("HOLD SPACE  /  THRUST"),30,SH-155,Mint,.85f);
    if(P->Building) {
        const bool Frame=P->BuildPiece<AETConstruction::Core;
        const auto Tier=P->BuildMaterial;
        const FString Name=Frame?FString(AETConstruction::MaterialName(Tier))+TEXT(" ")+AETConstruction::Name(P->BuildPiece):FString(AETConstruction::Name(P->BuildPiece));
        Text(Name.ToUpper(),SW-300,SH-96,Amber,1.08f);
        FString Cost=AETConstruction::Recipe(P->BuildPiece,Tier);
        if(Frame) Cost+=FString::Printf(TEXT("  /  carrying %d"),S->Inventory.FindRef(AETConstruction::Resource(Tier)));
        if(Frame&&Tier==0&&S->BuildStock) Cost+=FString::Printf(TEXT(" + %d kits"),S->BuildStock);
        if(S->Wake.Body)Cost=TEXT("No material cost");
        Text(Cost,SW-300,SH-73,Bone,.92f);
        Text(Frame?TEXT("Wheel  piece     C  material     R  rotate"):TEXT("Wheel  machine     R  rotate"),SW-300,SH-52,Mint,.84f);
        Text(TEXT("LMB place   RMB salvage   T upgrade   B exit"),SW-300,SH-32,Mint,.82f);
        Text(P->BuildHint,SW*.5f-105,SH*.5f+27,Bone,.95f);
    } else if(P->Borer)
    {
        Text(S->SquareBorer?TEXT("BORER / SQUARE"):TEXT("BORER / ROUND"),SW-300,SH-90,Amber,1.2f);
        Text(TEXT("LMB  Blast     RMB  Survey"),SW-300,SH-63,Bone,1.05f);
        Text(TEXT("R shape   V winch   G stow"),SW-300,SH-38,Mint,.95f);
        if(P->Aiming&&P->Panel.IsEmpty()) {
            DrawRect(FLinearColor(.025f,.025f,.018f,.9f),SW*.5f-149,SH*.5f+18,298,28);
            Text(P->SurveyReading,SW*.5f-140,SH*.5f+24,Amber,.95f);
        }
    }
    else if(P->Ride()) {Text(S->Wake.Seat==ETWake::Car?TEXT("ROADSTER"):TEXT("SURVEY PLANE"),SW-300,SH-91,Amber,1.15f);
        Text(FString::Printf(TEXT("%.0f km/h   /   %s"),P->Ride()->State().Speed*.036f,P->Ride()->State().Started?TEXT("engine on"):TEXT("engine off")),SW-300,SH-64,Bone,1.12f);}
    else if(S->Wake.Body==ETWake::Bug){Text(TEXT("MOLT / LIVING CLAWS"),SW-300,SH-90,Amber,1.12f);Text(TEXT("LMB strike    G burrow    B grow"),SW-300,SH-60,Bone,1.f);}
    else if(W->Weapons.IsValidIndex(S->Weapon))
    {
        const int32 Rank=P->CurrentRank();
        Text(FString::Printf(TEXT("%s  /  Mk %d"),*P->CurrentGun().Name,Rank),SW-300,SH-94,Bone,1.05f);
        Text(P->ReloadTime>0?TEXT("RELOADING"):FString::Printf(TEXT("%02d  /  UNLIMITED"),P->CurrentMagazine()),SW-300,SH-68,Mint,1.23f);
        const float Next=P->CurrentThreshold(Rank);
        Text(Next>0?FString::Printf(TEXT("%.0f / %.0f damage to next upgrade"),P->CurrentGunDamage(),Next):TEXT("Maximum weapon rank"),SW-300,SH-40,Muted,.85f);
    }
    if(P->Panel.IsEmpty())
    {
        DrawNavigation(P);DrawWakeTrail(P);
        DrawLine(SW/2-6,SH/2,SW/2+6,SH/2,Mint,1); DrawLine(SW/2,SH/2-6,SW/2,SH/2+6,Mint,1);
        if(P->HitTime>0) {DrawLine(SW/2-10,SH/2-10,SW/2-4,SH/2-4,Amber,2); DrawLine(SW/2+10,SH/2+10,SW/2+4,SH/2+4,Amber,2); DrawLine(SW/2+10,SH/2-10,SW/2+4,SH/2-4,Amber,2); DrawLine(SW/2-10,SH/2+10,SW/2-4,SH/2+4,Amber,2);}
        if(auto* Site=P->FocusBuild.Get();Site&&Site->Design.Piece>=AETConstruction::Core) Text(FString(TEXT("[E] "))+Site->Name(Site->Design.Piece),SW*.5f-100,SH*.5f+48,Mint,1.12f);
        if(S->RaidStage) Text(W->IsNight()?FString::Printf(TEXT("RELAY  /  %s   %ds"),S->RaidStage==1?TEXT("Bug raiders forming"):TEXT("Under attack"),FMath::CeilToInt(S->RaidTime)):TEXT("RELAY  /  Raid resumes at nightfall"),32,151,Amber,1.15f);
        else for(const auto& It:S->Threats) if(It.Value.Kind==TEXT("hive")) {
            const FVector To=It.Value.Position-P->GetActorLocation();
            const float Angle=FMath::FindDeltaAngleDegrees(float(P->GetControlRotation().Yaw),float(To.Rotation().Yaw));
            Text(FString::Printf(TEXT("NEST  /  %.0f m   %s"),To.Size2D()/100,FMath::Abs(Angle)<10?TEXT("ahead"):Angle>0?TEXT("turn right"):TEXT("turn left")),32,150,Amber,1); break;
        }
        if(IsValid(P->Focus)) Text((S->DropPositions.Contains(P->Focus->Id)?FString(TEXT("[Hold Ctrl] Sweep  /  ")):FString(TEXT("[E] ")))+P->Focus->Label,SW/2-180,SH/2+51,Bone,1.15f);
        else if(P->TreeInReach) Text(TEXT("[E] Fell tree   +8 wood"),SW/2-106,SH/2+51,Mint,1.05f);
        if(auto* A=P->Sighted.Get();A&&!P->Building&&!P->Borer) {
            const FString Name=A->Kind==TEXT("drone")?TEXT("SUPERTECH / SHARD DRONE"):A->Kind==TEXT("robot")?TEXT("SUPERTECH / PATROL"):A->IsBrood()?TEXT("BUG / ")+A->Kind.ToUpper():A->Kind.ToUpper();
            Text(FString::Printf(TEXT("%s   %.0f HP"),*Name,A->Health),SW*.5f-92,SH*.5f+25,Amber,.85f);
        }
        if(P->EnemyReacquireTime>0&&!W->AtRefuge&&W->IsNight())
            Text(FString::Printf(TEXT("Enemies can attack again in %ds"),FMath::CeilToInt(P->EnemyReacquireTime)),32,SH-163,Amber,1.05f);
        for(auto* RailLine:W->RailLines) {
            const FString Rail=RailLine->Prompt(*P);
            if(!Rail.IsEmpty()) {
                float Width,Height; GetTextSize(Rail,Width,Height,GEngine->GetSmallFont(),1.12f);
                DrawRect(FLinearColor(.023,.048,.045,.93),(SW-Width)*.5f-10,SH*.67f-4,Width+20,Height+9);
                Text(Rail,(SW-Width)*.5f,SH*.67f,Mint,1.12f);
                break;
            }
        }
        if(W->NoticeTime>0) Text(W->Notice,SW*.2f,SH-145,Mint,1.1f); DialogueSubtitle(); return;
    }
    if(P->Panel==TEXT("skills")) {DrawSkillWeb(P);return;}
    DrawRect(FLinearColor(.015f,.025f,.02f,.97f),SW*.035f,SH*.12f,SW*.93f,SH*.73f);
    const float X=SW*.07f,Y=SH*.16f, Width=SW*.86f;
    Text(P->PanelTitle,X,Y,Mint,1.6f);
    if(P->Panel==TEXT("inventory"))
    {
        Text(P->PanelBody,X,Y+38,Muted,SH<800?.8f:1.f);
        Text(FString::Printf(TEXT("SUPPLIES   /   %d wood   %d stone   %d kits"),S->Inventory.FindRef(TEXT("wood")),S->Inventory.FindRef(TEXT("stone")),S->BuildStock),X,Y+74,Mint,1.0f);
        const float Row=FMath::Min(38.f,SH*.045f), Right=X+Width*.52f;
        int32 N=0;
        for(const auto& Supply:W->Supplies) {
            float RY=Y+105+N++*Row;
            FString Label=FString::Printf(TEXT("%2d  %s"),S->Inventory.FindRef(Supply.Id),*Supply.Name);
            if(Supply.Usable) Button(TEXT("use:")+Supply.Id,Label,X,RY,Width*.44f);
            else Text(Label,X+10,RY+8,Bone,1.05f);
        }
        Button(TEXT("relics"),FString::Printf(TEXT("Unique gun rack   /   %d found"),S->Relics.Num()),Right,Y+65,Width*.44f);
        for(int32 I=0;I<W->Weapons.Num();++I) {
            float RY=Y+105+I*Row; bool Owned=S->Unlocked.Contains(I);
            FString Label=FString::Printf(TEXT("%s%s"),S->EquippedRelic==INDEX_NONE&&S->Weapon==I?TEXT("> "):TEXT(""),*W->Weapons[I].Name);
            if(Owned) Button(FString::Printf(TEXT("equip:%d"),I),Label+FString::Printf(TEXT(" / Mk %d"),P->GunRank(I)),Right,RY,Width*.44f);
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
        if(P->Panel==TEXT("journal"))Button(TEXT("wake-journal"),TEXT("Kit / MegaWAKE field journal"),X,BY,325);
        if(P->Panel==TEXT("contact"))
        {
            if(P->Contact==TEXT("SALT-7")||P->Contact==TEXT("Nora"))Button(TEXT("wake-journal"),TEXT("Kit / MegaWAKE"),X+590,BY+44,192);
            Button(TEXT("quest"),TEXT("Accept / turn in quest"),X,BY,250);
            if(P->Contact==TEXT("SALT-7"))
            {
                Button(TEXT("kits"),TEXT("6 kits / 20 wrappers"),X+270,BY,210); Button(TEXT("repair"),TEXT("Suit / 25 wrappers"),X+500,BY,210);
                Button(TEXT("lmg"),TEXT("Bastion-70 / 90 wrappers"),X,BY+44,250);
                Button(TEXT("sell-ore"),TEXT("Sell 1 gold ore / 12 wrappers"),X+270,BY+44,305);
            }
            else {Button(TEXT("ration"),TEXT("Emergency water ration"),X+270,BY,250); Button(TEXT("supply-run"),TEXT("Supply run / 12 scrap"),X+540,BY,250); if(P->Contact==TEXT("Nora")&&!S->Welcome) Button(TEXT("welcome"),TEXT("Welcome / 2 water"),X,BY+44,250);}
        }
        if(P->Panel==TEXT("device")) {
            const auto* B=S->Builds.Find(P->DeviceId); const float Col=FMath::Min(245.f,(Width-28)/3);
            Button(TEXT("mend"),TEXT("Repair / 1 iron"),X,BY,Col);
            if(B&&B->Piece==AETConstruction::Core) {
                Button(TEXT("plate"),TEXT("Upgrade armor"),X+Col+12,BY,Col); Button(TEXT("hunt"),TEXT("Trace Bug nest"),X+2*(Col+12),BY,Col);
            } else if(B&&B->Piece==AETConstruction::Miner) Button(TEXT("hopper"),TEXT("Collect hopper"),X+Col+12,BY,Col);
        }
        if(P->Panel==TEXT("pause")) {
            const float ControlWidth=FMath::Min(300.f,(Width-24)/3);
            BY=Y+SH*.22f;
            Button(TEXT("controls"),TEXT("Field guide & controls   /   F2"),X,Y+54,Width*.46f);
            Button(TEXT("mods"),TEXT("Mod workshop"),X+Width*.49f,Y+54,Width*.46f);
            Button(TEXT("view"),FString::Printf(TEXT("View distance / %d m"),S->ViewDistance),X,BY,ControlWidth);
            Button(TEXT("render"),FString::Printf(TEXT("Render detail / %d%%"),S->RenderScale),X+ControlWidth+12,BY,ControlWidth);
            Button(TEXT("field-display"),S->RetroDisplay?TEXT("Display / PS1"):TEXT("Display / native"),X+2*(ControlWidth+12),BY,ControlWidth);
            Text(FString::Printf(TEXT("%d x %d internal picture / higher detail costs performance"),FMath::RoundToInt(SW*S->RenderScale/100),FMath::RoundToInt(SH*S->RenderScale/100)),X,BY+47,Muted,.95f);
            Button(TEXT("music-volume"),FString::Printf(TEXT("Music volume   %d%%"),S->MusicVolume),X,BY+80,ControlWidth);
            Button(TEXT("weather-volume"),FString::Printf(TEXT("Weather volume   %d%%"),S->WeatherVolume),X+ControlWidth+12,BY+80,ControlWidth);
            Button(TEXT("slots"),TEXT("Survivor saves"),X+2*(ControlWidth+12),BY+80,ControlWidth);
            Button(TEXT("aim-assist"),S->LightAimAssist?TEXT("Aim assist / light"):TEXT("Aim assist / off"),X+2*(ControlWidth+12),BY+122,ControlWidth);
            Button(TEXT("skills"),TEXT("Survivor skill web  /  K"),X+ControlWidth+12,BY+122,ControlWidth);
            Button(TEXT("updates"),TEXT("Game updates"),X,BY+122,ControlWidth);
        }
        if(P->Panel==TEXT("ending")) {Button(TEXT("garden"),TEXT("Wake irrigation"),X,BY,260); Button(TEXT("signal"),TEXT("Transmit warning"),X+280,BY,260);}
        if(P->Panel==TEXT("death")) {
            Button(TEXT("reload-save"),TEXT("Continue last save"),X,BY,245);
            Button(TEXT("main-menu"),TEXT("Main menu"),X+260,BY,210);
            if(W->HadSave) Button(TEXT("shelter-rescue"),TEXT("Recover at Shelter 09"),X,BY+44,245);
        }
    }
    if(P->Panel!=TEXT("death")) Button(TEXT("resume"),TEXT("RETURN TO THE WORLD"),X,SH*.88f,310);
    if(P->Panel==TEXT("pause")) Button(TEXT("main-menu"),TEXT("Save and return to main menu"),X+325,SH*.88f,305);
    if(P->Panel!=TEXT("pause")) DialogueSubtitle();
}
void AETHUD::NotifyHitBoxClick(FName Name) {Super::NotifyHitBoxClick(Name); if(auto P=Cast<AETCharacter>(GetOwningPawn())) P->Action(Name.ToString());}
