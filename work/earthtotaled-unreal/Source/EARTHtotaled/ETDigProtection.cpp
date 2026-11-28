#include "ETWake.h"
#include "ETGeology.h"
#include "ETGame.h"
#include "ETRing.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"

const TCHAR* UETGeology::DigRestriction(const FBox2D& Area,bool ShallowField) const
{
    const FVector2D Center=Area.GetCenter();
    const bool Lot=ShallowField&&World->FarmSite(Center);
    if(Lot)for(const auto& Foundation:World->ShelterBuildings)if(Area.Intersect(Foundation))return TEXT("Protected Shelter 09 foundation");
    if(!Lot&&ETWake::Reserved(World->State->Wake,Center,Area.GetExtent().Size()))return TEXT("Protected wildlife or airfield ground");
    const FVector2D Town(World->CityCenter(Center.X,Center.Y));
    const FVector2D Nearest(FMath::Clamp(Town.X,Area.Min.X,Area.Max.X),FMath::Clamp(Town.Y,Area.Min.Y,Area.Max.Y));
    if(!Lot&&FVector2D::DistSquared(Town,Nearest)<=FMath::Square(27000.)) return TEXT("Protected city ground / mine outside the district");

    // Keep the ground under each concrete pier, including piers in unloaded districts.
    constexpr double PierFoot=94.;
    for(int Line=0;Line<ETRing::Services;++Line) {
        const bool Straight=ETRing::Shuttle(Line);
        if(Straight) {
            if(!Area.ExpandBy(PierFoot).Intersect(FBox2D(ETRing::Dock(Line,0),ETRing::Dock(Line,1)))) continue;
        } else {
            const auto Origin=ETRing::Center+(Line==ETRing::East?ETRing::EastOffset():FVector2D::ZeroVector);
            if(FMath::Abs(FVector2D::Distance(Center,Origin)-ETRing::Radius)>Area.GetExtent().Size()+PierFoot) continue;
        }
        const int End=ETRing::SpanCount(Line)+(Straight?1:0);
        for(int I=0;I<End;I+=2) {
            const auto Pier=ETRing::Track(Line,I*ETRing::StepSize(Line));
            if(Area.ComputeSquaredDistanceToPoint(Pier)<=FMath::Square(PierFoot)) return TEXT("Protected Ringline pier / mine between the supports");
        }
    }

    if(FVector2D::Distance(Center,Town)<43500.+Area.GetExtent().Size()) {
        const FVector2D Tank(World->TowerLocation(ETNav::District(Center)));
        if(Area.Intersect(FBox2D(Tank-FVector2D(430.),Tank+FVector2D(430.)))) return TEXT("Protected water tower foundation");
    }
    const TCHAR* Reason=nullptr;
    ETNav::Sites(Area,[&](const ETNav::FSite& Site){
        const auto E=Site.Along.GetAbs()*10400.+Site.Inward.GetAbs()*9000.;
        if(Area.Intersect(FBox2D(Site.Center-E,Site.Center+E))) Reason=TEXT("Protected settlement / mine beyond the perimeter");
    });
    if(Reason) return Reason;
    ETNav::Roads(Area.ExpandBy(210.),[&](const ETNav::FRoad& Road){
        auto A=Road.A,B=Road.B;
        if(ETNav::Clip(Area.ExpandBy(Road.Width*.5+(Road.Highway?110.:Road.Feeder?0.:210.)),A,B))
            Reason=TEXT("Protected roadbed / move the entire cut clear of the shoulder");
    });
    if(Reason) return Reason;
    for(const auto& Pair:World->State->Builds) {
        const auto& Build=Pair.Value; const auto Extent=AETConstruction::Extents(Build.Piece);
        const auto B=FBox(-Extent,Extent).TransformBy(AETConstruction::Shape(Build));
        if(Area.Intersect(FBox2D(FVector2D(B.Min),FVector2D(B.Max)))) return TEXT("Protected foundation / salvage the structure first");
    }
    // The column also exists before its house or campsite has streamed into view.
    const auto From=ETRock::Chunk(FVector(Area.Min-FVector2D(2400.),0));
    const auto To=ETRock::Chunk(FVector(Area.Max+FVector2D(2400.),0));
    const auto First=ETRock::Region(From),Last=ETRock::Region(To);
    for(int Y=First.Y;Y<=Last.Y;++Y) for(int X=First.X;X<=Last.X;++X) for(int I=0;I<2;++I) {
        const auto Home=World->HouseAnchor({X,Y},I); const FVector2D H(Home),Camp=H+FVector2D(1600,0);
        if(!Area.Intersect(FBox2D(H-FVector2D(690,620),H+FVector2D(690,620)))&&
           !Area.Intersect(FBox2D(Camp-FVector2D(340,330),Camp+FVector2D(340,330)))) continue;
        if(World->InCity(Home)) continue;
        bool Omitted=false;
        const auto Check=FBox2D(H-FVector2D(1800),H+FVector2D(1800));
        ETNav::Sites(Check,[&](const ETNav::FSite& Site){Omitted|=Site.InGrounds(H,700);});
        ETNav::Roads(Check,[&](const ETNav::FRoad& Road){Omitted|=(Road.Highway||Road.Feeder)&&Road.Distance(H)<Road.Width*.5+750;});
        if(!Omitted) return TEXT("Protected homestead / mine beyond the buildings");
    }
    const auto A=ETRock::Chunk(FVector(Area.Min,0)),B=ETRock::Chunk(FVector(Area.Max,0));
    for(int Y=A.Y;Y<=B.Y;++Y) for(int X=A.X;X<=B.X;++X) {
        const auto* Props=GroundProps.Find({X,Y}); if(!Props) continue;
        for(const auto& Prop:*Props) if(auto* Actor=Prop.Get()) {
            const auto* C=Actor->GetStaticMeshComponent();
            if(!C->IsVisible()||!C->ComponentHasTag(TEXT("ET_Foundation"))) continue;
            const auto Name=C->GetStaticMesh()->GetFName();
            if(Name==TEXT("SM_Barrel")||Name==TEXT("SM_Wreck")||Name==TEXT("SM_Shelves")) continue;
            const auto Bounds=C->Bounds.GetBox();
            if(Area.Intersect(FBox2D(FVector2D(Bounds.Min),FVector2D(Bounds.Max)))) return TEXT("Protected structure / move clear of its footprint");
        }
    }
    return nullptr;
}
