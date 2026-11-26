#include "ETGeology.h"
#include "ETGame.h"

using namespace ETRock;
namespace
{
    FVector SamplePosition(FIntVector Node) {return FVector(Node)*Step+FVector(0,0,ZOffset);}
    FIntVector GridFloor(FVector Position)
    {
        Position=(Position-FVector(0,0,ZOffset))/Step;
        return FIntVector(FMath::FloorToInt32(Position.X),FMath::FloorToInt32(Position.Y),FMath::FloorToInt32(Position.Z));
    }
}
void UETGeology::IndexFallen(FIntVector Node,TSet<FIntVector>& Dirty)
{
    const FVector P=SamplePosition(Node);
    const auto A=Chunk(P-FVector(Step)),B=Chunk(P+FVector(Step));
    for(int Z=A.Z;Z<=FMath::Min(B.Z,-1);++Z) for(int Y=A.Y;Y<=B.Y;++Y) for(int X=A.X;X<=B.X;++X)
        TouchChunk({X,Y,Z},Dirty).Fallen.Add(Node);
}
FBox UETGeology::BreakDetachedRock(FVector Center,FVector Reach,TSet<FIntVector>& Dirty)
{
    // These are the shared edges of the tetrahedra used by the visual and collision surface.
    static const FIntVector Steps[]={{0,0,-1},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},
        {1,1,0},{-1,-1,0},{1,0,1},{-1,0,-1},{0,1,1},{0,-1,-1},{1,1,1},{-1,-1,-1}};
    TSet<FIntVector> Anchored,Air;
    TMap<FIntPoint,double> Heights;
    auto HeightAt=[&](FIntVector Node) {
        const FIntPoint XY(Node.X,Node.Y);
        if(const double* H=Heights.Find(XY)) return *H;
        const double H=World->SurfaceHeight(double(Node.X)*Step,double(Node.Y)*Step);
        Heights.Add(XY,H); return H;
    };
    auto Solid=[&](FIntVector Node) {
        if(Node.Z>=0||Air.Contains(Node)) return false;
        const FVector P=SamplePosition(Node);
        const auto* Field=Fields.Find(Chunk(P));
        if(!Field||Field->AirDistance(P,HeightAt(Node))>0) return true;
        Air.Add(Node); return false;
    };
    auto Bedrock=[&](FIntVector Node) {
        const auto K=Chunk(SamplePosition(Node));
        const int32* Low=LowestCut.Find(FIntPoint(K.X,K.Y));
        return !Low||K.Z<*Low;
    };
    FBox Lost(ForceInit);
    const auto First=GridFloor(Center-Reach)-FIntVector(1),Last=GridFloor(Center+Reach)+FIntVector(1);
    for(int Z=First.Z;Z<=FMath::Min(-1,Last.Z);++Z) for(int Y=First.Y;Y<=Last.Y;++Y) for(int X=First.X;X<=Last.X;++X) {
        const FIntVector Start(X,Y,Z);
        // Only a new exposed surface can cut a formerly anchored component loose.
        if(Anchored.Contains(Start)||!Solid(Start)) continue;
        bool Exposed=false; for(const auto Dir:Steps) if(!Solid(Start+Dir)) {Exposed=true; break;}
        if(!Exposed) continue;
        TArray<FIntVector> Queue={Start}; TSet<FIntVector> Seen; Seen.Add(Start);
        bool Fixed=false;
        for(int I=0;I<Queue.Num()&&!Fixed;++I) {
            const FIntVector Here=Queue[I];
            // A continuous solid column into unedited bedrock is a cheap positive
            // proof. Otherwise follow actual mesh edges, including across tile seams.
            FIntVector Below=Here;
            while(Solid(Below)) {
                if(Anchored.Contains(Below)||Bedrock(Below)) {Fixed=true; break;}
                --Below.Z;
            }
            if(Fixed) break;
            for(const auto Dir:Steps) {
                const auto Next=Here+Dir;
                if(Anchored.Contains(Next)) {Fixed=true;break;}
                if(!Seen.Contains(Next)&&Solid(Next)) {Seen.Add(Next); Queue.Add(Next);}
            }
        }
        if(!Fixed) for(const FIntVector Node:Queue) {
            const FVector2D XY(Node.X*double(Step),Node.Y*double(Step)),Half(Step*.5);
            if(DigRestriction(FBox2D(XY-Half,XY+Half))) {Fixed=true;break;}
        }
        if(Fixed) {Anchored.Append(Seen); continue;}
        for(const auto Node:Queue) {
            World->State->FallenRock.Add(Node); IndexFallen(Node,Dirty); Air.Add(Node);
            const FVector P=SamplePosition(Node)+FVector(0,0,HeightAt(Node)); Lost+=P;
        }
    }
    return Lost.IsValid?Lost.ExpandBy(Step):Lost;
}
