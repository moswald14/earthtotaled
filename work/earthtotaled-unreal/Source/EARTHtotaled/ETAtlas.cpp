#include "ETGame.h"
#include "ETRingLine.h"
#include "ETRailDistricts.h"
#include "ETWarClock.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

namespace
{
    const FLinearColor Paper(.047,.083,.071),Panel(.022,.041,.035,.96),Ink(.88,.92,.83),Sage(.59,.74,.63);
    const FLinearColor Gold(.99,.71,.34),Blue(.38,.87,.93),Contour(.15,.23,.173),Grid(.107,.166,.137);
    const TCHAR* Winds[]={TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
    void Line(AETHUD& H,FVector2D A,FVector2D B,FLinearColor Color,float Width=1) {H.DrawLine(A.X,A.Y,B.X,B.Y,Color,Width);}
    void FillBox(AETHUD& H,const FBox2D& B,FLinearColor Color) {if(B.Max.X>B.Min.X&&B.Max.Y>B.Min.Y) H.DrawRect(Color,B.Min.X,B.Min.Y,B.Max.X-B.Min.X,B.Max.Y-B.Min.Y);}
    void TickText(AETHUD& H,const FString& S,FVector2D At,FLinearColor Color,float Scale) {
        float W,T; H.GetTextSize(S,W,T,GEngine->GetSmallFont(),Scale);
        H.Text(S,At.X-W*.5,At.Y,Color,Scale);
    }
    void Pin(AETHUD& H,FVector2D P,double R,bool City,FLinearColor C) {
        const FVector2D Top=P-FVector2D(0,R),Left=P-FVector2D(R,0),Right=P+FVector2D(R,0);
        Line(H,Left,Top,C,2); Line(H,Top,Right,C,2);
        if(City) {
            Line(H,Left,Left+FVector2D(0,R*.85),C,2); Line(H,Right,Right+FVector2D(0,R*.85),C,2);
            Line(H,Left+FVector2D(0,R*.85),Right+FVector2D(0,R*.85),C,2);
        } else {Line(H,Right,P+FVector2D(0,R),C,2); Line(H,P+FVector2D(0,R),Left,C,2);}
    }
    void Arrow(AETHUD& H,FVector2D P,FVector2D D,float Size,FLinearColor Color) {
        const FVector2D Side(-D.Y,D.X),Tip=P+D*Size,Back=P-D*Size*.65;
        const auto L=Back+Side*Size*.64,R=Back-Side*Size*.64;
        Line(H,L,Tip,Panel,5); Line(H,R,Tip,Panel,5); Line(H,L,R,Panel,5);
        Line(H,L,Tip,Color,2); Line(H,R,Tip,Color,2); Line(H,L,P,Color,2); Line(H,R,P,Color,2);
    }
    void RefreshContours(ETNav::FContours& Cache,AETWorld& W,const ETNav::FChart& Chart) {
        const auto Bounds=Chart.Bounds();
        const double Step=FMath::Pow(2.,FMath::RoundToDouble(FMath::Log2(Chart.Span/24.)));
        if(Cache.Seed==W.State->Seed&&Cache.Step==Step&&Cache.Area.bIsValid&&Cache.Area.IsInside(Bounds)) return;
        const auto Cell=[&](FVector2D P){return FIntPoint(FMath::FloorToInt32(P.X/Step),FMath::FloorToInt32(P.Y/Step));};
        const auto A=Cell(Bounds.Min)-FIntPoint(3,3),B=Cell(Bounds.Max)+FIntPoint(4,4);
        Cache.Area=FBox2D(FVector2D(A)*Step,FVector2D(B)*Step); Cache.Step=Step; Cache.Seed=W.State->Seed; Cache.Lines.Reset();
        const int NX=B.X-A.X+1,NY=B.Y-A.Y+1;
        TArray<double> H; H.Reserve(NX*NY);
        for(int Y=A.Y;Y<=B.Y;++Y) for(int X=A.X;X<=B.X;++X) H.Add(W.Height(X*Step,Y*Step));
        const double Interval=Step>5500?1600:Step>1800?800:400;
        for(int Y=0;Y<NY-1;++Y) for(int X=0;X<NX-1;++X) {
            const int I=Y*NX+X;
            const double Z[]={H[I],H[I+1],H[I+NX+1],H[I+NX]};
            const FVector2D P((X+A.X)*Step,(Y+A.Y)*Step);
            const FVector2D V[]={P,P+FVector2D(Step,0),P+FVector2D(Step),P+FVector2D(0,Step)};
            const double Min=FMath::Min(FMath::Min(Z[0],Z[1]),FMath::Min(Z[2],Z[3])),Max=FMath::Max(FMath::Max(Z[0],Z[1]),FMath::Max(Z[2],Z[3]));
            for(double Level=(FMath::CeilToDouble(Min/Interval-.37)+.37)*Interval;Level<Max;Level+=Interval) {
                FVector2D Cross[4]; int Count=0;
                for(int K=0;K<4;++K) {
                    const int J=(K+1)%4;
                    if((Z[K]<=Level)!=(Z[J]<=Level)) Cross[Count++]=FMath::Lerp(V[K],V[J],(Level-Z[K])/(Z[J]-Z[K]));
                }
                for(int K=1;K<Count;K+=2) Cache.Lines.Emplace(Cross[K-1],Cross[K]);
            }
        }
    }
}
void AETHUD::DrawChart(AETCharacter* P,const ETNav::FChart& Chart,bool Small)
{
    auto& W=*P->World; const auto& S=*W.State;
    const auto Box=Chart.Bounds(); const FBox2D Screen(Chart.Position,Chart.Position+Chart.Size);
    const double Scale=Chart.Scale(),U=Small?FMath::Clamp(Chart.Size.X/205.,.7,1.5):Chart.Position.Y/64.;
    FillBox(*this,Screen,Paper);
    auto Stroke=[&](FVector2D A,FVector2D B,FLinearColor Color,float Width=1) {
        if(ETNav::Clip(Box,A,B)) Line(*this,Chart.Plot(A),Chart.Plot(B),Color,Width);
    };
    auto Block=[&](FVector2D Min,FVector2D Max,FLinearColor Color) {
        const FVector2D A=Chart.Plot(FVector2D(FMath::Max(Box.Min.X,Min.X),FMath::Max(Box.Min.Y,Min.Y)));
        const FVector2D B=Chart.Plot(FVector2D(FMath::Min(Box.Max.X,Max.X),FMath::Min(Box.Max.Y,Max.Y)));
        FillBox(*this,FBox2D(A,B),Color);
    };
    auto& Cache=Contours[Small?0:1]; RefreshContours(Cache,W,Chart);
    for(const auto& L:Cache.Lines) Stroke(L.Key,L.Value,Contour);
    const double Unit=Chart.Span<40000?5000:Chart.Span<110000?10000:Chart.Span<230000?25000:50000;
    for(double X=FMath::CeilToDouble(Box.Min.X/Unit)*Unit;X<Box.Max.X;X+=Unit) Stroke({X,Box.Min.Y},{X,Box.Max.Y},Grid);
    for(double Y=FMath::CeilToDouble(Box.Min.Y/Unit)*Unit;Y<Box.Max.Y;Y+=Unit) Stroke({Box.Min.X,Y},{Box.Max.X,Y},Grid);
    const auto Here=FVector2D(P->GetActorLocation()); const auto Home=ETNav::District(Here);
    const auto City=ETNav::Refuge(Home),Tracked=S.MapPins.IsValidIndex(S.TrackedPin)?S.MapPins[S.TrackedPin]:City;
    const auto From=ETNav::District(Box.Min),To=ETNav::District(Box.Max);
    const bool Overview=!Small&&Chart.Span>450000;
    const FLinearColor Lot(.25,.30,.237),Town(.084,.116,.095);
    for(int Y=From.Y;Y<=To.Y;++Y) for(int X=From.X;X<=To.X;++X) {
        const FIntPoint Cell(X,Y); const FVector2D C(X*ETNav::DistrictSpacing,Y*ETNav::DistrictSpacing);
        const bool Shelter=Cell==FIntPoint::ZeroValue; const double Edge=Shelter?15900.:22400.;
        if(!Box.Intersect(FBox2D(C-FVector2D(Edge),C+FVector2D(Edge,24000)))) continue;
        Block(C-FVector2D(Edge),C+FVector2D(Edge),Town);
        if(Chart.Span>170000) continue;
        if(Shelter) {for(const auto& B:W.ShelterBuildings) Block(B.Min,B.Max,Lot); continue;}
        for(int Row=-3;Row<=3;++Row) for(int Col=-3;Col<=3;++Col) {
            const FVector2D At=C+FVector2D(Col*6400,Row*6400);
            if(!Box.ExpandBy(3300).IsInsideOrOn(At)) continue;
            auto Building=[&](FVector2D Offset,FVector2D Half){Block(At+Offset-Half,At+Offset+Half,Lot);};
            if(Col==-1&&Row==0) Building(FVector2D::ZeroVector,{1450,880});
            else if(Col==0&&Row==1) for(int Side:{-1,1}) Building({Side*900.,100},{250,250});
            else if(Col==2&&Row==2) Building(FVector2D::ZeroVector,{710,615});
            else for(int Side:{-1,1}) Building({Side*((Col==0&&Row==-1)?1500.:1600.),0},{610,510});
        }
    }
    TArray<ETNav::FSite,TInlineAllocator<12>> Sites;
    if(!Overview) ETNav::Sites(Box.ExpandBy(1800),[&](const ETNav::FSite& Site) {Sites.Add(Site);});
    TArray<ETNav::FRoad,TInlineAllocator<80>> Roads;
    if(!Overview) ETNav::Roads(Box.ExpandBy(1800),[&](const ETNav::FRoad& Road){Roads.Add(Road);});
    if(Chart.Span<105000) {
        const int SX=FMath::FloorToInt32(Box.Min.X/AETWorld::RegionSize),EX=FMath::FloorToInt32(Box.Max.X/AETWorld::RegionSize);
        const int SY=FMath::FloorToInt32(Box.Min.Y/AETWorld::RegionSize),EY=FMath::FloorToInt32(Box.Max.Y/AETWorld::RegionSize);
        for(int Y=SY;Y<=EY;++Y) for(int X=SX;X<=EX;++X) for(int I=0;I<2;++I) {
            const auto House=W.HouseAnchor({X,Y},I); if(W.InCity(House)) continue;
            const FVector2D HP(House); bool InRoute=false;
            for(const auto& Site:Sites) if(Site.InGrounds(HP,700)) {InRoute=true;break;}
            if(!InRoute) for(const auto& Road:Roads)
                if((Road.Highway||Road.Feeder)&&Road.Distance(HP)<Road.Width*.5+750) {InRoute=true;break;}
            if(InRoute) continue;
            const FVector2D C(House); Block(C-FVector2D(615,515),C+FVector2D(615,515),Lot);
        }
    }
    if(Chart.Span<=175000) for(const auto& Site:Sites) {
        const auto Shade=Site.Bug?FLinearColor(.39,.22,.11):FLinearColor(.12,.34,.39);
        Block(Site.Center-FVector2D(1720),Site.Center+FVector2D(1720),Shade);
        for(int Side:{-1,1}) for(int I=0;I<4;++I) {
            const double Y=-5900.+I*3630.;
            for(const FVector2D P:{Site.At(Side*(I==0?6900.:I==3?7150.:8450.),Y),Site.At(Side*(I%2?4050.:3100.),Y+390)})
                Block(P-FVector2D(650),P+FVector2D(650),Shade);
        }
    }
    TArray<TPair<FVector2D,FString>> RoadLabels;
    auto DrawRoad=[&](const ETNav::FRoad& Road) {
        const float Width=FMath::Clamp(float(Road.Width*Scale),Small?1.f:1.4f,float(55*U));
        Stroke(Road.A,Road.B,Road.Highway?FLinearColor(.58,.48,.31):FLinearColor(.35,.39,.318),Width);
        if(Width>5) Stroke(Road.A,Road.B,Road.Highway?FLinearColor(.76,.64,.43):FLinearColor(.50,.53,.445),Width-2);
        if(Small||Chart.Span>135000||RoadLabels.Num()>=8) return;
        if(!Road.Highway&&!Road.Feeder&&FCString::Strcmp(Road.Name,TEXT("Shelter Approach"))&&FCString::Strcmp(Road.Name,TEXT("Market Street"))&&FCString::Strcmp(Road.Name,TEXT("Signal Avenue"))) return;
        auto Start=Road.A,End=Road.B;
        if(!ETNav::Clip(Box,Start,End)||FVector2D::Distance(Start,End)*Scale<125*U) return;
        const auto Label=Chart.Plot(FMath::Lerp(Start,End,Road.Horizontal()?.25:.72))+FVector2D(6*U,4*U);
        if(Label.X+112*U>Screen.Max.X||Label.Y+16*U>Screen.Max.Y||FVector2D::Distance(Label,Chart.Plot(City))<70*U) return;
        for(const auto& Old:RoadLabels) if(Old.Value==Road.Name||FVector2D::Distance(Old.Key,Label)<105*U) return;
        RoadLabels.Emplace(Label,FString(Road.Name));
    };
    for(const auto& Road:Roads) DrawRoad(Road);
    if(W.RingTrain) {
        static const TArray<FVector2D> Circle=[]{
            TArray<FVector2D> Path;
            for(int I=0;I<=512;++I) Path.Add(ETRing::Position(I*UE_TWO_PI/512.));
            return Path;
        }();
        const FLinearColor Lines[]={{.27,.91,.80},{.58,.74,1.},{1.,.69,.26},{1.,.48,.31}};
        const FBox2D Inset(Screen.Min+FVector2D(10*U),Screen.Max-FVector2D(10*U));
        for(const auto* ServiceActor:W.RailLines) {
            const int Service=ServiceActor->Service;
            const auto Rail=Lines[Service];
            if(ETRing::Shuttle(Service)) {
                Stroke(ETRing::Dock(Service,0),ETRing::Dock(Service,1),Rail,Small?2.f:3.f);
                const auto Mid=Chart.Plot(ETRing::Track(Service,.5));
                if(Overview&&Inset.IsInsideOrOn(Mid)) TickText(*this,ETRing::ServiceName(Service),Mid-FVector2D(0,19*U),Rail,.93*U);
                for(int End:{0,1}) {
                    const auto A=ETRing::Dock(Service,End),B=ETRing::CityStation(ETRing::CityAt(Service,End));
                    for(int Dash=0;Dash<8;Dash+=2) Stroke(FMath::Lerp(A,B,Dash/8.),FMath::Lerp(A,B,(Dash+1)/8.),Rail,1.4f);
                }
            } else {
                const auto Shift=Service==ETRing::West?FVector2D::ZeroVector:ETRing::EastOffset();
                for(int I=1;I<Circle.Num();++I) Stroke(Circle[I-1]+Shift,Circle[I]+Shift,Rail,Small?1.5f:2.f);
            }
            const auto Train=Chart.Plot(ETRing::Track(Service,ServiceActor->Phase.Angle));
            if(Inset.IsInsideOrOn(Train)) Arrow(*this,Train,ServiceActor->TravelDirection(),8*U,Rail);
            for(int I=0;I<ETRing::StopCount(Service);++I) {
                const int District=ETRing::CityAt(Service,I);
                const auto Pixel=Chart.Plot(ETRing::Dock(Service,I));
                if(!Inset.IsInsideOrOn(Pixel)) continue;
                const double R=4*U;
                DrawRect(Panel,Pixel.X-R-1,Pixel.Y-R-1,2*R+2,2*R+2);
                DrawRect(Rail,Pixel.X-R,Pixel.Y-R,2*R,2*R);
                if(Chart.Span<=60000) {
                    const auto Lift=Chart.Plot(ETRing::LiftPoint(Service,I));
                    if(Inset.IsInsideOrOn(Lift)) {
                        Line(*this,Lift-FVector2D(0,4*U),Lift+FVector2D(0,4*U),Rail,2*U);
                        Line(*this,Lift-FVector2D(3*U,0),Lift-FVector2D(0,4*U),Rail,2*U);
                        if(!Small) Text(TEXT("Street lift"),Lift.X+6*U,Lift.Y,Rail,.8*U);
                    }
                }
                if(!Small&&Pixel.X+105*U<Screen.Max.X&&Pixel.Y+24*U<Screen.Max.Y&&(!ETRing::Shuttle(Service)||Chart.Span<120000))
                    Text(ETRing::Shuttle(Service)?FString(ETRing::ServiceName(Service)):FString(ETRing::CityName(District)),Pixel.X+7*U,Pixel.Y+6*U,Rail,.82*U);
            }
        }
        if(Chart.Span<=60000) for(int K=0;K<2*ETRing::Stops;++K) for(int J=0;J<2;++J) {
            const auto Pxl=Chart.Plot(ETRailDistrict::Lot(K,J));
            if(!Inset.IsInsideOrOn(Pxl)) continue;
            const auto& Quarter=ETRailDistrict::Get(K);
            const bool Taken=S.Consumed.Contains(ETRailDistrict::CacheId(K,J));
            const auto Color=Taken?Sage:Gold;
            DrawRect(Panel,Pxl.X-4*U,Pxl.Y-4*U,8*U,8*U);
            Line(*this,Pxl-FVector2D(3*U,0),Pxl+FVector2D(3*U,0),Color,2*U);
            if(!Taken) Line(*this,Pxl-FVector2D(0,3*U),Pxl+FVector2D(0,3*U),Color,2*U);
            if(!Small) {
                const FString Label=FString(Quarter.Places[J].Name)+(Taken?TEXT(" (searched)"):TEXT(""));
                float TW,TH;GetTextSize(Label,TW,TH,GEngine->GetSmallFont(),.85*U);
                const double X=FMath::Clamp(Pxl.X+(J?7*U:-TW-7*U),Inset.Min.X,FMath::Max(Inset.Min.X,Inset.Max.X-TW));
                const double Y=FMath::Clamp(Pxl.Y-TH-3*U,Inset.Min.Y,Inset.Max.Y-TH);
                DrawRect(Panel,X-1,Y,TW+3,TH);Text(Label,X,Y,Color,.85*U);
            }
        }
    }
    for(const auto& Label:RoadLabels) Text(Label.Value,Label.Key.X,Label.Key.Y,Sage,.82*U);
    // Clip first, so a waypoint on the other side of the world can never create an unbounded dotted line.
    FVector2D A=Here,B=Tracked;
    if(ETNav::Clip(Box,A,B)) {
        A=Chart.Plot(A); B=Chart.Plot(B); const auto D=(B-A).GetSafeNormal(); const double Length=FVector2D::Distance(A,B);
        for(double T=0;T<Length;T+=12*U) Line(*this,A+D*T,A+D*FMath::Min(T+5*U,Length),S.TrackedPin==INDEX_NONE?Gold:Blue,1.5);
    }
    auto Marker=[&](FVector2D Place,bool IsCity,bool Active,const FString& Label) {
        const FVector2D Raw=Chart.Plot(Place),Middle=Chart.Position+Chart.Size*.5;
        const FBox2D Inset(Screen.Min+FVector2D(12*U),Screen.Max-FVector2D(12*U));
        auto Pixel=Raw;
        if(!Inset.IsInsideOrOn(Pixel)) {
            if(!Active||(!Small&&!Inset.IsInsideOrOn(Chart.Plot(Here)))) return;
            const auto Origin=Small?Middle:Chart.Plot(Here);
            auto Start=Origin; if(!ETNav::Clip(Inset,Start,Pixel)) return;
            const auto D=(Raw-Origin).GetSafeNormal(); Arrow(*this,Pixel-D*3*U,D,5*U,IsCity?Gold:Blue); Pixel-=D*10*U;
        }
        Pin(*this,Pixel,(Active?7:5)*U,IsCity,IsCity?Gold:Blue);
        if(!Small&&!Label.IsEmpty()) {
            float TW,TH; const float Font=1.f*U; GetTextSize(Label,TW,TH,GEngine->GetSmallFont(),Font);
            const FVector2D At(FMath::Clamp(Pixel.X+11*U,Screen.Min.X+3,Screen.Max.X-TW-8),FMath::Clamp(Pixel.Y-TH-7*U,Screen.Min.Y+3,Screen.Max.Y-TH-3));
            DrawRect(Panel,At.X-3,At.Y-1,TW+6,TH+2); Text(Label,At.X,At.Y,IsCity?Gold:Blue,Font);
        }
    };
    for(const auto& Site:Sites) {
        const auto Pixel=Chart.Plot(Site.Center);
        if(!FBox2D(Screen.Min+FVector2D(9*U),Screen.Max-FVector2D(9*U)).IsInsideOrOn(Pixel)) continue;
        const auto Color=Site.Bug?FLinearColor(1,.51,.19):FLinearColor(.35,.78,1.);
        const double R=(Small?5:8)*U; const int N=Site.Bug?6:4;
        for(int I=0;I<N;++I) {
            auto V=[&](int J){const double A=2*PI*J/N-PI*.5;return Pixel+FVector2D(FMath::Cos(A),FMath::Sin(A))*R;};
            Line(*this,V(I),V(I+1),Color,2);
        }
        Line(*this,Pixel-FVector2D(0,R*.5),Pixel+FVector2D(0,R*.5),Color,2);
        if(!Small) {
            const FString Label=FString(Site.Bug?TEXT("Bug / "):TEXT("Supertech / "))+Site.Name();
            float TW,TH; GetTextSize(Label,TW,TH,GEngine->GetSmallFont(),U);
            const auto LX=FMath::Clamp(Pixel.X+11*U,Screen.Min.X+3,Screen.Max.X-TW-4);
            const double LY=FMath::Clamp(Pixel.Y-TH-3,Screen.Min.Y+3,Screen.Max.Y-TH-4);
            DrawRect(Panel,LX-2,LY,TW+4,TH+2); Text(Label,LX,LY+1,Color,U);
            if(Chart.Span<200000&&LY+2*TH<Screen.Max.Y) {
                const FString Intel=ETWar::BugAdvance(Site,S.Elapsed)?TEXT("Bug advance"):TEXT("Supertech advance");
                GetTextSize(Intel,TW,TH,GEngine->GetSmallFont(),U*.82);
                DrawRect(Panel,LX-2,LY+14*U,TW+4,TH+2); Text(Intel,LX,LY+14*U,Sage,U*.82);
            }
        }
    }
    if(!Small&&!Overview) for(int Y=From.Y;Y<=To.Y;++Y) for(int X=From.X;X<=To.X;++X) {
        const FIntPoint Cell(X,Y); if(Cell!=Home) Marker(ETNav::Refuge(Cell),true,false,P->World->TownStatus(Cell));
    }
    if(!Overview) Marker(City,true,true,Small?FString(ETNav::RefugeName(Home)):P->World->TownStatus(Home));
    for(int I=0;I<S.MapPins.Num();++I) Marker(S.MapPins[I],false,I==S.TrackedPin,ETRailDistrict::PinLabel(S.MapPins[I],I));
    const auto PlayerPos=Chart.Plot(Here);
    if(FBox2D(Screen.Min+FVector2D(9*U),Screen.Max-FVector2D(9*U)).IsInsideOrOn(PlayerPos))
        Arrow(*this,PlayerPos,FVector2D(FRotator(0,P->GetControlRotation().Yaw,0).Vector()),Small?8*U:10*U,Ink);
}

void AETHUD::DrawNavigation(AETCharacter* P)
{
    const float SW=Canvas->SizeX,SH=Canvas->SizeY;
    const float U=FMath::Clamp(FMath::Min(SW/1440.f,SH/850.f),.7f,1.4f);
    const float W=SW>=1440?FMath::Clamp(SW-990.f,290.f,505.f*U):FMath::Clamp(SW*.4f,248.f,430.f*U);
    const float Y=SW>=1440?18:139,Mid=SW*.5f,Left=Mid-W*.5f;
    const FVector2D Here(P->GetActorLocation()); const auto& S=*P->World->State;
    const auto Cell=ETNav::District(Here); const auto City=ETNav::Refuge(Cell);
    const double Heading=FMath::Fmod(FRotator::ClampAxis(P->GetControlRotation().Yaw+90),360.);
    DrawRect(Panel,Left,Y,W,69*U);
    for(int Degree=0;Degree<360;Degree+=15) {
        const double Offset=FMath::FindDeltaAngleDegrees(Heading,double(Degree));
        if(FMath::Abs(Offset)>58) continue;
        const float X=Mid+Offset*W/124.;
        DrawLine(X,Y+22*U,X,Y+(Degree%45==0?31:26)*U,Sage,1);
        if(Degree%45==0) TickText(*this,Winds[Degree/45],{X,Y+2*U},Ink,1.02*U);
    }
    DrawLine(Mid,Y+24*U,Mid,Y+35*U,Ink,2.4);
    auto BearingMark=[&](FVector2D T,bool IsCity) {
        const auto D=T-Here; if(D.Size()<225) return;
        const double Offset=FMath::FindDeltaAngleDegrees(Heading,ETNav::Bearing(D));
        const float X=Mid+FMath::Clamp(Offset,-58.,58.)*W/124.;
        const FLinearColor Color=IsCity?Gold:Blue;
        Pin(*this,{X,Y+36*U},4.5*U,IsCity,Color);
        if(FMath::Abs(Offset)>58) {
            const float Dir=Offset<0?-1:1;
            DrawLine(X+Dir*5*U,Y+36*U,X+Dir*9*U,Y+36*U,Color,1.5);
            DrawLine(X+Dir*7*U,Y+34*U,X+Dir*9*U,Y+36*U,Color,1.5);
        }
    };
    BearingMark(City,true); if(S.MapPins.IsValidIndex(S.TrackedPin)) BearingMark(S.MapPins[S.TrackedPin],false);
    const FString Reading=FString::Printf(TEXT("%03d°   %s"),FMath::RoundToInt(Heading)%360,Winds[FMath::RoundToInt(Heading/45.)%8]);
    Text(Reading,Left+8*U,Y+47*U,Ink,.90*U);
    FString Travel=FVector2D::Distance(Here,City)<300?FString(TEXT("At "))+ETNav::RefugeName(Cell):FString(TEXT("City  "))+ETNav::Distance(FVector2D::Distance(Here,City));
    if(S.MapPins.IsValidIndex(S.TrackedPin)) Travel+=FString::Printf(TEXT("    Pin %d  %s"),S.TrackedPin+1,*ETNav::Distance(FVector2D::Distance(Here,S.MapPins[S.TrackedPin])));
    Text(Travel,Left+95*U,Y+47*U,S.MapPins.IsValidIndex(S.TrackedPin)?Blue:Gold,.90*U);
    const double Side=FMath::Clamp(FMath::Min(SW*.16f,SH*.23f),118.f,241.f);
    const FVector2D Top(SW-Side-19,135);
    FillBox(*this,FBox2D(Top-FVector2D(3,3),Top+FVector2D(Side+3,Side+23*U)),Panel);
    const ETNav::FChart Mini{Top,FVector2D(Side),Here,26500.}; DrawChart(P,Mini,true);
    Text(TEXT("N"),Top.X+5,Top.Y+2,Ink,.9*U);
    FString Nearby=TEXT("M   Field atlas");
    if(auto* Region=P->World->Regions.FindRef({FMath::FloorToInt32(Here.X/AETWorld::RegionSize),FMath::FloorToInt32(Here.Y/AETWorld::RegionSize)})) {
        double Nearest=800;
        for(const auto& Road:Region->LocalRoads) {
            const double D=Road.Distance(Here);
            if(D<Road.Width*.5+135&&D<Nearest) {Nearby=FString(TEXT("M   "))+Road.Name;Nearest=D;}
        }
    }
    Text(Nearby,Top.X+3,Top.Y+Side+3,Sage,.85*U);
}
void AETHUD::DrawAtlas(AETCharacter* P)
{
    const FVector2D View(Canvas->SizeX,Canvas->SizeY); const auto Chart=ETNav::Atlas(View,P->MapCenter,P->MapSpan);
    const float U=Chart.Position.Y/64.;
    const auto& S=*P->World->State; const FVector2D Here(P->GetActorLocation()); const auto Cell=ETNav::District(Here);
    const bool Pinned=S.MapPins.IsValidIndex(S.TrackedPin);
    const auto Dest=Pinned?S.MapPins[S.TrackedPin]:ETNav::Refuge(Cell);
    const FString Route=Pinned?ETRailDistrict::PinLabel(S.MapPins[S.TrackedPin],S.TrackedPin):ETNav::RefugeName(Cell);
    DrawRect(FLinearColor(.015,.027,.023,.985),0,0,View.X,View.Y);
    const float X=Chart.Position.X;
    Text(TEXT("FIELD ATLAS"),X,16*U,Ink,1.9*U);
    const FString Header=FString::Printf(TEXT("%s   /   %s"),*Route,*ETNav::Distance(FVector2D::Distance(Here,Dest)));
    float TW,TH; GetTextSize(Header,TW,TH,GEngine->GetSmallFont(),1.22*U);
    Text(Header,View.X-X-TW,25*U,Pinned?Blue:Gold,1.22*U);
    FillBox(*this,FBox2D(Chart.Position-FVector2D(2),Chart.Position+Chart.Size+FVector2D(2)),Sage);
    DrawChart(P,Chart,false);
    const auto End=Chart.Position+Chart.Size;
    DrawRect(Panel,End.X-45*U,Chart.Position.Y+8*U,35*U,52*U);
    TickText(*this,TEXT("N"),{End.X-28*U,Chart.Position.Y+9*U},Ink,1.1*U);
    Arrow(*this,{End.X-28*U,Chart.Position.Y+43*U},{0,-1},8*U,Ink);
    const double Want=Chart.Span*.18,Power=FMath::Pow(10.,FMath::FloorToDouble(FMath::LogX(10.,Want)));
    const double Length=(Want/Power>=5?5.:Want/Power>=2?2.:1.)*Power,Pixels=Length*Chart.Scale();
    const FVector2D Start=Chart.Position+FVector2D(14*U,Chart.Size.Y-11*U);
    DrawRect(Panel,Start.X-5*U,Start.Y-22*U,Pixels+10*U,28*U);
    Line(*this,Start,Start+FVector2D(Pixels,0),Ink,2);
    for(double Offset:{0.,Pixels}) Line(*this,Start+FVector2D(Offset,0),Start+FVector2D(Offset,-5*U),Ink,1);
    Text(ETNav::Distance(Length),Start.X,Start.Y-22*U,Sage,.85*U);
    // Street names are read from the same road segment the cursor is pointing at.
    if(auto* PC=Cast<APlayerController>(P->Controller)) {
        float MX,MY; if(PC->GetMousePosition(MX,MY)&&Chart.Inside({MX,MY})) {
            const auto Cursor=Chart.WorldAt({MX,MY}); const double Tolerance=FMath::Clamp(8./Chart.Scale(),500.,1800.);
            const TCHAR* Name=nullptr; double Best=Tolerance;
            ETNav::Roads(FBox2D(Cursor-FVector2D(Tolerance),Cursor+FVector2D(Tolerance)),[&](const ETNav::FRoad& R){
                const double D=R.Distance(Cursor); if(D<Best){Name=R.Name;Best=D;}
            });
            if(Name) {
                GetTextSize(Name,TW,TH,GEngine->GetSmallFont(),U);
                DrawRect(Panel,End.X-TW-17*U,End.Y-TH-10*U,TW+12*U,TH+6*U);
                Text(Name,End.X-TW-11*U,End.Y-TH-6*U,Ink,U);
            }
        }
    }
    const TCHAR* Labels[]={TEXT("−  Zoom out"),TEXT("+  Zoom in"),TEXT("Home  /  Player"),TEXT("C  /  Nearest city"),TEXT("Del  /  Remove pin"),TEXT("M  /  Back")};
    for(int I=0;I<6;++I) {
        const auto B=ETNav::AtlasButton(Chart,I);
        FillBox(*this,B,FLinearColor(.10,.15,.119));
        TickText(*this,Labels[I],{B.GetCenter().X,B.Min.Y+6*U},I==4&&!Pinned?Sage:Ink,1.02*U);
    }
    Text(P->MapMessage.IsEmpty()?TEXT("R  Rail network    ·    L  Local stops & loot    ·    Click to track    ·    Drag to pan    ·    Scroll to zoom"):P->MapMessage,
        X,End.Y+56*U,Ink,.99*U);
    const FString Legend=TEXT("City / amber     Pin / blue     Rail / teal"); GetTextSize(Legend,TW,TH,GEngine->GetSmallFont(),.92*U);
    if(View.X>1050*U) Text(Legend,End.X-TW,End.Y+57*U,Sage,.92*U);
}
