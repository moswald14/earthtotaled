#include "ETField.h"
#include "ETWake.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

using Build=AETConstruction;
namespace {
    const TCHAR* const Crops[]={TEXT("empty"),TEXT("Cinderbean"),TEXT("Saltroot"),TEXT("Mendleaf")};
    const float Maturity[]={0,65,150,115};
    const float Demand[]={0,.14f,.24f,.42f};
    FIntPoint Junction(FVector V){return {FMath::RoundToInt32(V.X),FMath::RoundToInt32(V.Y)};}
}
const TCHAR* ETField::Crop(int Kind){return Crops[FMath::Clamp(Kind,0,3)];}
int ETField::Stage(const FETFarm& F){
    if(!F.Crop)return 0;
    return F.Growth>=1?4:F.Growth>=.62f?3:F.Growth>=.27f?2:1;
}
const TCHAR* ETField::FarmName(int Piece,int Style){
    static const TCHAR* const Names[][4]={
        {TEXT("Field cistern"),TEXT("Well pump"),TEXT("Field clinic"),TEXT("Prepared bed")},
        {TEXT("Dew bladder"),TEXT("Root pump"),TEXT("Mendleaf reclaimer"),TEXT("Brood garden")},
        {TEXT("Alloy cistern"),TEXT("Atmospheric condenser"),TEXT("Field fabricator clinic"),TEXT("Cultivation tray")}};
    return Names[FMath::Clamp(Style,0,2)][FMath::Clamp(Piece-Build::Cistern,0,3)];
}
FString ETField::FarmMesh(int Piece,int Style){
    if(Piece==Build::Bed)return TEXT("SM_FieldBed");
    const TCHAR* Suffix=Piece==Build::Cistern?TEXT("Tank"):Piece==Build::Pump?TEXT("Pump"):TEXT("Clinic");
    return FString(Style==ETWake::Bug?TEXT("SM_Root"):TEXT("SM_Field"))+Suffix;
}
bool AETWorld::FarmSite(FVector2D P) const {
    if(FVector2D::Distance(P,FVector2D(-4400,-4400))<=3200)return true;
    for(int I=0;I<=ETWake::Herds;++I){
        const bool Restored=I?((State->Wake.Silenced|State->Wake.Retuned)&(1<<(I-1)))!=0:State->Wake.Chapter>=ETWake::Garage;
        if(Restored&&FVector2D::Distance(P,ETWake::Glade(I))<=2800)return true;
    }
    return false;
}
bool AETWorld::PumpLinked(const FETBuild& Tank) const {
    if(Tank.Piece!=Build::Cistern)return false;
    for(const auto& Pair:State->Builds)if(Pair.Value.Piece==Build::Pump&&FVector::DistSquared(Pair.Value.Anchor,Tank.Anchor)<=FMath::Square(1250.))return true;
    return false;
}
namespace {
FIntPoint WaterCell(FVector Point){
    return {FMath::FloorToInt32(Point.X/1600.),FMath::FloorToInt32(Point.Y/1600.)};
}
TSet<int32> ConnectedChannels(const UETSave& S,TSet<FGuid>& Pumped){
    const auto& Lines=S.Furrows;
    TMultiMap<FIntPoint,FVector> Pumps;
    TMultiMap<FIntPoint,int32> Intakes;
    for(const auto& Pair:S.Builds)if(Pair.Value.Piece==Build::Pump)Pumps.Add(WaterCell(Pair.Value.Anchor),Pair.Value.Anchor);
    for(int I=0;I<Lines.Num();++I)Intakes.Add(WaterCell(Lines[I].Inlet),I);
    TMultiMap<FIntPoint,int32> Starts;
    for(int I=0;I<Lines.Num();++I) Starts.Add(Junction(Lines[I].Inlet),I);
    TSet<int32> Wet; TArray<int32> Queue;
    for(const auto& Pair:S.Builds){
        if(Pair.Value.Piece!=Build::Cistern)continue;
        const FVector Tank=Pair.Value.Anchor;const FIntPoint Cell=WaterCell(Tank);bool HasPump=false;
        for(int Y=-1;Y<=1&&!HasPump;++Y)for(int X=-1;X<=1&&!HasPump;++X)
            for(auto It=Pumps.CreateConstKeyIterator(Cell+FIntPoint(X,Y));It;++It)
                if(FVector::DistSquared(Tank,It.Value())<=FMath::Square(1250.)){HasPump=true;break;}
        if(!HasPump)continue;Pumped.Add(Pair.Key);
        for(int Y=-1;Y<=1;++Y)for(int X=-1;X<=1;++X)
            for(auto It=Intakes.CreateConstKeyIterator(Cell+FIntPoint(X,Y));It;++It){
                const int I=It.Value();const auto V=Lines[I].Inlet;
                if(!Wet.Contains(I)&&FVector::Dist2D(V,Tank)<=690&&FMath::Abs(V.Z-Tank.Z)<165){Wet.Add(I);Queue.Add(I);}
            }
    }
    for(int N=0;N<Queue.Num();++N){
        const FVector End=Lines[Queue[N]].Outlet;
        for(auto It=Starts.CreateConstKeyIterator(Junction(End));It;++It){
            const int I=It.Value();
            if(!Wet.Contains(I)&&FMath::Abs(Lines[I].Inlet.Z-End.Z)<3){Wet.Add(I);Queue.Add(I);}
        }
    }
    return Wet;
}
}
TSet<int32> AETWorld::FlowingFurrows() const{
    TSet<FGuid> Pumped;return ConnectedChannels(*State,Pumped);
}
void AETWorld::StepFarms(float D){
    if(D<=0)return;
    TSet<FGuid> Pumped;WetFurrows=ConnectedChannels(*State,Pumped);
    TMultiMap<FIntPoint,int32> Nearby;
    for(int I:WetFurrows){
        const auto& L=State->Furrows[I];const FVector Pad(435,435,0);
        const auto Low=WaterCell(L.Inlet.ComponentMin(L.Outlet)-Pad),High=WaterCell(L.Inlet.ComponentMax(L.Outlet)+Pad);
        for(int Y=Low.Y;Y<=High.Y;++Y)for(int X=Low.X;X<=High.X;++X)Nearby.Add({X,Y},I);
    }
    for(auto& Pair:State->Builds){
        auto& B=Pair.Value; auto& F=B.Farm;
        if(B.Piece==Build::Cistern&&Pumped.Contains(Pair.Key)&&F.Stock<8){
            F.Time+=D;const int Batch=FMath::Min(8-F.Stock,FMath::FloorToInt(F.Time/90.f));F.Stock+=Batch;F.Time=F.Stock==8?0:F.Time-90*Batch;
        }else if(B.Piece==Build::Clinic&&F.Orders>0&&F.Stock<4){
            F.Time+=D;const int Batch=FMath::Min3(4-F.Stock,F.Orders,FMath::FloorToInt(F.Time/75.f));
            F.Stock+=Batch;F.Orders-=Batch;F.Time=F.Stock==4||F.Orders==0?0:F.Time-75*Batch;
        }else if(B.Piece==Build::Bed){
            const int Before=ETField::Stage(F);
            bool Fed=false;
            for(auto It=Nearby.CreateConstKeyIterator(WaterCell(B.Anchor));It;++It){const auto& L=State->Furrows[It.Value()];const auto Point=FMath::ClosestPointOnSegment(B.Anchor,L.Inlet,L.Outlet);
                if(FVector::Dist2D(Point,B.Anchor)<=430&&FMath::Abs(Point.Z-B.Anchor.Z)<160){Fed=true;break;}}
            const int Crop=FMath::Clamp(F.Crop,0,3);
            const float Starting=F.Moisture;
            F.Moisture=FMath::Clamp(F.Moisture+D*(Fed?1.4f:-Demand[Crop]),0.f,100.f);
            const float WetTime=Fed||!Crop?D:FMath::Min(D,Starting/Demand[Crop]);
            if(Crop&&F.Growth<1){
                const bool Mineral=ETField::Mineral(FVector2D(B.Anchor),State->Seed)||F.Fertility>=3;
                const float Soil=1+.08f*F.Fertility+(Crop==2&&Mineral?.30f:0.f);
                F.Growth=FMath::Min(1.f,F.Growth+WetTime*Soil/Maturity[Crop]);
            }
            if(Crop){
                F.Time=F.Moisture>0?0.f:F.Time+D-WetTime;
                if(F.Time>=2*ETDay::Period){
                    ++State->FarmStores.FindOrAdd(ETField::Crop(Crop));
                    F.Crop=0;F.Growth=0;F.Time=0;
                }
            }
            if(ETField::Stage(F)!=Before)if(auto* A=LiveBuilds.FindRef(Pair.Key).Get()) A->Configure(this,B);
        }
    }
}
void AETWorld::TickFields(float D,AETCharacter& P){
    FieldClock+=D;FlowClock+=D;
    if(FieldClock>=1){StepFarms(FieldClock);FieldClock=0;}
    if(FlowClock>=.12f){FlowClock=0;if(!WetFurrows.IsEmpty()||FlowSurface)UpdateFlowView(P.GetActorLocation());}
}
void AETWorld::UpdateFlowView(FVector Viewer){
    auto Make=[&](const TCHAR* Asset,const TCHAR* Mat){
        auto* C=NewObject<UInstancedStaticMeshComponent>(this);
        C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Asset));C->SetMaterial(0,Material(Mat));C->SetCastShadow(false);C->RegisterComponent();return C;
    };
    if(!FlowSurface){FlowSurface=Make(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Irrigation"));FlowRipples=Make(TEXT("/Engine/BasicShapes/Sphere.Sphere"),TEXT("FlowGlint"));}
    TArray<FTransform> Water,Ripples;
    for(int I:WetFurrows){
        if(!State->Furrows.IsValidIndex(I))continue;const auto& L=State->Furrows[I];
        if(FVector::DistSquared(Viewer,L.Inlet)>FMath::Square(12500.))continue;
        const FVector Direction=L.Outlet-L.Inlet;const auto R=Direction.Rotation();const double Length=Direction.Size();
        Water.Emplace(R,(L.Inlet+L.Outlet)*.5,FVector(Length/100.,.66,.021));
        const int Marks=FMath::Max(1,FMath::RoundToInt(Length/225));
        for(int K=0;K<Marks;++K){
            const double U=FMath::Fmod(State->Elapsed*125./Length+double(K)/Marks,1.);
            const FVector At=FMath::Lerp(L.Inlet,L.Outlet,U);
            Ripples.Emplace(R,At+FVector(0,0,2.8),FVector(.09,.44,.025));
        }
        for(int K=0;K<2;++K){
            const double T=FMath::Frac(State->Elapsed*1.5+K*.5);
            Ripples.Emplace(FQuat::Identity,L.Outlet+R.RotateVector(FVector(T*14,(K?1:-1)*T*16,2+46*T*(1-T))),FVector(.043));
        }
    }
    auto Apply=[](UInstancedStaticMeshComponent* C,const TArray<FTransform>& Frames){
        if(C->GetInstanceCount()!=Frames.Num()){C->ClearInstances();if(Frames.Num())C->AddInstances(Frames,false,true,false);}
        else if(Frames.Num())C->BatchUpdateInstancesTransforms(0,Frames,true,true,true);
    };
    Apply(FlowSurface,Water);Apply(FlowRipples,Ripples);
}
