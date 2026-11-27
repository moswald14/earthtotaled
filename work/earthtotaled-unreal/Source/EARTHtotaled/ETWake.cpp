#include "ETWake.h"
#include "ETGeology.h"
#include "ETGame.h"
#include "ETWakeVehicle.h"

using namespace ETWake;

namespace
{
    const FSite Sites[]={
        {TEXT("Shelter crossing"),{5400,2600}}, {TEXT("Foxglove Cut"),{18500,4600}},
        {TEXT("Chalk Beck"),{23100,-6100}}, {TEXT("Widow Orchard"),{30000,6600}},
        {TEXT("Split Alder"),{35600,-5200}}, {TEXT("Lichen Bank"),{53400,16700}},
        {TEXT("Radio Fen"),{64800,7400}}, {TEXT("Glass Meadow"),{72100,22700}},
        {TEXT("Ash Orchard"),{81700,13800}}};
    bool Nearby(FVector2D P,FVector2D Q,double R) {return FVector2D::DistSquared(P,Q)<R*R;}
}
const FSite& ETWake::Herd(int I) {return Sites[FMath::Clamp(I,0,Herds)];}
FVector2D ETWake::Glade(int I) {return Herd(I).Center+FVector2D(1220,1050);}
FVector2D ETWake::Field(int I,const FETWakeSave& S) {
    const auto Base=I==0?FVector2D(45200,9200):FVector2D(177000,28500);
    return Base+FVector2D(0,16000.*FMath::Max(0,I==0?S.FieldSurvey.X:S.FieldSurvey.Y));
}
void ETWake::SurveyFields(AETWorld& W) {
    auto& S=W.State->Wake;if(S.FieldSurvey.X>=0&&S.FieldSurvey.Y>=0)return;
    S.FieldSurvey=FIntPoint::ZeroValue;if(!W.HadSave||S.Chapter!=Missing)return;
    // Choose locations before grading the runways. Old foundations, excavations and saved arrival points keep their original terrain.
    for(int I=0;I<2;++I){
        const FVector2D Base=Field(I,S);TSet<int> Occupied;
        auto Preserve=[&](FVector P){
            if(FMath::Abs(P.X-Base.X)>8400)return;
            const int Row=FMath::RoundToInt32((P.Y-Base.Y)/16000.);
            if(Row>=0&&FMath::Abs(P.Y-Base.Y-Row*16000.)<7600.)Occupied.Add(Row);
        };
        Preserve(W.State->Position);Preserve(W.State->WinchAnchor);
        for(const auto& It:W.State->Builds)Preserve(It.Value.Anchor);
        for(auto Cut:W.State->BoreCuts)Preserve(FVector(Cut)*UETGeology::CutQuantum);
        for(auto Cut:W.State->SquareCuts)Preserve(FVector(Cut)*ETRock::Step);
        for(auto Vein:W.State->MinedVeins)Preserve(FVector((Vein.X+.5)*1150.,(Vein.Y+.5)*1150.,0));
        auto& Row=I==0?S.FieldSurvey.X:S.FieldSurvey.Y;
        while(Occupied.Contains(Row)||(Row>0&&W.InCity(FVector(Base+FVector2D(0,Row*16000.),0))))++Row;
    }
}
FVector2D ETWake::Location(const FETWakeSave& S,FName N,int I)
{
    if(N=="thrum") return {1100,860};
    if(N=="band") return {3570,2550};
    if(N=="source"||N=="herd") return Herd(I).Center+FVector2D(-200,-170);
    if(N=="glade") return Glade(I);
    if(N=="tracks")return Herd(0).Center+FVector2D(105,190);
    if(N=="fernfold")return Glade(0)+FVector2D(-260,-100);
    if(N=="paws")return Field(0,S)+FVector2D(-3930,-1150);
    if(N=="liftfold")return Location(S,"cell")+FVector2D(105,-190);
    if(N=="garage") return {5650,2210};
    if(N=="roadgate") return {9200,2600};
    if(N=="dispatch") return {13700,2410};
    if(N=="runway") return Field(0,S)+FVector2D(-3700,-810);
    if(N=="hangar"||N=="field0") return Field(0,S)+FVector2D(-3500,-460);
    if(N=="land"||N=="field1") return Field(1,S)+FVector2D(-3400,-460);
    const auto Base=Field(1,S)+FVector2D(-2200,-2850);
    if(N=="vault") return Base;
    if(N=="juice") return Base+FVector2D(210,-165);
    if(N=="frame") return Base+FVector2D(210,165);
    if(N=="burrow") return Base+FVector2D(2200,-740);
    if(N=="gate") return Base+FVector2D(2240,700);
    if(N=="cell") return Base+FVector2D(3370,0);
    if(N=="evac") return Base+FVector2D(4950,1100);
    if(N=="strike") return Base+FVector2D(5080,1330);
    if(N=="home") return Glade(0);
    return Location(S,"thrum");
}
int ETWake::StripAt(const FETWakeSave& S,FVector2D P) {
    for(int I=0;I<2;++I) {auto D=(P-Field(I,S)).GetAbs();if(D.X<=5900&&D.Y<=680)return I;}return INDEX_NONE;
}
double ETWake::StripZ(const AETWorld& W,int I) {auto P=Field(I,W.State->Wake);return W.Height(P.X,P.Y)+9;}
bool ETWake::Paved(const FETWakeSave& S,FVector2D P) {
    for(int I=0;I<2;++I){
        const auto From=P-Field(I,S);const auto Runway=From.GetAbs();
        if(Runway.X<6200&&Runway.Y<970)return true;
        const auto Shed=(From-FVector2D(-3600,-1500)).GetAbs();if(Shed.X<750&&Shed.Y<560)return true;
    }
    const auto Vault=(P-Location(S,"vault")).GetAbs(),Cell=(P-Location(S,"cell")).GetAbs();
    return (Vault.X<750&&Vault.Y<610)||(Cell.X<610&&Cell.Y<580);
}
bool ETWake::Reserved(const FETWakeSave& S,FVector2D P,double Extra)
{
    for(int I=0;I<2;++I) {auto D=(P-Field(I,S)).GetAbs();if(D.X<6400+Extra&&D.Y<1250+Extra)return true;}
    if(Nearby(P,Location(S,"cell"),3400+Extra)||Nearby(P,Location(S,"vault"),1200+Extra)) return true;
    for(int I=0;I<=Herds;++I) if(Nearby(P,Herd(I).Center+FVector2D(560,510),1530+Extra))return true;
    return false;
}
bool ETWake::Refuge(const FETWakeSave& S,FVector2D P)
{
    if(S.Chapter>=FirstHerd&&S.Chapter<=Dispatch&&P.X>2500&&P.X<15500&&P.Y>1400&&P.Y<5200)return true;
    if(S.Chapter>=Runway&&S.Chapter<=Flight&&(StripAt(S,P)>=0||Nearby(P,Location(S,"hangar"),2100)))return true;
    if(S.Chapter>=Vault&&Nearby(P,Location(S,"cell"),4800))return true;
    for(int I=0;I<=Herds;++I) {
        bool Restored=I?((S.Silenced|S.Retuned)&(1<<(I-1)))!=0:S.Chapter>=Garage;
        if(Restored&&Nearby(P,Glade(I),1900))return true;
    }
    return false;
}
int ETWake::NextHerd(const FETWakeSave& S) {
    for(int I=1;I<=Herds;++I) if(!((S.Silenced|S.Retuned)&(1<<(I-1))))return I;
    return 0;
}
const TCHAR* ETWake::Frequency(int I) {static const TCHAR* Band[]={TEXT("Amber"),TEXT("Violet"),TEXT("Cyan")};return Band[(I*5)%3];}
FName ETWake::Target(const FETWakeSave& S)
{
    if(S.TaskSide) return S.ReadSource?"glade":"source";
    switch(S.Chapter) {
        case Missing:return "thrum";case Triage:return "band";case FirstHerd:return S.SharedLead?"glade":"herd";
        case Garage:return "garage";case Driving:return S.Seat!=Car&&!S.DriveGate?"car":S.DriveGate?"dispatch":"roadgate";
        case Dispatch:case Surveys:return "dispatch";case Runway:return "runway";case Hangar:return "hangar";
        case Flight:return S.Seat==Plane||S.TookOff?"land":"plane";case Vault:return "vault";
        case Passage:return S.Body==Bug?"burrow":"gate";case Cell:return "cell";case Escort:return "evac";
        case Strike:return "strike";default:return "home";
    }
}
FString ETWake::Objective(const FETWakeSave& S)
{
    if(S.TaskSide) {
        FString Site=Herd(S.TaskHerd).Name;
        if(!S.ReadSource)return (S.TaskSide==Bug?TEXT("Meet Thrum at "):TEXT("Read the relay at "))+Site;
        return S.TaskSide==Bug?TEXT("Guide the herd to the quiet glade. E to sever."):TEXT("Tune the glade receiver to ")+FString(Frequency(S.TaskHerd));
    }
    switch(S.Chapter) {
        case Missing:return TEXT("Ask Thrum about Kit at Shelter 09");
        case Triage:return TEXT("Search the evacuation lane for Kit's wristband");
        case FirstHerd:return S.SharedLead?TEXT("Lead the deer to the glade. E at the marker."):TEXT("Meet Thrum beside the deer blocking the road");
        case Garage:return TEXT("Meet SALT-7 beside the roadster");
        case Driving:return S.DriveGate?TEXT("Brake at East Dispatch and exit with E"):TEXT("Drive through the amber gate on the shelter road");
        case Dispatch:return TEXT("Read the evacuation manifest at East Dispatch");
        case Surveys:return TEXT("Take a herd job at Dispatch or on the field journal [J]");
        case Runway:return TEXT("Help Thrum clear Hollow Field's runway");
        case Hangar:return TEXT("Help SALT-7 bring the survey plane online");
        case Flight:return TEXT("Fly to Crown Field. Land, brake and exit to find Kit.");
        case Vault:return TEXT("Meet both friends in the shaking MegaWAKE vault");
        case Passage:return S.Body==Bug?TEXT("Sense the root seam and burrow into custody"):TEXT("Authorize the custody gate with your frame");
        case Cell:return TEXT("Open Kit's cell and get him out");
        case Escort:return TEXT("Bring Kit to the evacuation beacon");
        case Strike:return TEXT("Use the Sunseed console after Kit is safe");
        default:return TEXT("Visit Kit and the restored herds at Shelter 09");
    }
}
FVector2D ETWake::Waypoint(const AETWorld& W) {
    const auto& S=W.State->Wake;const auto N=Target(S);
    if(N=="car"&&W.WakeCar)return FVector2D(W.WakeCar->GetActorLocation());
    if(N=="plane"&&W.WakePlane)return FVector2D(W.WakePlane->GetActorLocation());
    return Location(S,N,S.TaskHerd);
}
bool ETWake::BugPack(const AETCharacter& P) {return P.World&&P.World->State&&P.World->State->Wake.Body==Bug;}
bool ETWake::CanBuild(const FETWakeSave& S,int P) {
    if(P<AETConstruction::Nursery)return S.Body!=Bug;
    return P< AETConstruction::Fabricator ? S.Body==Bug : S.Body==Frame;
}
bool FETWakeSave::Valid() const
{
    if(TrailFound<0||TrailFound>=(1<<ClueCount)||TrailHeard<0||(TrailHeard&~TrailFound))return false;
    if(Chapter<Missing||Chapter>Home||Body<Human||Body>Frame||Seat<Foot||Seat>Plane||Airfields<0||Airfields>3||
       Silenced<0||Retuned<0||Silenced>255||Retuned>255||(Silenced&Retuned)||TaskSide<0||TaskSide>Frame||TaskHerd<0||TaskHerd>Herds||
       (TaskSide==0)!=(TaskHerd==0)||WarningSeen<0||WarningSeen>3||VaultLine<0||VaultLine>4||EndingHerds<0||EndingHerds>9)return false;
    if(FieldSurvey.X< -1||FieldSurvey.Y< -1||FieldSurvey.X>125000||FieldSurvey.Y>125000)return false;
    if(EscortPosition.ContainsNaN()||EscortPosition.GetAbsMax()>2.e9)return false;
    if((Body!=Human)!=(Chapter>=Passage)||(Seat==Car&&!HasCar())||(Seat==Plane&&!HasPlane())||
       (Count(Bug)>=4&&Count(Frame)>=4)||(TaskHerd&&((Silenced|Retuned)&(1<<(TaskHerd-1)))))return false;
    for(auto* V:{&Roadster,&Aircraft}) if(V->Position.ContainsNaN()||V->Position.GetAbsMax()>2.e9||!FMath::IsFinite(V->Speed)||
       !FMath::IsFinite(V->Heading)||FMath::Abs(V->Speed)>10000)return false;
    return true;
}
