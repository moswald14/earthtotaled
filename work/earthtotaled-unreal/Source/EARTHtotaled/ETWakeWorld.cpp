#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "ETWake.h"
#include "ETGame.h"
#include "ETWakeVehicle.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

using namespace ETWake;
namespace {
    FName Numbered(const TCHAR* Prefix,int I){return FName(*FString::Printf(TEXT("%s%d"),Prefix,I));}
    UStaticMesh* Art(const FString& Name){return LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Name+TEXT(".")+Name));}
}
FVector AETWorld::WakeSpot(FName Node,int Index) const {
    const FVector2D V=Location(State->Wake,Node,Index);
    const FVector2D Ground=Node=="juice"||Node=="frame"?Location(State->Wake,"vault"):V;
    return FVector(V,SurfaceHeight(Ground.X,Ground.Y)+5);
}
void AETWorld::SetWakePerks() {
    Perks.RemoveAll([](const FETPerk& P){return P.Branch=="MOLT"||P.Branch=="OVERRIDE";});
    if(!State->Wake.Body)return;
    const bool Organic=State->Wake.Body==Bug;
    const TCHAR* Ids[]={Organic?TEXT("molt"):TEXT("override"),Organic?TEXT("molt-shell"):TEXT("override-fabricate"),Organic?TEXT("molt-sense"):TEXT("override-permit"),Organic?TEXT("molt-burrow"):TEXT("override-span")};
    const TCHAR* Names[]={Organic?TEXT("Molt"):TEXT("Override"),Organic?TEXT("Dense shell"):TEXT("Fabricate"),Organic?TEXT("Signal sense"):TEXT("Authorize"),Organic?TEXT("Burrow"):TEXT("Long reach")};
    const TCHAR* Descs[]={Organic?TEXT("Living claws, a water-free shell and organic architecture."):TEXT("Build without spending material. The frame recycles its own supply."),
        Organic?TEXT("Incoming damage is reduced by one quarter."):TEXT("Use a fabricator to print a field medkit and repair supplies."),
        Organic?TEXT("Reveal every unhealed herd on the field atlas. Your claws reach farther."):TEXT("Authorize construction of a Parallax Spire."),
        Organic?TEXT("Press G on firm wilderness ground to burrow forward under cover."):TEXT("Place authorized structures up to thirty metres away.")};
    State->Skills.AddUnique(Ids[0]);
    for(int I=0;I<4;++I){FETPerk N;N.Id=Ids[I];N.Name=Names[I];N.Branch=Organic?TEXT("MOLT"):TEXT("OVERRIDE");N.Description=Descs[I];
        N.Position={85.+I*157.,374};N.Cost=1;if(I)N.Neighbors.Add(Ids[I==3?2:0]);else {N.Neighbors.Add(Ids[1]);N.Neighbors.Add(Ids[2]);}Perks.Add(N);}
}
void AETWorld::RefreshWake() {
    if(!State||!Geology)return;auto& S=State->Wake;
    auto Node=[&](FName Key,FName At,int Index,const TCHAR* Label,const TCHAR* Asset,double Scale,bool Active,FVector Offset=FVector::ZeroVector){
        auto*& A=WakeObjects.FindOrAdd(Key);
        if(!IsValid(A)&&Active) {
            A=Item(TEXT("wake/")+Key.ToString(),TEXT("wake"),Label,WakeSpot(At,Index)+Offset,Index,Key.ToString());
            if(!A)return;A->Mesh->SetStaticMesh(Art(Asset));DressMesh(A->Mesh,Key=="paws"?TEXT("Ground"):TEXT("Steel"));A->SetActorScale3D(FVector(Scale));A->Mesh->SetCullDistance(48000);
            if(Key=="band"&&A->Mesh->GetStaticMesh()){
                auto* Foil=NewObject<UStaticMeshComponent>(A);Foil->SetupAttachment(A->Mesh);Foil->SetStaticMesh(Art(TEXT("SM_KitFold")));
                Foil->SetRelativeLocation(FVector(4,-3,A->Mesh->GetStaticMesh()->GetBoundingBox().Max.Z));Foil->SetRelativeScale3D(FVector(2.3));
                Foil->SetCollisionEnabled(ECollisionEnabled::NoCollision);Foil->RegisterComponent();
            }
            if(Key=="juice")for(int I=0;I<A->Mesh->GetNumMaterials();++I)A->Mesh->SetMaterial(I,Material(TEXT("Greenline")));
        }
        if(IsValid(A)&&Key=="juice"&&A->Mesh->GetNumChildrenComponents()==0){
            auto JarPart=[&](FVector Scale,double Height,const TCHAR* Mat){
                auto* M=NewObject<UStaticMeshComponent>(A);M->SetupAttachment(A->Mesh);
                M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
                M->SetRelativeLocation({0,0,Height});M->SetRelativeScale3D(Scale);M->SetMaterial(0,Material(Mat));M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->RegisterComponent();};
            A->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
            A->Mesh->SetWorldScale3D({.36,.36,.37});A->Mesh->SetMaterial(0,Material(TEXT("Greenline")));
            JarPart({1.09,1.09,.11},-48,TEXT("Glass"));JarPart({1.06,1.06,.11},44,TEXT("Glass"));
            JarPart({.80,.80,.35},64,TEXT("Glass"));JarPart({.91,.91,.12},85,TEXT("DarkMetal"));
        }
        if(IsValid(A)){A->SetActorHiddenInGame(!Active);A->SetActorEnableCollision(Active);}
    };
    const bool First=WakeLayout==INDEX_NONE;
    if(First){
        // Older slots already passed these three unavoidable discoveries. Leave any queued live radio intact.
        int Visited=(S.Chapter>=FirstHerd?1<<CaseFold:0)|(S.Chapter>=Dispatch||S.Seat==Car?1<<DashFold:0)|((S.Silenced|S.Retuned)?1<<Carrier:0);
        S.TrailHeard|=Visited&~S.TrailFound;S.TrailFound|=Visited;
    }
    Node("thrum","thrum",0,TEXT("Thrum / listen for Kit"),TEXT("SM_MoltSpitter"),.92,S.Chapter<=Triage);
    Node("band","band",0,TEXT("Examine the evacuation wristband"),TEXT("SM_FieldKit"),.55,S.Chapter==Triage,FVector(0,0,15));
    Node("herd","herd",0,TEXT("Thrum / guide the crossing herd"),TEXT("SM_MoltSpitter"),.98,S.Chapter==FirstHerd);
    Node("glade0","glade",0,TEXT("Quiet glade / settle the first herd"),TEXT("SM_Sign"),1,S.Chapter>=FirstHerd&&S.Chapter<Home);
    Node("garage","garage",0,TEXT("SALT-7 / roadside motor pool"),TEXT("SM_Robot"),1,S.Chapter>=FirstHerd);
    Node("tracks","tracks",0,TEXT("Examine the crossing's hoofprint pattern"),TEXT("SM_HoofReplay"),1.55,S.Chapter>=FirstHerd);
    if(S.Chapter==Home)if(auto* Tracks=WakeObjects.FindRef("tracks"))Tracks->Mesh->SetMaterial(0,Material(TEXT("Ground")));
    Node("fernfold","fernfold",0,TEXT("Inspect the folded candy wrapper"),TEXT("SM_KitFold"),1.65,S.Chapter>=FirstHerd,FVector(0,0,23));
    Node("paws","paws",0,TEXT("Inspect the five-toed tracks"),TEXT("SM_BearSign"),1.6,S.Chapter>=Runway);
    Node("liftfold","liftfold",0,TEXT("Inspect the foil in the lift grille"),TEXT("SM_KitFold"),1.6,S.Chapter>=Cell,FVector(0,0,67));
    Node("dispatch","dispatch",0,TEXT("East Dispatch / search records and take herd jobs"),TEXT("SM_Relay"),1.2,true);
    for(int I=1;I<=Herds;++I){
        const bool Done=((S.Silenced|S.Retuned)&(1<<(I-1)))!=0;
        Node(Numbered(TEXT("source"),I),"source",I,TEXT("Relay source / field crew"),TEXT("SM_Relay"),1.15,S.Chapter>=Surveys&&!Done);
        Node(Numbered(TEXT("glade"),I),"glade",I,Done?TEXT("Restored herd / safe building ground"):TEXT("Glade receiver / guide or calibrate"),TEXT("SM_Sign"),1,S.Chapter>=Surveys);
        if(auto* A=WakeObjects.FindRef(Numbered(TEXT("glade"),I)))A->Label=Done?TEXT("Restored herd / safe building ground"):TEXT("Glade receiver / guide or calibrate");
    }
    Node("runway","runway",0,TEXT("Thrum / clear runway growth"),TEXT("SM_MoltRunner"),1,S.Chapter>=Runway&&S.Chapter<=Flight);
    Node("hangar","hangar",0,TEXT("SALT-7 / Hollow Field hangar"),TEXT("SM_Robot"),1,S.Chapter>=Runway);
    Node("field1","field1",0,TEXT("Crown Field / call aircraft"),TEXT("SM_CrossingSignal"),.8,(S.Airfields&2)!=0);
    Node("vault","vault",0,TEXT("Meet your friends / MegaWAKE vault"),TEXT("SM_Relay"),1.15,S.Chapter>=Vault&&S.Chapter<Home);
    Node("juice","juice",0,TEXT("Thrum's glowing MORPHING JUICE"),TEXT("SM_Core"),.88,S.Chapter==Vault,FVector(0,0,98));
    Node("frame","frame",0,TEXT("SALT-7's unoccupied Supertech frame"),TEXT("SM_SupertechWarden"),1,S.Chapter==Vault);
    Node("burrow","burrow",0,TEXT("Root seam / burrow into custody"),TEXT("SM_BellowsHive"),.24,S.Chapter==Passage&&S.Body==Bug);
    Node("gate","gate",0,TEXT("Freight gate / authorize entry"),TEXT("SM_ElectronicsCase"),1,S.Chapter==Passage&&S.Body==Frame,FVector(0,0,50));
    Node("kit","cell",0,TEXT("Kit Venn / open custody cell"),TEXT("SM_Contact"),.73,S.Chapter==Cell||S.Chapter==Escort);
    if(First&&S.Chapter==Escort&&!S.EscortPosition.IsNearlyZero())if(auto* Kid=WakeObjects.FindRef("kit"))Kid->SetActorLocation(S.EscortPosition);
    Node("evac","evac",0,TEXT("Evacuation beacon / get Kit clear"),TEXT("SM_CrossingSignal"),.8,S.Chapter>=Escort);
    Node("strike","strike",0,TEXT("Sunseed / end the MegaWAKE operation"),TEXT("SM_Relay"),1,S.Chapter==Strike);
    Node("home","home",0,TEXT("Kit Venn / the morning after"),TEXT("SM_Contact"),.73,S.Chapter==Home);
    Node("homebug","home",0,TEXT("Thrum / the new refuge"),TEXT("SM_MoltSpitter"),.82,S.Chapter==Home,FVector(-540,580,0));
    Node("homesalt","home",0,TEXT("SALT-7 / rebuilding crew"),TEXT("SM_Robot"),1,S.Chapter==Home,FVector(-325,785,0));
    auto Decor=[&](const TCHAR* Asset,const TCHAR* Mat,FVector At,FVector Scale,FRotator R=FRotator::ZeroRotator){
        auto* A=Static(Asset,Mat,At,Scale,R);WakeScenery.Add(A);return A;
    };
    if(First) {
        auto Dressing=[&](FName Node,FVector2D Offset,const TCHAR* Mesh,FVector Scale,double Yaw,double Rise=0.){
            const auto XY=Location(S,Node)+Offset;
            auto* A=Decor(Mesh,TEXT("Bark"),FVector(XY,SurfaceHeight(XY.X,XY.Y)+Rise),Scale,FRotator(0,Yaw,0));A->SetActorEnableCollision(false);
        };
        Dressing("fernfold",{-5,3},TEXT("SM_CedarFall"),{2.1,1.85,1.65},29);
        for(int K=0;K<6;++K)Dressing("fernfold",{-55.-K*82,41.-K*43},TEXT("SM_BentBracken"),FVector(.72+(K%3)*.19),K*83.);
        Dressing("paws",{132,-32},TEXT("SM_Rock"),{1.45,.83,.48},42);
        Dressing("paws",{-49,24},TEXT("SM_AmberCaps"),FVector(.75),29);
        const FVector Grille=WakeSpot("liftfold")+FVector(18,0,31);
        Decor(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("DarkMetal"),Grille,{.17,.79,.66})->SetActorEnableCollision(false);
        for(int Slat=0;Slat<5;++Slat)Decor(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Steel"),Grille+FVector(-11,0,Slat*11.-20.),{.06,.78,.022})->SetActorEnableCollision(false);
        auto Sign=[&](const TCHAR* Words,FVector At,double Angle,float Size){
            auto* Post=Decor(TEXT("SM_Sign"),TEXT("DarkMetal"),At,FVector(.72),FRotator(0,Angle,0));
            auto* Letter=NewObject<UTextRenderComponent>(Post);Letter->SetupAttachment(Post->GetRootComponent());Letter->RegisterComponent();
            Letter->SetWorldLocation(At+FVector(0,0,236));Letter->SetWorldRotation(FRotator(0,Angle-90,0));Letter->SetText(FText::FromString(Words));
            Letter->SetWorldSize(Size);Letter->SetHorizontalAlignment(EHTA_Center);Letter->SetTextRenderColor(FColor(229,213,157));Letter->SetCullDistance(18500);};
        for(int I=0;I<2;++I) {
            const auto C=ETWake::Field(I,S);const double Z=StripZ(*this,I);
            Sign(I?TEXT("CROWN FIELD"):TEXT("HOLLOW FIELD"),FVector(C+FVector2D(-3850,-795),Z),0,33);
            FVector Hangar=FVector(C+FVector2D(-3600,-1500),SurfaceHeight(C.X-3600,C.Y-1500));
            for(int X:{-1,1})for(int Y:{-1,1})Decor(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Rust"),Hangar+FVector(X*450.,Y*295.,175),{.22,.22,3.5});
            Decor(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Steel"),Hangar+FVector(0,0,356),{10.,6.8,.14});
            auto* Bed=Decor(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Asphalt"),FVector(C,Z-9),{118,13.6,.16});
            Bed->SetActorEnableCollision(true);
            for(int K=-5;K<=5;++K) {auto* Stripe=Decor(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Plaster"),FVector(C+FVector2D(K*950.,0),Z+1),{2.8,.18,.025});Stripe->SetActorEnableCollision(false);}
            for(int Side:{-1,1})for(int K=-2;K<=2;++K) {
                auto* Lamp=Decor(TEXT("SM_Core"),TEXT("Signal"),FVector(C+FVector2D(K*2400.,Side*645.),Z),FVector(.42));Lamp->SetActorEnableCollision(false);}
        }
        const FVector V=WakeSpot("vault"),C=WakeSpot("cell");
        Sign(TEXT("fraudCORP / CUSTODY"),C+FVector(-530,415,0),90,27);
        auto Slab=[&](FVector At,FVector Size,const TCHAR* M){return Decor(TEXT("/Engine/BasicShapes/Cube.Cube"),M,At,Size/100.);};
        for(int Sgn:{-1,1}) {
            Slab(V+FVector(40,Sgn*365.,175),{930,32,350},TEXT("DarkMetal"));
            Slab(C+FVector(0,Sgn*355.,150),{650,27,300},TEXT("Steel"));
            Decor(TEXT("SM_Lamp"),TEXT("Signal"),V+FVector(-320,Sgn*290.,0),FVector(.85));
            auto* Post=Decor(TEXT("SM_CrossingSignal"),TEXT("Signal"),WakeSpot("roadgate")+FVector(0,Sgn*350.,0),FVector(.75));Post->SetActorRotation(FRotator(0,90,0));
        }
        Slab(V+FVector(40,0,0),{930,745,14},TEXT("DarkMetal"));
        auto* Bulkhead=Slab(V+FVector(477,0,174),{34,755,356},TEXT("Steel"));
        for(int Side:{-1,1}){
            Slab(V+FVector(451,Side*257.,171),{22,24,330},TEXT("DarkMetal"));
            Slab(V+FVector(436,Side*254.,180),{8,9,219},Side==1?TEXT("CircuitLight"):TEXT("ResinLight"));
            auto* Lamp=NewObject<UPointLightComponent>(Bulkhead);Lamp->SetupAttachment(Bulkhead->GetRootComponent());Lamp->SetCastShadows(false);
            Lamp->SetIntensity(21000);Lamp->SetAttenuationRadius(780);Lamp->SetLightColor(Side==1?FLinearColor(.57,.82,1):FLinearColor(.65,1,.54));
            Lamp->RegisterComponent();Lamp->SetWorldLocation(V+FVector(172,Side*253.,234));
        }
        auto* Name=NewObject<UTextRenderComponent>(Bulkhead);Name->SetupAttachment(Bulkhead->GetRootComponent());Name->RegisterComponent();
        Name->SetAbsolute(false,false,true);Name->SetWorldScale3D(FVector::OneVector);Name->SetWorldLocation(V+FVector(451,0,236));Name->SetWorldRotation(FRotator(0,180,0));
        Name->SetText(FText::FromString(TEXT("fraudCORP  /  MEGAWAKE")));Name->SetHorizontalAlignment(EHTA_Center);Name->SetWorldSize(21);Name->SetTextRenderColor(FColor(189,222,194));
        Slab(V+FVector(210,-165,39),{85,85,78},TEXT("DarkMetal"));
        Slab(V+FVector(40,0,375),{930,755,27},TEXT("AlloyHull"));
        auto* Shutter=Slab(C+FVector(-330,0,135),{22,690,270},TEXT("AlloyHull"));Shutter->Tags.Add(TEXT("WakeShutter"));
        Slab(C+FVector(0,0,330),{650,735,24},TEXT("DarkMetal"));
        Decor(TEXT("SM_MoltSpitter"),TEXT("Carapace"),V+FVector(230,-275,0),FVector(.81))->Tags.Add(TEXT("WakeVaultFriend"));
        Decor(TEXT("SM_Robot"),TEXT("Greenline"),V+FVector(230,275,0),FVector(.85))->Tags.Add(TEXT("WakeVaultFriend"));
        auto* Tower=Decor(TEXT("SM_ParallaxSpire"),TEXT("CircuitLight"),C+FVector(730,-950,0),FVector(.76));Tower->Tags.Add(TEXT("WakeTransmitter"));
        for(int K=0;K<5;++K) {
            auto* Growth=Decor(TEXT("SM_Fern"),TEXT("Leaf"),WakeSpot("runway")+FVector((K-2)*68.,150,0),FVector(2.3));Growth->Tags.Add(TEXT("WakeGrowth"));Growth->SetActorEnableCollision(false);}
    }
    for(auto* A:WakeScenery)if(IsValid(A)){
        if(A->ActorHasTag(TEXT("WakeVaultFriend"))){bool Present=S.Chapter>=Vault&&S.Chapter<Home;A->SetActorHiddenInGame(!Present);A->SetActorEnableCollision(Present);}
        if(A->ActorHasTag(TEXT("WakeShutter"))){const bool Open=S.Body==Frame&&S.Chapter>=Cell;A->SetActorHiddenInGame(Open);A->SetActorEnableCollision(!Open);}
        if(A->ActorHasTag(TEXT("WakeGrowth")))A->SetActorHiddenInGame(S.Chapter>=Hangar);
        if(A->ActorHasTag(TEXT("WakeTransmitter"))){A->SetActorHiddenInGame(S.Chapter==Home);A->SetActorEnableCollision(S.Chapter!=Home);}
    }
    if(S.Chapter>=FirstHerd&&!IsValid(WakeCar)){WakeCar=GetWorld()->SpawnActor<AETWakeVehicle>();WakeCar->Prepare(*this,Car);}
    if(S.Chapter>=Runway&&!IsValid(WakePlane)){WakePlane=GetWorld()->SpawnActor<AETWakeVehicle>();WakePlane->Prepare(*this,Plane);}
    if(S.Chapter==Home&&WakeLayout!=Home) {
        FVector Mast=WakeSpot("cell")+FVector(730,-950,0);
        for(int I=0;I<4;++I)Decor(TEXT("SM_Rubble"),TEXT("DarkMetal"),Mast+FVector((I-2)*145.,(I%2)*190.,0),FVector(1.4),FRotator(0,I*71.,0));
        auto* Flare=Decor(TEXT("/Engine/BasicShapes/Sphere.Sphere"),TEXT("Signal"),Mast+FVector(0,0,140),FVector(1));Flare->Tags.Add(TEXT("WakeFlare"));Flare->SetActorEnableCollision(false);Flare->SetActorHiddenInGame(true);
        const FVector Base=WakeSpot("home");
        Decor(S.Body==Bug?TEXT("SM_HiveNursery"):TEXT("SM_ComputeHall"),TEXT("Greenline"),Base+FVector(-900,910,0),FVector(.42));
        for(int N=0;N<S.EndingHerds;++N) {
            const FVector P=Base+FVector(590+N*125.,675+(N%3)*177.,0);
            auto* Tree=Decor(N%3?TEXT("SM_Fern"):TEXT("SM_Birch"),TEXT("Leaf"),P,FVector(.7+N*.04));Tree->SetActorEnableCollision(false);
            auto* A=GetWorld()->SpawnActor<AETAgent>(P+FVector(-70,-530,0),FRotator::ZeroRotator);
            A->WakeHerd=0;A->HerdMember=N+4;A->Configure(this,FString::Printf(TEXT("wake/home%d"),N),TEXT("deer"));WakeDeer.Add(A);
        }
    }
    const int Last=S.Chapter>=Surveys?Herds:0;
    for(int I=0;I<=Last;++I) {
        if(WakeDeer.ContainsByPredicate([I](AETAgent* A){return IsValid(A)&&A->WakeHerd==I&&A->HerdMember==0;}))continue;
        bool Restored=I?((S.Silenced|S.Retuned)&(1<<(I-1)))!=0:S.Chapter>=Garage;
        for(int K=0;K<3;++K){
            auto XY=(Restored?Glade(I):Herd(I).Center)+FVector2D(100+K*84.,(K-1)*160.);
            auto* A=GetWorld()->SpawnActor<AETAgent>(FVector(XY,SurfaceHeight(XY.X,XY.Y)+8),FRotator(0,K*25.,0));
            A->WakeHerd=I;A->HerdMember=K;A->Configure(this,FString::Printf(TEXT("wake/deer%d.%d"),I,K),TEXT("deer"));WakeDeer.Add(A);
        }
    }
    WakeLayout=S.Chapter;
}
void AETWorld::TickWake(AETCharacter& P,float D) {
    if(WakeLayout!=State->Wake.Chapter)RefreshWake();
    if(State->Wake.HasPlane()&&State->Wake.Seat==Foot&&!(State->Wake.Airfields&2)&&FVector2D::Distance(FVector2D(P.GetActorLocation()),Location(State->Wake,"field1"))<950){
        State->Wake.Airfields|=2;RefreshWake();Say(TEXT("Crown Field discovered / call your aircraft at the beacon"));Save();}
    WakeFlash=FMath::Max(0.f,WakeFlash-D);WakePulse+=D;
    if(P.WakePanelNode=="blast"&&State->Wake.Chapter==Home){
        for(auto* A:WakeScenery)if(IsValid(A)&&A->ActorHasTag(TEXT("WakeFlare"))){A->SetActorHiddenInGame(WakeFlash<1.f);
            double Wave=1.+(3.5-WakeFlash)*11.;A->SetActorScale3D(FVector(Wave,Wave,Wave*.6));}
        if(WakeFlash<=0)P.WakeAction(TEXT("wake-homecoming"));
    }
    if(State->Wake.Chapter>=Surveys&&State->Wake.Chapter<Home&&WakePulse>=.45f){
        for(int I:{2,4,6,8}){
            FVector2D NearHerd=Herd(I).Center+FVector2D(3800,-2400);
            if(FVector2D::Distance(FVector2D(P.GetActorLocation()),NearHerd)<11500){
                FVector At(NearHerd,SurfaceHeight(NearHerd.X,NearHerd.Y)+9);
                FrontFighter(FString::Printf(TEXT("front/wake/%d"),I),I%4==0?TEXT("runner"):TEXT("robot"),At);}}
    }
    if(State->Wake.Chapter==Escort)if(auto* Kit=WakeObjects.FindRef("kit")) {
        FVector Here=Kit->GetActorLocation();const auto Delta=P.GetActorLocation()-Here;
        if(Delta.Size2D()>155&&Delta.Size2D()<5000) {
            const auto Step=Delta.GetSafeNormal2D()*FMath::Min(Delta.Size2D()-150,710.*D);
            Here+=Step;Here.Z=SurfaceHeight(Here.X,Here.Y)+5;State->Wake.EscortPosition=Here;Kit->SetActorLocationAndRotation(Here,FRotator(0,Delta.Rotation().Yaw,0));}
    }
    if(State->Wake.Chapter==Vault&&FVector2D::Distance(FVector2D(P.GetActorLocation()),Location(State->Wake,"vault"))<1000&&P.Panel.IsEmpty()){
        const double T=State->Elapsed;P.Camera->SetRelativeLocation({0,0,72.+2.4*FMath::Sin(T*23.)});}
    if(WakePulse<.45f)return;WakePulse=0;
    if(P.Panel.IsEmpty()&&!HasSubtitle())PlayTrailClue();
    for(auto* Deer:WakeDeer)if(IsValid(Deer))Deer->UpdatePresence(P.GetActorLocation());
}
bool AETAgent::TickWakeDeer(float D,AETCharacter& P) {
    if(WakeHerd<0)return false;const auto& S=World->State->Wake;
    const bool Restored=S.Chapter==ETWake::Home||(WakeHerd?((S.Silenced|S.Retuned)&(1<<(WakeHerd-1)))!=0:S.Chapter>=Garage);
    const bool Leading=WakeHerd?(S.TaskSide==Bug&&S.TaskHerd==WakeHerd&&S.ReadSource):(S.Chapter==FirstHerd&&S.SharedLead);
    const FVector2D HomeXY=Restored?Glade(WakeHerd):Herd(WakeHerd).Center;
    const FVector2D Offset(95+HerdMember*66.,(HerdMember-1)*147.);
    const double T=Phase+HerdMember*.76;FVector2D Goal=HomeXY+Offset;FRotator R=GetActorRotation();double Speed=450;
    auto Pos=FVector2D(GetActorLocation());const auto PlayerXY=FVector2D(P.GetActorLocation());
    if(Restored) {
        Goal+=FVector2D(230*FMath::Sin(T*.23),195*FMath::Cos(T*.19));
        if(FVector2D::Distance(Pos,PlayerXY)<370)Goal+= (Pos-PlayerXY).GetSafeNormal()*260.;
        Speed=150;R.Pitch=-8+9*FMath::Sin(T*.8);R.Roll=0;
    } else if(Leading&&FVector2D::Distance(Pos,PlayerXY)<4100) {
        Goal=PlayerXY+Offset*.72;Speed=570;R.Pitch=2;R.Roll=0;
    } else {
        const int Beat=FMath::FloorToInt(FMath::Fmod(T,2.4)*3.3);
        Goal+=FVector2D(Beat%4==1?155.:0,Beat%4==3?-115.:0);Speed=820;
        R.Pitch=Beat%3==0?21:-11;R.Roll=Beat%3==0?-19:5;
    }
    const FVector2D Step=Goal-Pos;
    Pos+=Step.GetSafeNormal()*FMath::Min(Step.Size(),Speed*D);
    if(Step.Size()>15)R.Yaw=FMath::FixedTurn(float(R.Yaw),float(FMath::RadiansToDegrees(FMath::Atan2(Step.Y,Step.X))),D*(Restored?95:430));
    SetActorLocationAndRotation(FVector(Pos,World->SurfaceHeight(Pos.X,Pos.Y)+8),R);return true;
}
bool AETAgent::WakeEnemy() const {
    if(Id.StartsWith(TEXT("run/")))return false;
    if(!World||!World->State)return false;int Side=World->State->Wake.Winner();return Side==Bug?IsMachine():Side==Frame&&IsBrood();
}
bool AETAgent::WakeTruce() const {
    if(Id.StartsWith(TEXT("run/")))return false;
    if(!World||!World->State)return false;const auto& S=World->State->Wake;int Side=S.Winner();
    if(!Side&&S.Chapter>Missing&&FrontUnit)return IsMachine()||IsBrood();
    return Side==Bug?IsBrood():Side==Frame&&IsMachine();
}
