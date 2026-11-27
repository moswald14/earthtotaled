#include "ETNavigation.h"
#include "ETRailDistricts.h"
#include "ETTerrainMath.h"

namespace ETNav
{
    FIntPoint District(FVector2D P)
    {
        return {FMath::FloorToInt32(P.X/DistrictSpacing+.5),FMath::FloorToInt32(P.Y/DistrictSpacing+.5)};
    }
    FVector2D Refuge(FIntPoint C)
    {
        return FVector2D(C.X*DistrictSpacing,C.Y*DistrictSpacing)+(C==FIntPoint::ZeroValue?FVector2D(0,-1500):FVector2D(0,5660));
    }
    const TCHAR* RefugeName(FIntPoint C) {
        const int City=ETRing::CityIn(C);
        return City==INDEX_NONE?TEXT("Greenline refuge"):ETRing::CityName(City);
    }
    const TCHAR* FSite::Name() const
    {
        const TCHAR* Hives[]={TEXT("Bellows Hive"),TEXT("Cindercomb Hive"),TEXT("Ochre Hive"),TEXT("Rattlewell Hive")};
        const TCHAR* Cities[]={TEXT("Parallax City"),TEXT("Vesper Array"),TEXT("Meridian Stack"),TEXT("Lattice Nine")};
        return Bug?Hives[Variant%4]:Cities[Variant%4];
    }
    FString FSite::Id() const {return FString::Printf(TEXT("front/%d,%d/%d"),Cell.X,Cell.Y,Axis);}
    bool FSite::InGrounds(FVector2D P,double Extra) const
    {
        const auto D=P-Center;
        return FMath::Abs(FVector2D::DotProduct(D,Along))<10400+Extra&&FMath::Abs(FVector2D::DotProduct(D,Inward))<9000+Extra;
    }
    FSite Site(FIntPoint Cell,int Axis)
    {
        const bool East=Axis==0,Home=Cell==FIntPoint::ZeroValue;
        const FIntPoint Next=Cell+(East?FIntPoint(1,0):FIntPoint(0,1));
        uint32 H=ETMath::Mix(uint32(Cell.X)*73856093u^uint32(Cell.Y)*19349663u^uint32(Axis+1)*83492791u);
        FSite S; S.Cell=Cell; S.Axis=Axis; S.Variant=Axis==0&&(Home||Next==FIntPoint::ZeroValue)?0:H;
        S.Bug=((uint32(Cell.X)+uint32(Cell.Y)+uint32(Axis))&1u)!=0;
        S.Along=East?FVector2D(1,0):FVector2D(0,1);
        S.Inward=(East?FVector2D(0,1):FVector2D(1,0))*((H&32u)?1.:-1.);
        const double Run=65000+double((H>>8)%30001);
        const double First=Home?2600.:East?-1700.:3200.,Last=Next==FIntPoint::ZeroValue?2600.:East?-1700.:3200.;
        const double Cross=FMath::Lerp(First,Last,(Run-51000.)/58000.);
        S.Exit=FVector2D(Cell)*DistrictSpacing+(East?FVector2D(Run,Cross):FVector2D(Cross,Run));
        S.Center=S.Exit+S.Inward*(14000.+double((H>>17)%1800)); return S;
    }
    void Sites(const FBox2D& Area,TFunctionRef<void(const FSite&)> Visit)
    {
        const auto From=District(Area.Min),To=District(Area.Max);
        for(int Y=From.Y-1;Y<=To.Y;++Y) for(int X=From.X-1;X<=To.X;++X) for(int Axis=0;Axis<2;++Axis) {
            const auto S=Site({X,Y},Axis);
            FBox2D Bounds(S.Center-FVector2D(11500),S.Center+FVector2D(11500));
            Bounds+=S.Exit-FVector2D(9500); Bounds+=S.Exit+FVector2D(9500);
            if(Bounds.Intersect(Area)) Visit(S);
        }
    }
    double FRoad::Distance(FVector2D P) const
    {
        const FVector2D D=B-A;
        return FVector2D::Distance(P,A+D*FMath::Clamp(FVector2D::DotProduct(P-A,D)/D.SizeSquared(),0.,1.));
    }
    void Roads(const FBox2D& Area,TFunctionRef<void(const FRoad&)> Visit)
    {
        const auto From=District(Area.Min),To=District(Area.Max);
        auto Add=[&](FVector2D A,FVector2D B,const TCHAR* Name,double Width,bool Highway,bool Feeder=false) {
            FBox2D Bounds(ForceInit); Bounds+=A; Bounds+=B;
            if(Area.Intersect(Bounds.ExpandBy(Width*.5))) Visit({A,B,Name,Width,Highway,Feeder});
        };
        static const TCHAR* Streets[]={TEXT("Cinder Street"),TEXT("Orchard Street"),TEXT("Foundry Street"),TEXT("Market Street"),TEXT("Relief Street"),TEXT("Kiln Street"),TEXT("Ember Street")};
        static const TCHAR* Avenues[]={TEXT("Birch Avenue"),TEXT("Depot Avenue"),TEXT("Mill Avenue"),TEXT("Signal Avenue"),TEXT("Waterworks Avenue"),TEXT("Eastgate Avenue"),TEXT("Quarry Avenue")};
        static const TCHAR* HomeSt[]={TEXT("West Pump Street"),TEXT("Shelter Approach"),TEXT("Ash Street")};
        static const TCHAR* HomeAv[]={TEXT("Ranger Avenue"),TEXT("Signal Avenue"),TEXT("Garden Avenue")};
        for(int Y=From.Y-1;Y<=To.Y;++Y) for(int X=From.X-1;X<=To.X;++X) {
            const FIntPoint Cell(X,Y); const bool Home=Cell==FIntPoint::ZeroValue;
            const int RailCity=ETRing::CityIn(Cell);
            const FVector2D C(X*DistrictSpacing,Y*DistrictSpacing);
            if(Area.Intersect(FBox2D(C-FVector2D(25000),C+FVector2D(25000)))) {
                for(int I=Home?-1:-3;I<=(Home?1:3);++I) {
                    const double Row=Home?2600+I*7000:-1700+I*6400,Col=Home?2600+I*7000:3200+I*6400;
                    const double Edge=Home?16000.:22400.;
                    const auto RowName=!Home&&I==-1&&RailCity!=INDEX_NONE?ETRailDistrict::Street(RailCity).Name:Home?HomeSt[I+1]:Streets[I+3];
                    Add(C+FVector2D(-Edge,Row),C+FVector2D(Edge,Row),RowName,Home?600:800,false);
                    Add(C+FVector2D(Col,Home?-Edge:-20900.),C+FVector2D(Col,Home?Edge:23900.),Home?HomeAv[I+1]:Avenues[I+3],Home?600:800,false);
                }
            }
            if(!Home&&Area.Intersect(FBox2D(C+FVector2D(2200,9900),C+FVector2D(26900,28400)))) {
                // Foundry Crescent skirts the old yards and joins two open ends of the street grid.
                auto Previous=C+FVector2D(3200,23900);
                for(const FVector2D Bend:{FVector2D(3200,24750),{3750,26050},{5200,26900},{9600,27400},
                    {14200,26750},{18200,25100},{22300,21500},{24800,17500},{25100,13750},{24650,12050},{23600,11100},{22400,11100}}) {
                    const auto Next=C+Bend; Add(Previous,Next,TEXT("Foundry Crescent"),720,false,true); Previous=Next;
                }
                Add(C+FVector2D(9600,23900),C+FVector2D(9600,27400),TEXT("Quayside Lane"),540,false,true);
            }
            // Join the actual ends of the municipal streets. Bends occur outside both city grids.
            for(int Axis=0;Axis<2;++Axis) {
                const bool East=Axis==0;
                const bool NextHome=(Cell+(East?FIntPoint(1,0):FIntPoint(0,1)))==FIntPoint::ZeroValue;
                auto Point=[&](double Along,double Across){return C+(East?FVector2D(Along,Across):FVector2D(Across,Along));};
                const double Out=Home?16000.:East?22400.:23900.;
                const double In=DistrictSpacing-(NextHome?16000.:East?22400.:20900.);
                const double First=Home?2600.:East?-1700.:3200.;
                const double Last=NextHome?2600.:East?-1700.:3200.;
                const TCHAR* Name=East?TEXT("H7 / Blackpine Highway"):TEXT("H2 / Relay Highway");
                const auto A=Point(Out,First),B=Point(51000,First),D=Point(109000,Last),E=Point(In,Last);
                Add(A,B,Name,1860,true); Add(B,D,Name,1860,true); Add(D,E,Name,1860,true);
            }
        }
        Sites(Area,[&](const FSite& S) {
            const TCHAR* Ramp=S.Bug?TEXT("Moltbank Exit"):TEXT("Parallax Exit");
            const TCHAR* Street=S.Bug?TEXT("Husk Track"):TEXT("Index Circuit");
            const auto Merge=S.Exit+S.Inward*3500;
            for(int Side:{-1,1}) {
                auto Previous=S.Exit+S.Along*(Side*8500.);
                for(const FVector2D P:{FVector2D(6200,950),{3850,1850},{1900,2950},{0,3500}}) {
                    const auto Next=S.Exit+S.Along*(Side*P.X)+S.Inward*P.Y;
                    Add(Previous,Next,Ramp,590,false,true); Previous=Next;
                }
            }
            const auto Gate=S.At(0,-6900);
            Add(Merge,Gate,Ramp,720,false,true);
            for(int Side:{-1,1}) {
                auto Previous=Gate;
                for(const FVector2D P:{FVector2D(2950,-5350),{5400,-3350},{6050,500},{4750,3760},{2360,5640},{0,6100}}) {
                    const auto Next=S.At(P.X*Side,P.Y+(S.Bug?470.*FMath::Sin(P.X*.00048)*Side:0.));
                    Add(Previous,Next,Street,S.Bug?460.:780.,false,true); Previous=Next;
                }
            }
            Add(Gate,S.At(0,-1650),S.Bug?TEXT("Amber Mouth"):TEXT("Processional Way"),S.Bug?520.:840.,false,true);
        });
    }
    double Bearing(FVector2D D) {return FMath::Fmod(360.+FMath::RadiansToDegrees(FMath::Atan2(D.X,-D.Y)),360.);}
    FString Distance(double Cm) {return Cm>=100000.?FString::Printf(TEXT("%.2f km"),Cm/100000.):FString::Printf(TEXT("%.0f m"),Cm/100.);}
    bool FChart::Inside(FVector2D P) const {return P.X>=Position.X&&P.Y>=Position.Y&&P.X<=Position.X+Size.X&&P.Y<=Position.Y+Size.Y;}
    FChart Atlas(FVector2D V,FVector2D C,double Span)
    {
        const double U=FMath::Clamp(FMath::Min(V.X/1280.,V.Y/780.),.55,2.);
        return {{V.X*.045,64*U},{V.X*.91,V.Y-155*U},C,FMath::Clamp(Span,MinSpan,MaxSpan)};
    }
    bool Clip(const FBox2D& Box,FVector2D& A,FVector2D& B)
    {
        const FVector2D D=B-A; double Enter=0,Exit=1;
        for(int Axis=0;Axis<2;++Axis) {
            if(FMath::Abs(D[Axis])<1.e-9) {if(A[Axis]<Box.Min[Axis]||A[Axis]>Box.Max[Axis]) return false; continue;}
            double Lo=(Box.Min[Axis]-A[Axis])/D[Axis],Hi=(Box.Max[Axis]-A[Axis])/D[Axis];
            if(Lo>Hi) Swap(Lo,Hi);
            Enter=FMath::Max(Enter,Lo); Exit=FMath::Min(Exit,Hi);
            if(Enter>Exit) return false;
        }
        B=A+D*Exit; A+=D*Enter; return true;
    }
    FBox2D AtlasButton(const FChart& Chart,int Index)
    {
        const double U=Chart.Position.Y/64.,Width=Chart.Size.X/6.;
        const FVector2D Left=Chart.Position+FVector2D(Index*Width,Chart.Size.Y+11*U);
        return FBox2D(Left,Left+FVector2D(Width-4*U,33*U));
    }
}
