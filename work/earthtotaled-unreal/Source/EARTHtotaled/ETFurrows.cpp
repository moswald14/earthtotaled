#include "ETField.h"
#include "ETGeology.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"

void UETGeology::IndexFurrow(const FETFurrow& L,TSet<FIntVector>& Dirty){
    const FVector Pad(280,280,0);
    const auto A=ETRock::Chunk(L.Inlet.ComponentMin(L.Outlet)-Pad),B=ETRock::Chunk(L.Inlet.ComponentMax(L.Outlet)+Pad);
    for(int Y=A.Y;Y<=B.Y;++Y)for(int X=A.X;X<=B.X;++X) TouchChunk({X,Y,-1},Dirty).Channels.Emplace(L.Inlet,L.Outlet);
}
bool UETGeology::CarveFurrow(FVector A,FVector B,FString& Why){
    auto& Lines=World->State->Furrows;
    if(Lines.Num()>=4096){Why=TEXT("This world has reached its furrow capacity");return false;}
    // Snap to an existing outlet so a branch keeps its hydraulic height across streaming and saves.
    auto Snap=[&](FVector& P,bool Upstream){
        double Best=135.;TOptional<FVector> Close;
        for(const auto& L:Lines){const auto& V=Upstream?L.Outlet:L.Inlet;const double Dist=FVector::Dist2D(P,V);if(Dist<Best){Best=Dist;Close=V;}}
        if(Close.IsSet())P=Close.GetValue();return Close.IsSet();
    };
    const bool Joined=Snap(A,true),JoinsNext=Snap(B,false);
    const double Length=FVector::Dist2D(A,B);
    if(Length<295||Length>1450){Why=TEXT("Choose an endpoint 3 to 14 metres from the inlet");return false;}
    if(!Joined) A.Z=World->SurfaceHeight(A.X,A.Y)-18;
    if(!JoinsNext) B.Z=FMath::Min(World->SurfaceHeight(B.X,B.Y)-18,A.Z-Length*.008);
    if(A.Z-B.Z<Length*.005||A.Z-B.Z>Length*.14){Why=TEXT("Choose a gentler downhill route");return false;}
    for(const auto& Existing:Lines)if(FVector::Dist2D(A,Existing.Inlet)<45&&FVector::Dist2D(B,Existing.Outlet)<45){Why=TEXT("This furrow already exists");return false;}
    const int Samples=FMath::CeilToInt(Length/80.);
    for(int K=0;K<=Samples;++K){
        const auto P=FMath::Lerp(A,B,double(K)/Samples);
        const double Cover=World->SurfaceHeight(P.X,P.Y)-P.Z;
        if(Cover<2||Cover>95){Why=TEXT("Keep the furrow shallow; turn along the contour");return false;}
        if(IsOpen(P-FVector(0,0,195))){Why=TEXT("This bedrock opens into a deeper excavation");return false;}
        const FVector2D XY(P),Pad(159.);
        if(const auto* Denied=DigRestriction(FBox2D(XY-Pad,XY+Pad),true)){Why=Denied;return false;}
    }
    FETFurrow& L=Lines.AddDefaulted_GetRef();L.Inlet=A;L.Outlet=B;
    TSet<FIntVector> Dirty;IndexFurrow(L,Dirty);
    TSet<AETRegion*> Changed;
    for(const auto& Key:Dirty)if(auto* R=World->Regions.FindRef(ETRock::Region(Key));IsValid(R)&&R->TerrainGrid){
        Rebuild(*R,Key);Changed.Add(R);ClearScatter(*R,{Key.X,Key.Y});
    }
    for(auto* R:Changed) R->RefreshSurfaceCollision();
    World->WetFurrows=World->FlowingFurrows();
    Why=World->WetFurrows.Contains(Lines.Num()-1)?TEXT("Water flowing / continue from the outlet"):TEXT("Dry furrow / connect the inlet to a pumped cistern");
    return true;
}
void AETCharacter::AimFurrow(FVector Ground){
    if(!FurrowStart.IsSet()){
        FurrowStart=Ground;World->Say(TEXT("Inlet marked / aim 3-14 m downstream and tap fire"));return;
    }
    FString Why;
    if(World->Geology->CarveFurrow(FurrowStart.GetValue(),Ground,Why)){
        FurrowStart=World->State->Furrows.Last().Outlet;World->Sound(TEXT("BorerPulse"),.35f,1.2f);World->Save();
    }
    World->Say(Why);
}
