#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SaveGame.h"
#include "ETWeatherMath.h"
#include "ETGame.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class AETWorld;
class AETCharacter;
class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class USpotLightComponent;
class UAudioComponent;
class USoundBase;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USkyAtmosphereComponent;

struct FETSupply
{
    FString Id, Name, Description;
    bool Usable=false;
};
struct FETRecipe
{
    FString Id, Name, Output;
    int32 Scrap=0, Count=1;
    TMap<FString,int32> Ingredients;
};
struct FETLootEntry { FString Id; int32 Min=1, Max=1; };
struct FETVoiceLine { FString Speaker, Text; };

USTRUCT()
struct FETWeapon
{
    GENERATED_BODY()
    FString Name;
    int32 Mag=24, Reserve=180, Pellets=1;
    float Damage=24, Rate=.16f, Reload=1.45f, Spread=.011f, Range=9500;
};

USTRUCT()
struct FETPerk
{
    GENERATED_BODY()
    FString Id, Name, Description, Branch;
    FVector2D Position=FVector2D::ZeroVector;
    int32 Cost=1;
    TArray<FString> Neighbors;
};

USTRUCT()
struct FETQuest
{
    GENERATED_BODY()
    FString Id, Giver, Title, Description, Item;
    int32 Water=0, Gold=0, Weapon=-1;
};

UCLASS()
class EARTHTOTALED_API UETSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Schema=1;
    UPROPERTY() int32 Seed=84371;
    UPROPERTY() FVector Position=FVector(0,-1500,110);
    UPROPERTY() FRotator View=FRotator(0,90,0);
    UPROPERTY() float Health=100;
    UPROPERTY() float Armor=40;
    UPROPERTY() float Hydration=100;
    UPROPERTY() float Elapsed=0;
    UPROPERTY() int32 Water=3;
    // Candybar-wrapper balance. Keep this serialized name so existing saves retain every wrapper.
    UPROPERTY() int32 Gold=0;
    UPROPERTY() int32 Scrap=0;
    UPROPERTY() int32 Meds=3;
    UPROPERTY() int32 Kills=0;
    UPROPERTY() int32 XP=0;
    UPROPERTY() int32 Level=1;
    UPROPERTY() int32 Points=2;
    UPROPERTY() int32 Weapon=0;
    UPROPERTY() int32 Cores=0;
    UPROPERTY() bool Welcome=false;
    UPROPERTY() bool Allied=false;
    UPROPERTY() FString Ending;
    UPROPERTY() TArray<FString> Skills={TEXT("survivor")};
    UPROPERTY() TArray<int32> Ammo;
    UPROPERTY() TArray<int32> Reserve;
    UPROPERTY() TArray<int32> Unlocked={0};
    UPROPERTY() TSet<FString> Consumed;
    UPROPERTY() TSet<FString> Items;
    UPROPERTY() TMap<FString,int32> Quests;
    UPROPERTY() TMap<FString,float> Rations;
    UPROPERTY() TMap<FString,float> AgentHealth;
    UPROPERTY() TMap<FString,FVector> DropPositions;
    UPROPERTY() TMap<FString,int32> DropAmounts;
    UPROPERTY() TMap<FString,FString> DropKinds;
    UPROPERTY() TMap<FString,int32> Inventory;
    UPROPERTY() float StimTime=0;
    UPROPERTY() int32 ViewDistance=500;
    UPROPERTY() int32 RenderScale=100;
    UPROPERTY() int32 MusicVolume=50;
    UPROPERTY() int32 WeatherVolume=75;
};

UCLASS()
class EARTHTOTALED_API AETItem : public AActor
{
    GENERATED_BODY()
public:
    AETItem();
    UPROPERTY() UStaticMeshComponent* Mesh;
    FString Id, Kind, Label, Value;
    int32 Amount=0;
    FVector Destination=FVector::ZeroVector;
};

UCLASS()
class EARTHTOTALED_API AETAgent : public AActor
{
    GENERATED_BODY()
public:
    AETAgent();
    virtual void Tick(float Delta) override;
    virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    void Configure(AETWorld* InWorld, const FString& InId, const FString& InKind);
    UPROPERTY() USceneComponent* Pivot;
    UPROPERTY() TArray<UStaticMeshComponent*> Parts;
    UPROPERTY() AETWorld* World;
    FString Id, Kind;
    float Health=100, AttackTime=0, Phase=0;
    FVector Home=FVector::ZeroVector;
};

