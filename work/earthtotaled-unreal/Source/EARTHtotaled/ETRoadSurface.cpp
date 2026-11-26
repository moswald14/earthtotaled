#include "ETGame.h"
#include "ETGeology.h"
#include "ProceduralMeshComponent.h"

bool AETRegion::OnHighway(FVector2D Position,double Margin) const
{
    for(const auto& Road:LocalRoads)
        if((Road.Highway||Road.Feeder)&&Road.Distance(Position)<Road.Width*.5+Margin) return true;
    return false;
}

void AETRegion::BuildRoadSurface()
{
    if(LocalRoads.IsEmpty()||!TerrainGrid) return;
    const auto* Land=Terrain->GetProcMeshSection(0);
    if(!Land||Land->ProcVertexBuffer.Num()!=(TerrainGrid+1)*(TerrainGrid+1)) return;
    struct FSection {TArray<FVector> Vertices,Normals; TArray<int32> Indices; TArray<FVector2D> UV;};
    FSection Sections[4];
    const FVector2D Origin(GetActorLocation()),Corner=Origin+FVector2D(AETWorld::RegionSize);
    const double Step=AETWorld::RegionSize/TerrainGrid;
    auto Height=[&](FVector2D P) {
        const auto UV=(P-Origin)/Step;
        const int X=FMath::Clamp(FMath::FloorToInt(UV.X),0,TerrainGrid-1),Y=FMath::Clamp(FMath::FloorToInt(UV.Y),0,TerrainGrid-1);
        const double U=UV.X-X,V=UV.Y-Y;
        const int A=Y*(TerrainGrid+1)+X,B=A+1,C=A+TerrainGrid+1,E=C+1;
        const auto& Vert=Land->ProcVertexBuffer;
        if(U+V<=1) return Vert[A].Position.Z*(1-U-V)+Vert[B].Position.Z*U+Vert[C].Position.Z*V;
        return Vert[E].Position.Z*(U+V-1)+Vert[C].Position.Z*(1-U)+Vert[B].Position.Z*(1-V);
    };
    // Clip paint, shoulders and asphalt against the same surface openings as destructible city paving.
    auto Triangle=[&](FSection& S,FVector A,FVector B,FVector C,double Da,double Db,double Dc) {
        const FVector P[]={A,B,C}; const double D[]={Da,Db,Dc}; const int Start=S.Vertices.Num();
        const FVector Normal=FVector::CrossProduct(B-A,C-A).GetSafeNormal();
        if(FMath::IsNearlyZero(Normal.Z)) return;
        auto Vertex=[&](FVector V){S.Vertices.Add(V-FVector(Origin,0)); S.Normals.Add(Normal.Z<0?-Normal:Normal); S.UV.Add(FVector2D(V)/400.);};
        for(int I=0;I<3;++I) {
            const int J=(I+1)%3;
            if(D[I]>=0) Vertex(P[I]);
            if((D[I]>=0)!=(D[J]>=0)) Vertex(FMath::Lerp(P[I],P[J],D[I]/(D[I]-D[J])));
        }
        for(int I=Start+2;I<S.Vertices.Num();++I) {
            if(Normal.Z<0) S.Indices.Append({Start,I-1,I}); else S.Indices.Append({Start,I,I-1});
        }
    };
    auto Quad=[&](int Slot,FVector2D A,FVector2D B,FVector2D C,FVector2D E,double Lift,TOptional<double> Flat={}) {
        FVector2D Points[]={A,B,C,E}; FVector V[4]; double D[4];
        for(int I=0;I<4;++I) {
            auto& P=Points[I]; P.X=FMath::Clamp(P.X,Origin.X,Corner.X); P.Y=FMath::Clamp(P.Y,Origin.Y,Corner.Y);
            V[I]=FVector(P,(Flat.IsSet()?Flat.GetValue():Height(P))+Lift);
            D[I]=World->Geology->SurfaceOpening(P.X,P.Y)-14;
        }
        Triangle(Sections[Slot],V[0],V[1],V[2],D[0],D[1],D[2]);
        Triangle(Sections[Slot],V[2],V[1],V[3],D[2],D[1],D[3]);
    };
    for(const auto& Road:LocalRoads) {
        if(!Road.Highway&&!Road.Feeder) continue;
        const bool Horizontal=Road.Horizontal(); const int Axis=Horizontal?0:1;
        const FVector2D Delta=Road.B-Road.A;
        const double Start=FMath::Max(Origin[Axis],FMath::Min(Road.A[Axis],Road.B[Axis])),End=FMath::Min(Corner[Axis],FMath::Max(Road.A[Axis],Road.B[Axis]));
        auto Point=[&](double Along,double Across){return Road.A+Delta*((Along-Road.A[Axis])/Delta[Axis])+(Horizontal?FVector2D(0,Across):FVector2D(Across,0));};
        for(double T=Start;T<End;) {
            const int Cell=FMath::FloorToInt(T/200.);
            const double Next=FMath::Min(End,(Cell+1)*200.);
            const auto Mid=Point((T+Next)*.5,0);
            const auto District=ETNav::District(Mid);
            const double Run=FMath::Abs(Mid[Axis]-FVector2D(District)[Axis]*ETNav::DistrictSpacing);
            const double Edge=District==FIntPoint::ZeroValue?16000.:(Horizontal?22400.:22000.);
            const double Half=Road.Highway?FMath::Lerp(395.,930.,FMath::SmoothStep(0.,5200.,Run-Edge)):Road.Width*.5;
            const double Edges[]={-Half,-Half*.89,-Half*.46,0.,Half*.46,Half*.89,Half};
            for(int L=0;L<6;++L) {
                const double A=Edges[L],B=Edges[L+1];
                Quad(L==0||L==5?0:1,Point(T,A),Point(Next,A),Point(T,B),Point(Next,B),5);
            }
            auto Stripe=[&](int Slot,double Offset,double Width) {
                Quad(Slot,Point(T,Offset-Width),Point(Next,Offset-Width),Point(T,Offset+Width),Point(Next,Offset+Width),8);
            };
            if(Road.Highway) {
                for(int Side:{-1,1}) {
                    Stripe(3,Side*18.,6); Stripe(2,Side*Half*.865,6);
                    if(Half>620&&(Cell%6+6)%6<2) Stripe(2,Side*Half*.45,7);
                }
            } else if((Cell%7+7)%7<2) Stripe(3,0,6);
            T=Next;
        }
    }
    // Lay curb ramps and pavements on both sides, leaving the junction itself open.
    for(const auto& Street:LocalRoads) {
        if(Street.Highway||Street.Feeder) continue;
        const bool East=Street.Horizontal(); const int Axis=East?0:1;
        const double Start=FMath::Max(Origin[Axis],Street.A[Axis]),End=FMath::Min(Corner[Axis],Street.B[Axis]);
        auto Point=[&](double T,double Cross) {
            return East?FVector2D(T,Street.A.Y+Cross):FVector2D(Street.A.X+Cross,T);
        };
        const double Half=Street.Width*.5,Z=World->CityCenter(Street.A.X,Street.A.Y).Z;
        TArray<double,TInlineAllocator<8>> Crossings;
        for(const auto& Other:LocalRoads) if(!Other.Highway&&!Other.Feeder&&Other.Horizontal()!=East) Crossings.Add(Other.A[Axis]);
        for(double T=Start;T<End;) {
            const int Slab=FMath::FloorToInt(T/160.); const double Next=FMath::Min(End,(Slab+1)*160.);
            double FromCorner=10000;
            for(double At:Crossings) FromCorner=FMath::Min(FromCorner,FMath::Abs(At-(T+Next)*.5));
            if(FromCorner>Half+125) {
                for(int Side:{-1,1}) {
                    const double In=Side*(Half+10),Out=Side*(Half+208);
                    const double Lift=FMath::Lerp(12.,22.,FMath::Clamp((FromCorner-Half-160)/220.,0.,1.));
                    Quad(0,Point(T,In),Point(Next-1,In),Point(T,Out),Point(Next-1,Out),Lift,Z);
                    if((Slab%13+13)%13==3&&FromCorner>1050) {
                        Quad(2,Point(T,Side*(Half-85)),Point(T+9,Side*(Half-85)),Point(T,Side*(Half-20)),Point(T+9,Side*(Half-20)),14,Z);
                    }
                }
                if(FromCorner>1200&&(Slab%7+7)%7<2) Quad(3,Point(T,-6),Point(Next,-6),Point(T,6),Point(Next,6),14,Z);
            }
            T=Next;
        }
    }
    // Paint the crossings and stop lines on the four approaches.
    for(const auto& Row:LocalRoads) if(!Row.Highway&&!Row.Feeder&&Row.Horizontal()) {
        for(const auto& Col:LocalRoads) if(!Col.Highway&&!Col.Feeder&&!Col.Horizontal()) {
            const FVector2D Center(Col.A.X,Row.A.Y);
            if(Row.Distance(Center)>1||Col.Distance(Center)>1) continue;
            const double Z=World->CityCenter(Center.X,Center.Y).Z;
            for(int Side:{-1,1}) for(int Axis=0;Axis<2;++Axis) for(int Stripe=-2;Stripe<=2;++Stripe) {
                const double Edge=Side*((Axis?Row:Col).Width*.5+165);
                const FVector2D Pos=Center+(Axis?FVector2D(Stripe*95,Edge):FVector2D(Edge,Stripe*95));
                const FVector2D Half=Axis?FVector2D(22,105):FVector2D(105,22);
                if(Pos.X+Half.X<=Origin.X||Pos.Y+Half.Y<=Origin.Y||Pos.X-Half.X>=Corner.X||Pos.Y-Half.Y>=Corner.Y) continue;
                Quad(2,Pos-Half,Pos+FVector2D(Half.X,-Half.Y),Pos+FVector2D(-Half.X,Half.Y),Pos+Half,15,Z);
                if(Stripe==0) {
                    const FVector2D Bar=Pos+(Axis?FVector2D(-Side*175,Side*160):FVector2D(Side*160,Side*175));
                    const FVector2D H=Axis?FVector2D(145,12):FVector2D(12,145);
                    Quad(2,Bar-H,Bar+FVector2D(H.X,-H.Y),Bar+FVector2D(-H.X,H.Y),Bar+H,15,Z);
                }
            }
        }
    }
    if(!RoadMesh) {
        if(Sections[0].Indices.IsEmpty()&&Sections[2].Indices.IsEmpty()) return;
        RoadMesh=NewObject<UProceduralMeshComponent>(this); RoadMesh->SetupAttachment(Terrain);
        RoadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); RoadMesh->SetCastShadow(false);
        RoadMesh->bAffectDistanceFieldLighting=false; RoadMesh->RegisterComponent();
        const TCHAR* Materials[]={TEXT("Concrete"),TEXT("Asphalt"),TEXT("Plaster"),TEXT("Canvas")};
        for(int I=0;I<4;++I) RoadMesh->SetMaterial(I,World->Material(Materials[I]));
    }
    for(int I=0;I<4;++I) {
        auto& S=Sections[I];
        if(S.Indices.Num()) RoadMesh->CreateMeshSection_LinearColor(I,S.Vertices,S.Indices,S.Normals,S.UV,{}, {},false);
        else RoadMesh->ClearMeshSection(I);
    }
}
