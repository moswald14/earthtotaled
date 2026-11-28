#include "ETGame.h"
#include "ETGeology.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

bool AETRegion::InHighwayGrounds(FVector2D P,double Extra) const
{
    for(const auto& Site:HighwaySites) if(Site.InGrounds(P,Extra)) return true;
    return false;
}

void AETRegion::BuildHighwaySites()
{
    const FVector2D Origin(GetActorLocation());
    auto Belongs=[&](FVector2D P) {
        return FMath::FloorToInt32(P.X/AETWorld::RegionSize)==Coord.X&&FMath::FloorToInt32(P.Y/AETWorld::RegionSize)==Coord.Y;
    };
    auto Ground=[&](FVector2D P,double Lift=0.) {return FVector(P,World->SurfaceHeight(P.X,P.Y)+Lift);};
    auto Prop=[&](const TCHAR* Mesh,const TCHAR* Material,FVector At,FVector Scale,double Yaw) {
        auto* A=World->Static(Mesh,Material,At,Scale,FRotator(0,Yaw,0)); A->SetOwner(this); Residents.Add(A); return A;
    };
    UHierarchicalInstancedStaticMeshComponent* Rails=nullptr;
    auto Steel=[&](FVector At,const FRotator& R,FVector Size) {
        if(!Rails) {
            Rails=NewObject<UHierarchicalInstancedStaticMeshComponent>(this); Rails->SetupAttachment(GetRootComponent());
            Rails->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EARTHtotaled/Art/SM_Block.SM_Block")));
            Rails->SetMaterial(0,World->Material(TEXT("Steel"))); Rails->SetCollisionProfileName(TEXT("BlockAll"));
            Rails->ComponentTags.Add(TEXT("ET_HighwayRail")); Rails->SetCullDistances(17000,32000); Rails->RegisterComponent(); Instances.Add(Rails);
        }
        Rails->AddInstance(FTransform(R,At-GetActorLocation(),Size));
    };
    for(const auto& Road:LocalRoads) if(Road.Highway) {
        const FVector2D D=(Road.B-Road.A).GetSafeNormal(),Right(-D.Y,D.X);
        const double Length=FVector2D::Distance(Road.A,Road.B);
        for(double T=400;T<Length-355;T+=800) {
            const auto Mid=Road.A+D*T; const int Bay=FMath::FloorToInt32(T/800);
            for(int Side:{-1,1}) {
                const auto At=Mid+Right*(Side*978.);
                if(!Belongs(At)||(Bay+(Side>0?9:0))%21>=13) continue;
                bool Exit=false; for(const auto& S:HighwaySites) if(FVector2D::Distance(At,S.Exit)<10200) {Exit=true;break;}
                if(Exit||World->Geology->SurfaceOpening(At.X,At.Y)<-12) continue;
                const auto Base=Ground(At),A=Ground(At-D*399),B=Ground(At+D*399);
                const auto Line=(B-A).Rotation(); const double L=FVector::Distance(A,B)/100.;
                Steel((A+B)*.5+FVector(0,0,70),Line,{L,.13,.25});
                Steel((A+B)*.5+FVector(0,0,53),Line,{L,.17,.075});
                Steel(Base+FVector(0,0,36),{0,Line.Yaw,0},{.15,.16,.78});
            }
        }
    }
    // Signals face approaching traffic. A major junction has a pair on opposing corners.
    for(const auto& Row:LocalRoads) if(!Row.Highway&&!Row.Feeder&&Row.Horizontal())
        for(const auto& Col:LocalRoads) if(!Col.Highway&&!Col.Feeder&&!Col.Horizontal()) {
            const FVector2D Cross(Col.A.X,Row.A.Y);
            if(Row.Distance(Cross)>1||Col.Distance(Cross)>1) continue;
            const uint32 Mix=World->SeedFor({FMath::RoundToInt32(Cross.X/100),FMath::RoundToInt32(Cross.Y/100)},61);
            if(Mix%3!=0&&FCString::Strcmp(Row.Name,TEXT("Market Street"))&&FCString::Strcmp(Row.Name,TEXT("Shelter Approach"))) continue;
            for(int Side:{-1,1}) {
                const auto At=Cross+FVector2D(Side*(Col.Width*.5+91),Side*(Row.Width*.5+106));
                if(Belongs(At)) Prop(TEXT("SM_CrossingSignal"),TEXT("Steel"),Ground(At),FVector::OneVector,Side<0?0.:180.);
            }
        }

    for(const auto& S:HighwaySites) {
        const double Face=FVector(S.Inward,0).Rotation().Yaw-90.;
        const auto Prefix=S.Id();
        const auto RelayBoard=S.At(650,-5900);
        if(Belongs(RelayBoard))if(auto* Board=World->Item(Prefix+TEXT("/dispatch"),TEXT("runboard"),TEXT("Faction relay / optional Threat Runs"),Ground(RelayBoard,5))){
            Board->SetOwner(this);Residents.Add(Board);
        }
        auto Building=[&](const TCHAR* Mesh,FVector2D P,double Scale,double Stretch,double Turn=0.) {
            if(!Belongs(P)) return;
            auto* A=Prop(Mesh,S.Bug?TEXT("Hide"):TEXT("Steel"),Ground(P),FVector(Scale,Scale,Scale*Stretch),Face+Turn);
            A->Tags.Add(S.Bug?TEXT("ET_BugHive"):TEXT("ET_SupertechCity"));
        };
        auto Locker=[&](FString Suffix,FVector2D P,const TCHAR* Table,const TCHAR* Label) {
            if(!Belongs(P)) return;
            if(auto* A=World->Item(Prefix+Suffix,TEXT("container"),Label,Ground(P,55),0,Table)) {A->SetOwner(this); Residents.Add(A);}
        };
        Building(S.Bug?TEXT("SM_BellowsHive"):TEXT("SM_ParallaxSpire"),S.Center,1.52,S.Bug?1.16:1.04);
        Locker(TEXT("/vault"),S.Center,S.Bug?TEXT("medical"):TEXT("electronics"),S.Bug?TEXT("Search resin-sealed salvage"):TEXT("Search the Parallax memory vault"));
        for(int Side:{-1,1}) {
            for(int I=0;I<4;++I) {
                const double Y=-5900.+I*3630.;
                const double X=Side*(I==0?6900.:I==3?7150.:8450.);
                const double Variation=.82+double((S.Variant+I*7+Side*3)&7u)*.057;
                const bool Spire=I==1||I==3;
                const TCHAR* Mesh=S.Bug?(Spire?TEXT("SM_BellowsHive"):TEXT("SM_HiveNursery")):(Spire?TEXT("SM_ParallaxSpire"):TEXT("SM_ComputeHall"));
                Building(Mesh,S.At(X,Y),Variation*(Spire?.83:1.48),.82+I*.085,Side*15);
                const auto Outer=S.At(Side*(I%2?4050.:3100.),Y+390);
                Building(S.Bug?TEXT("SM_HiveNursery"):TEXT("SM_ComputeHall"),Outer,S.Bug?1.12:1.05,1.,Side*(I%2?27:-12));
                if(I==1||I==2) Locker(FString::Printf(TEXT("/bay%d.%d"),Side,I),Outer+(S.Bug?S.Along*750.:FVector2D::ZeroVector),
                    I==1?TEXT("tools"):TEXT("armory"),S.Bug?TEXT("Recover a cocooned supply case"):TEXT("Search the Supertech maintenance bay"));
            }
            Building(S.Bug?TEXT("SM_HiveNursery"):TEXT("SM_ComputeHall"),S.At(Side*1450,-7480),.85,1.24);
        }
        for(int Approach:{-1,1}) {
            const FVector2D At=S.Exit+S.Along*(Approach*8600.)+S.Inward*1550.;
            if(!Belongs(At)) continue;
            const auto Base=Ground(At); const auto Forward=FVector(S.Along*Approach,0);
            const double Yaw=Forward.Rotation().Yaw; const FVector Right(-Forward.Y,Forward.X,0);
            for(int Side:{-1,1}) Prop(TEXT("SM_Block"),TEXT("Steel"),Base+Right*(Side*475.)+FVector(0,0,255),{.22,.23,5.65},Yaw);
            auto* Board=Prop(TEXT("SM_Block"),TEXT("DarkMetal"),Base+FVector(0,0,520),{.32,13.8,2.70},Yaw);
            auto* T=NewObject<UTextRenderComponent>(Board); T->SetupAttachment(Board->GetRootComponent()); T->SetAbsolute(true,true,true);
            T->SetWorldLocation(Base+Forward*19+FVector(0,0,520)); T->SetWorldRotation(Forward.Rotation());
            T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter); T->SetWorldSize(51);
            T->SetTextRenderColor(S.Bug?FColor(255,206,126):FColor(145,231,255));
            T->SetText(FText::FromString(FString::Printf(TEXT("%s     /     EXIT\n%s  /  %s"),S.Axis?TEXT("H2"):TEXT("H7"),
                S.Bug?TEXT("BUG"):TEXT("SUPERTECH"),*FString(S.Name()).ToUpper())));
            T->SetCullDistance(16500); T->RegisterComponent();
        }
    }
}