UCLASS()
class EARTHTOTALED_API AETRegion : public AActor
{
    GENERATED_BODY()
public:
    AETRegion();
    void Generate(AETWorld* InWorld, FIntPoint InCoord);
    void BeginGenerate(AETWorld* InWorld, FIntPoint InCoord);
    bool AdvanceGenerate();
    void AddDetails();
    bool BuildTerrainSlice();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY() UProceduralMeshComponent* Terrain;
    UPROPERTY() TArray<AActor*> Residents;
    UPROPERTY() TArray<UHierarchicalInstancedStaticMeshComponent*> Instances;
    FIntPoint Coord;
    UPROPERTY() AETWorld* World;
    FRandomStream Random;
    int32 Stage=0;
    int32 TerrainGrid=0, WantedTerrainGrid=80, TerrainBuildGrid=0, TerrainRow=0;
    TArray<FVector> TerrainVertices;
};

UCLASS()
class EARTHTOTALED_API AETWorld : public AActor
{
    GENERATED_BODY()
public:
    AETWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    void LoadDesign();
    void EnsureRegion(FIntPoint Coord);
    void BuildStory();
    void RestoreDrops(AETRegion* Region);
    void Save();
    void UpdateDaylight();
    void InitAtmosphere();
    void TickAtmosphere(float Delta);
    void UpdateAtmosphereAudio(float Delta,bool Immediate=false);
    void UpdateRain(float Delta,FVector Eye);
    bool IsSheltered(FVector Eye) const;
    float RainFloor(FVector Position) const;
    FString WeatherName() const;
    int32 MusicFor(FVector Position) const;
    void RunSelfTest();
    void RunAtmosphereChecks(TFunctionRef<void(bool,const FString&)> Check);
    void LoadExpansion();
    void LoadDialogue();
    bool Speak(const FString& Id,const FString& SpeakerOverride=TEXT(""));
    bool HasSubtitle() const;
    void StreamAround(FVector Position, bool Preload=false);
    double RegionDistance(FIntPoint Coord, FVector Position) const;
    void UpdateVisibility(FVector Position);
    void Sound(const FString& Name, float Volume=.5f, float Pitch=1.f);
    void Loot(AETCharacter* Player, const FString& Id, const FString& Table);
    FVector CityCenter(double X, double Y) const;
    bool InCity(FVector P) const;
    FString PlaceName(FVector P) const;
    float Hour() const;
    double Height(double X, double Y) const;
    FVector HouseAnchor(FIntPoint Coord, int32 Index) const;
    AETItem* Item(const FString& Id, const FString& Kind, const FString& Label, FVector Position, int32 Amount=0, const FString& Value=TEXT(""));
    AActor* Static(const FString& Asset, const FString& Material, FVector Position, FVector Scale=FVector::OneVector, FRotator Rotation=FRotator::ZeroRotator);
    UMaterialInterface* Material(const FString& Name) const;
    void DressMesh(UStaticMeshComponent* Component,const FString& BaseMaterial) const;
    uint32 SeedFor(FIntPoint Coord, int32 Salt=0) const;
    UPROPERTY() UETSave* State;
    UPROPERTY() TMap<FIntPoint,AETRegion*> Regions;
    UPROPERTY() ADirectionalLight* Sun;
    UPROPERTY() ADirectionalLight* Moon;
    UPROPERTY() ASkyLight* Sky;
    UPROPERTY() AExponentialHeightFog* Fog;
    UPROPERTY() TMap<FString,USoundBase*> Sounds;
    UPROPERTY() TArray<UAudioComponent*> Ambience;
    UPROPERTY() UAudioComponent* DialogueAudio=nullptr;
    UPROPERTY() TArray<UAudioComponent*> Music;
    UPROPERTY() TArray<UAudioComponent*> WeatherAudio;
    UPROPERTY() UInstancedStaticMeshComponent* RainMesh=nullptr;
    UPROPERTY() UStaticMeshComponent* Clouds=nullptr;
    UPROPERTY() UMaterialInstanceDynamic* CloudMaterial=nullptr;
    UPROPERTY() USkyAtmosphereComponent* Atmosphere=nullptr;
    UPROPERTY() ADirectionalLight* LightningLight=nullptr;
    ETClimate::Frame Weather;
    FRandomStream WeatherRandom;
    TArray<FVector> RainPositions;
    TArray<float> RainFloors;
    bool Sheltered=false, AtmosphereAuditWritten=false;
    int32 WeatherOverride=-1;
    float RainTick=0, ShelterTick=0, ThunderTimer=16, ThunderDelay=-1, Lightning=0, AtmosphereAge=0;
    double AudioLastTick=0;
    UPROPERTY() TArray<AActor*> StoryActors;
    TArray<FETWeapon> Weapons;
    TArray<FETSupply> Supplies;
    TArray<FETRecipe> Recipes;
    TMap<FString,TArray<FETLootEntry>> LootTables;
    TMap<FString,FETVoiceLine> VoiceLines;
    FString VoiceId, SubtitleSpeaker, SubtitleText;
    double VoiceStartedAt=0, SubtitleUntil=0;
    TArray<FETPerk> Perks;
    TArray<FETQuest> Quests;
    TArray<FString> Recordings, ChapterNames, Objectives;
    FString Notice;
    float NoticeTime=0, StreamTime=0, SaveTime=0;
    float LightTime=0;
    float VisibilityTime=0;
    bool Review=false, SelfTest=false, TestRan=false;
    void Say(const FString& Text);
    static constexpr double RegionSize=32000.0;
};

