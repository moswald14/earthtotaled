#include "ETGeology.h"
#include "ETGame.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"

using namespace ETRock;

namespace
{
    struct FSample { FVector Local, Position; double Solid; };
    struct FCorner { FVector Local, Position; };
    struct FSurface
    {
        TArray<FVector> Vertices, Normals;
        TArray<int32> Indices;
        TArray<FVector2D> UV;
    };
    FCorner Crossing(const FSample& A,const FSample& B)
    {
        const double T=A.Solid/(A.Solid-B.Solid);
        return {FMath::Lerp(A.Local,B.Local,T),FMath::Lerp(A.Position,B.Position,T)};
    }
    void Facet(FCorner A,FCorner B,FCorner C,FVector TowardAir,FVector Origin,
        const TArray<FVein>& Veins,FSurface (&Sections)[4])
    {
        FVector Normal=FVector::CrossProduct(B.Position-A.Position,C.Position-A.Position);
        if(!Normal.Normalize()) return;
        if(FVector::DotProduct(Normal,TowardAir)<0) {Swap(B,C); Normal=-Normal;}
        const bool Top=FMath::Abs(A.Local.Z)+FMath::Abs(B.Local.Z)+FMath::Abs(C.Local.Z)<.001;
        int32 Material=Top?0:1;
        if(!Top) {
            const FVector Center=(A.Local+B.Local+C.Local)/3.;
            for(const FVein& V:Veins) if(V.Shape.Distance(Center)<18) {Material=V.Gold?3:2; break;}
        }
        FSurface& S=Sections[Material];
        const int32 Start=S.Vertices.Num();
        const int32 Plane=Top?2:Normal.GetAbsMax()==FMath::Abs(Normal.Z)?2:FMath::Abs(Normal.X)>FMath::Abs(Normal.Y)?0:1;
        for(const auto& Corner:{A,B,C}) {
            const FVector V=Corner.Position;
            S.Vertices.Add(V-Origin); S.Normals.Add(Normal);
            S.UV.Add(Plane==2?FVector2D(V.X,V.Y)/(Top?400.:320.):Plane==0?FVector2D(V.Y,-V.Z)/320.:FVector2D(V.X,-V.Z)/320.);
        }
        // Unreal's front faces wind clockwise when seen from the empty side.
        S.Indices.Append({Start,Start+2,Start+1});
    }
}

void UETGeology::Rebuild(AETRegion& R,FIntVector Key)
{
    const auto& Field=Fields.FindChecked(Key);
    const FVector Origin(Key.X*double(Span),Key.Y*double(Span),Key.Z*double(Span)+ZOffset);
    constexpr int32 Row=Cells+1, Plane=Row*Row;
    auto& Heights=R.RockHeights.FindOrAdd({Key.X,Key.Y});
    if(Heights.IsEmpty()) {
        Heights.Reserve(Plane);
        for(int32 Y=0;Y<Row;++Y) for(int32 X=0;X<Row;++X)
            Heights.Add(World->SurfaceHeight(Origin.X+X*Step,Origin.Y+Y*Step));
    }
    TArray<FSample> Samples;
    Samples.Reserve(Plane*Row);
    for(int32 Z=0;Z<Row;++Z) for(int32 Y=0;Y<Row;++Y) for(int32 X=0;X<Row;++X) {
        const FVector Local=Origin+FVector(X*Step,Y*Step,Z*Step);
        Samples.Add({Local,Local+FVector(0,0,Heights[Y*Row+X]),FMath::Min(-Local.Z,Field.AirDistance(Local,Heights[Y*Row+X]))});
    }
    const auto Ore=Veins(FBox(Origin,Origin+FVector(Span)).ExpandBy(Step));
    FSurface Sections[4];
    const int32 Offsets[]={0,1,Row,Row+1,Plane,Plane+1,Plane+Row,Plane+Row+1};
    const int32 Tets[][4]={{0,1,3,7},{0,3,2,7},{0,2,6,7},{0,6,4,7},{0,4,5,7},{0,5,1,7}};
    for(int32 Z=0;Z<Cells;++Z) for(int32 Y=0;Y<Cells;++Y) for(int32 X=0;X<Cells;++X) {
        const FSample* First=&Samples[Z*Plane+Y*Row+X];
        bool CrossingCell=false;
        for(int32 I=1;I<8;++I) if((First[Offsets[I]].Solid>0)!=(First->Solid>0)) {CrossingCell=true; break;}
        if(!CrossingCell) continue;
        for(const auto& Tet:Tets) {
            const FSample* Solid[4], *Air[4]; int32 NS=0,NA=0;
            for(int32 I:Tet) {
                const FSample* S=First+Offsets[I];
                if(S->Solid>0) Solid[NS++]=S; else Air[NA++]=S;
            }
            if(!NS||!NA) continue;
            const FVector NormalHint=Air[0]->Position-Solid[0]->Position;
            if(NS==2) {
                const auto A=Crossing(*Solid[0],*Air[0]), B=Crossing(*Solid[0],*Air[1]);
                const auto C=Crossing(*Solid[1],*Air[1]), D=Crossing(*Solid[1],*Air[0]);
                Facet(A,B,C,NormalHint,Origin,Ore,Sections); Facet(A,C,D,NormalHint,Origin,Ore,Sections);
            } else {
                const FSample& Tip=NS==1?*Solid[0]:*Air[0];
                const FSample** Base=NS==1?Air:Solid;
                Facet(Crossing(Tip,*Base[0]),Crossing(Tip,*Base[1]),Crossing(Tip,*Base[2]),NormalHint,Origin,Ore,Sections);
            }
        }
    }
    auto* Mesh=R.RockMeshes.FindRef(Key);
    if(!Mesh) {
        Mesh=NewObject<UProceduralMeshComponent>(&R);
        Mesh->SetupAttachment(R.Terrain); Mesh->SetRelativeLocation(Origin-R.GetActorLocation());
        Mesh->ComponentTags.Add(TEXT("ET_Rock")); Mesh->bUseAsyncCooking=false;
        Mesh->bUseComplexAsSimpleCollision=true; Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->SetCastShadow(true); Mesh->bAffectDistanceFieldLighting=false;
        Mesh->RegisterComponent(); R.RockMeshes.Add(Key,Mesh);
    }
    const TCHAR* Names[]={TEXT("Ground"),TEXT("FaultShale"),TEXT("FaultIron"),TEXT("FaultGold")};
    for(int32 I=0;I<4;++I) {
        const auto& S=Sections[I];
        if(S.Indices.IsEmpty()) {if(Mesh->GetProcMeshSection(I)) Mesh->ClearMeshSection(I); continue;}
        Mesh->CreateMeshSection_LinearColor(I,S.Vertices,S.Indices,S.Normals,S.UV,{}, {},true);
        Mesh->SetMaterial(I,World->Material(Names[I]));
    }
}
