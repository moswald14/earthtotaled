#include "ETField.h"
#include "ETGeology.h"
#include "ETWake.h"

namespace {
    constexpr double BasinSpacing=84000.;
    FIntPoint PreserveCell(FVector P){return {FMath::FloorToInt32(P.X/6400.),FMath::FloorToInt32(P.Y/6400.)};}
    double Wrapped(double X,double Span){return X-Span*FMath::FloorToDouble(X/Span+.5);}
}
void ETField::Basin(FVector2D P,int Seed,double& Stream,double& Lake){
    const double Phase=(uint32(Seed)%4093)*.00153;
    const double RiverY=14500+5200*FMath::Sin(P.X/18000.+Phase)+2650*FMath::Sin(P.X/7950.-Phase);
    Stream=FMath::Abs(Wrapped(P.Y-RiverY,BasinSpacing));
    const double X=Wrapped(P.X-47700,BasinSpacing)/6400.;
    const double Y=Wrapped(P.Y-RiverY,BasinSpacing)/3800.;
    Lake=FMath::Sqrt(X*X+Y*Y);
}
bool ETField::Mineral(FVector2D XY,int Seed){double Stream,Lake;Basin(XY,Seed,Stream,Lake);return Lake<1.06;}
void AETWorld::DrySeasonMigration(){
    if(State->DrySeason)return;
    // A legacy survivor's machinery, mine coordinates and dropped supplies retain their surface datum.
    if(HadSave){
        auto Keep=[&](FVector P){State->DryPreserves.Add(PreserveCell(P));};
        Keep(State->Position);Keep(State->WinchAnchor);
        if(State->Wake.HasCar())Keep(State->Wake.Roadster.Position);
        if(State->Wake.HasPlane())Keep(State->Wake.Aircraft.Position);
        if(!State->Wake.EscortPosition.IsNearlyZero())Keep(State->Wake.EscortPosition);
        for(const auto& B:State->Builds)Keep(B.Value.Anchor);
        for(const auto& Cut:State->BoreCuts)Keep(FVector(Cut)*UETGeology::CutQuantum);
        for(const auto& Cut:State->SquareCuts)Keep(FVector(Cut)*ETRock::Step);
        for(const auto& It:State->DropPositions)Keep(It.Value);
        for(const auto& K:State->MinedVeins)Keep(FVector((K.X+.5)*1150,(K.Y+.5)*1150,0));
    }
    State->DrySeason=true;
}
double AETWorld::DryDepth(double X,double Y) const {
    const auto District=ETNav::District({X,Y});
    const FVector2D Town(District.X*ETNav::DistrictSpacing,District.Y*ETNav::DistrictSpacing);
    const double Bounds=FVector2D::DistSquared({X,Y},Town);
    if(Bounds<31000.*31000.)return 0;
    double Stream,Lake;ETField::Basin({X,Y},State->Seed,Stream,Lake);
    double Cut=FMath::Max(335.*(1.-FMath::SmoothStep(.53,1.17,Lake)),180.*(1.-FMath::SmoothStep(240.,1750.,Stream)));
    if(Cut<=0)return 0;
    if(!State->DryPreserves.IsEmpty()){
        const auto Cell=PreserveCell(FVector(X,Y,0));
        for(int J=-1;J<=1;++J)for(int I=-1;I<=1;++I){
            const auto K=Cell+FIntPoint(I,J);if(!State->DryPreserves.Contains(K))continue;
            const double DX=FMath::Max(0.,FMath::Abs(X-(K.X+.5)*6400)-3200);
            const double DY=FMath::Max(0.,FMath::Abs(Y-(K.Y+.5)*6400)-3200);
            Cut*=FMath::SmoothStep(700.,2400.,FMath::Sqrt(DX*DX+DY*DY));
        }
    }
    return Cut*FMath::SmoothStep(31000.,39000.,FMath::Sqrt(Bounds));
}
void AETWorld::DrawDryBed(AETRegion& R){
    const auto Origin=R.GetActorLocation();
    FRandomStream Rand(SeedFor(R.Coord,22531));
    for(int N=0;N<75;++N){
        FVector At=Origin+FVector(Rand.FRandRange(500,RegionSize-500),Rand.FRandRange(500,RegionSize-500),0);
        const FVector2D XY(At);
        if(DryDepth(At.X,At.Y)<122||R.OnHighway(XY,550)||R.InHighwayGrounds(XY,500)||ETWake::Reserved(State->Wake,XY,360))continue;
        double S,L;ETField::Basin(XY,State->Seed,S,L);At.Z=SurfaceHeight(At.X,At.Y);
        const bool Dock=L<1.08;const FRotator Turn(0,Dock?Rand.FRandRange(0,360):85,0);
        auto* Prop=Static(Dock?TEXT("SM_StrandedDock"):TEXT("SM_DryCulvert"),TEXT("Rust"),At,FVector(1.6),Turn);
        if(Prop){Prop->SetOwner(&R);R.Residents.Add(Prop);}
        if(!Dock){
            const auto Side=Turn.RotateVector(FVector(160,0,0));FVector Hatch=At+Side;Hatch.Z=SurfaceHeight(Hatch.X,Hatch.Y)+6;
            if(auto* Cache=Item(FString::Printf(TEXT("waterservice/%d/%d"),R.Coord.X,R.Coord.Y),TEXT("siltcache"),TEXT("Open buried irrigation service hatch"),Hatch)){
                Cache->SetOwner(&R);R.Residents.Add(Cache);
            }
        }
        break;
    }
}