UCLASS()
class EARTHTOTALED_API AETCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AETCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    UPROPERTY() UCameraComponent* Camera;
    UPROPERTY() UStaticMeshComponent* Gun;
    UPROPERTY() USpotLightComponent* Flashlight;
    UPROPERTY() class UPointLightComponent* Muzzle;
    UPROPERTY() AETWorld* World;
    UPROPERTY() AETItem* Focus;
    bool Firing=false, Aiming=false, Sprinting=false;
    bool Climbing=false;
    FVector ClimbStart, ClimbEnd;
    float ClimbAlpha=0, ShotTime=0, ReloadTime=0, FallSpeed=0;
    float FireGuard=0, CaptureRetry=0;
    float StepTime=0, HitTime=0, MuzzleTime=0;
    bool InputAudit=false;
    FString Panel, PanelTitle, PanelBody;
    FString Contact;
    void Forward(float Axis); void Right(float Axis); void Turn(float Axis); void Look(float Axis);
    void FireStart(); void FireStop(); void AimStart(); void AimStop(); void SprintStart(); void SprintStop();
    void Interact(); void Reload(); void Drink(); void Heal(); void Skills(); void Journal(); void Pause(); void Fullscreen();
    void SelectWeapon(int32 Index);
    void Weapon1(); void Weapon2(); void Weapon3(); void Weapon4(); void Weapon5(); void Weapon6();
    void Weapon7(); void Weapon8(); void Weapon9(); void Weapon10();
    void NextWeapon(); void PreviousWeapon(); void CycleWeapon(int32 Direction);
    void Inventory(); void UseSupply(const FString& Id); void Craft(const FString& Id);
    void Shoot(); void ClosePanel(); void OpenPanel(const FString& Kind, const FString& Title, const FString& Body);
    void Action(const FString& Id); void ContactPanel(const FString& Name);
    bool Has(const FString& Skill) const;
    float MaxHealth() const; float LootRate() const;
    void AddXP(int32 Amount);
    void SetMouseMode(bool Menu);
    void Die();
    void ToggleLight();
    void ApplyGraphics();
};

UCLASS()
class EARTHTOTALED_API AETHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    void Text(const FString& Value, float X, float Y, FLinearColor Color=FLinearColor::White, float Scale=1);
    void Wrapped(const FString& Value, float X, float Y, float Width, float Scale=1);
    void Button(const FString& Id, const FString& Label, float X, float Y, float Width=260);
    void DialogueSubtitle();
};

UCLASS()
class EARTHTOTALED_API AETGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AETGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& Error) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    UPROPERTY() AETWorld* World;
    UPROPERTY() AActor* Start;
};
