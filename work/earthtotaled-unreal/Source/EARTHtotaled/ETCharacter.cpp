#include "ETGame.h"
#include "ETLandfall.h"
#include "ETGeology.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundBase.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"

namespace
{
    // Shipping builds strip UE_LOG. Keep opt-in input evidence in the QA user directory.
    void WriteInputAudit(const FString& Line)
    {
        static int32 Lines=0;
        if(Lines>=4096) return;
        const FString Directory=FPaths::ProjectSavedDir();
        IFileManager::Get().MakeDirectory(*Directory,true);
        FFileHelper::SaveStringToFile(Line+TEXT("\n"),*(Directory/TEXT("input-verification.txt")),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);
        ++Lines;
    }
}

AETCharacter::AETCharacter()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(34,92); BaseEyeHeight=72;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch=true; GetCharacterMovement()->SetCrouchedHalfHeight(54);
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes")); Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(0,0,72)); Camera->bUsePawnControlRotation=true; Camera->FieldOfView=90;
    Gun=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon")); Gun->SetupAttachment(Camera);
    Gun->SetRelativeLocation(FVector(78,19,-22)); Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision); Gun->SetCastShadow(false);
    Flashlight=CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight")); Flashlight->SetupAttachment(Camera);
    Flashlight->SetRelativeLocation(FVector(35,0,-10)); Flashlight->SetIntensity(18000); Flashlight->SetAttenuationRadius(4800);
    Flashlight->SetInnerConeAngle(16); Flashlight->SetOuterConeAngle(34); Flashlight->SetLightColor(FLinearColor(1,.92f,.76f));
    Flashlight->SetCastShadows(true); Flashlight->SetVisibility(false);
    Muzzle=CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash")); Muzzle->SetupAttachment(Camera);
    Muzzle->SetRelativeLocation(FVector(110,19,-17)); Muzzle->SetIntensity(4500); Muzzle->SetAttenuationRadius(420);
    Muzzle->SetLightColor(FLinearColor(1,.55f,.16f)); Muzzle->SetCastShadows(false); Muzzle->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed=430; GetCharacterMovement()->JumpZVelocity=440; GetCharacterMovement()->AirControl=.25f;
    bUseControllerRotationYaw=true;
}
void AETCharacter::BeginPlay()
{
    Super::BeginPlay();
    for(TActorIterator<AETWorld> It(GetWorld());It;++It) {World=*It; break;}
    if(!World) return;
    ApplyGraphics();
    if(World->State->Relics.IsValidIndex(World->State->EquippedRelic)) EquipRelic(World->State->EquippedRelic); else SelectWeapon(World->State->Weapon); if(Controller) Controller->SetControlRotation(World->State->View);
    InputAudit=FParse::Param(FCommandLine::Get(),TEXT("ETInputAudit")); SetMouseMode(false); CaptureRetry=.3f;
    if(!World->Review&&!GetWorld()->URL.HasOption(TEXT("ETContinue"))) {Title(); return;}
    if(World->HadSave&&World->State->Elapsed>0&&GetWorld()->URL.HasOption(TEXT("ETContinue")))
        EnemyReacquireTime=8.f;
    if(World->State->ArrivalStage==0) StartArrival();
    else if(!World->State->IntroRead&&World->State->Elapsed<1&&World->Recordings.Num()) {OpenPanel(TEXT("story"),TEXT("EARTHtotaled / THE LAST SIGNAL"),World->Recordings[0]); World->Speak(TEXT("archive0"));}
    FString ReviewScene; FParse::Value(FCommandLine::Get(),TEXT("ETReview="),ReviewScene);
    if(ReviewScene==TEXT("bench")) OpenPanel(TEXT("bench"),TEXT("SHELTER WORKBENCH"),TEXT("Patch your suit or improve your kit. Candybar wrappers are currency. Machines carry wrappers; water comes from people and relief supplies."));
    if(ReviewScene==TEXT("voice")) ContactPanel(TEXT("SALT-7"));
    if(ReviewScene==TEXT("mine")) ToggleBorer();
}
void AETCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    if(World) SetMouseMode(!Panel.IsEmpty());
}
void AETCharacter::SetupPlayerInputComponent(UInputComponent* I)
{
    Super::SetupPlayerInputComponent(I);
    I->BindAxis("Forward",this,&AETCharacter::Forward); I->BindAxis("Right",this,&AETCharacter::Right);
    I->BindAxis("Turn",this,&AETCharacter::Turn); I->BindAxis("Look",this,&AETCharacter::Look);
    I->BindAction("Fire",IE_Pressed,this,&AETCharacter::FireStart); I->BindAction("Fire",IE_Released,this,&AETCharacter::FireStop);
    I->BindAction("Aim",IE_Pressed,this,&AETCharacter::AimStart); I->BindAction("Aim",IE_Released,this,&AETCharacter::AimStop);
    I->BindAction("Sprint",IE_Pressed,this,&AETCharacter::SprintStart); I->BindAction("Sprint",IE_Released,this,&AETCharacter::SprintStop);
    I->BindAction("Jump",IE_Pressed,this,&AETCharacter::JumpPress); I->BindAction("Jump",IE_Released,this,&AETCharacter::JumpRelease);
    I->BindAction("Interact",IE_Pressed,this,&AETCharacter::Interact); I->BindAction("Reload",IE_Pressed,this,&AETCharacter::TurnTool);
    I->BindAction("Build",IE_Pressed,this,&AETCharacter::ToggleBuild);
    I->BindKey(EKeys::C,IE_Pressed,this,&AETCharacter::CycleMaterial);
    I->BindKey(EKeys::T,IE_Pressed,this,&AETCharacter::UpgradeFrame);
    I->BindKey(EKeys::F2,IE_Pressed,this,&AETCharacter::Controls).bExecuteWhenPaused=true;
    I->BindAction("Map",IE_Pressed,this,&AETCharacter::ToggleMap).bExecuteWhenPaused=true;
    I->BindKey(EKeys::LeftControl,IE_Pressed,this,&AETCharacter::CrouchLoot);
    I->BindKey(EKeys::LeftControl,IE_Released,this,&AETCharacter::ReleaseLoot).bExecuteWhenPaused=true;
    I->BindKey(EKeys::RightControl,IE_Pressed,this,&AETCharacter::CrouchLoot);
    I->BindKey(EKeys::RightControl,IE_Released,this,&AETCharacter::ReleaseLoot).bExecuteWhenPaused=true;
    I->BindKey(FInputChord(EKeys::F10,true,true,false,false),IE_Pressed,this,&AETCharacter::HiddenTools).bExecuteWhenPaused=true;
    I->BindKey(EKeys::F1,IE_Pressed,this,&AETCharacter::OpenConsole).bExecuteWhenPaused=true;
    I->BindAction("Drink",IE_Pressed,this,&AETCharacter::Drink); I->BindAction("Heal",IE_Pressed,this,&AETCharacter::Heal);
    I->BindAction("Skills",IE_Pressed,this,&AETCharacter::Skills).bExecuteWhenPaused=true;
    I->BindAction("Journal",IE_Pressed,this,&AETCharacter::Journal).bExecuteWhenPaused=true;
    I->BindAction("Pause",IE_Pressed,this,&AETCharacter::Pause).bExecuteWhenPaused=true;
    I->BindAction("Fullscreen",IE_Pressed,this,&AETCharacter::Fullscreen).bExecuteWhenPaused=true;
    I->BindAction("Flashlight",IE_Pressed,this,&AETCharacter::ToggleLight);
    I->BindAction("Borer",IE_Pressed,this,&AETCharacter::ToggleBorer);
    I->BindAction("Winch",IE_Pressed,this,&AETCharacter::Winch);
    I->BindAction("Weapon1",IE_Pressed,this,&AETCharacter::Weapon1); I->BindAction("Weapon2",IE_Pressed,this,&AETCharacter::Weapon2);
    I->BindAction("Weapon3",IE_Pressed,this,&AETCharacter::Weapon3); I->BindAction("Weapon4",IE_Pressed,this,&AETCharacter::Weapon4);
    I->BindAction("Weapon5",IE_Pressed,this,&AETCharacter::Weapon5); I->BindAction("Weapon6",IE_Pressed,this,&AETCharacter::Weapon6);
    I->BindAction("Weapon7",IE_Pressed,this,&AETCharacter::Weapon7); I->BindAction("Weapon8",IE_Pressed,this,&AETCharacter::Weapon8);
    I->BindAction("Weapon9",IE_Pressed,this,&AETCharacter::Weapon9); I->BindAction("Weapon10",IE_Pressed,this,&AETCharacter::Weapon10);
    I->BindAction("NextWeapon",IE_Pressed,this,&AETCharacter::NextWeapon); I->BindAction("PreviousWeapon",IE_Pressed,this,&AETCharacter::PreviousWeapon);
    I->BindAction("Inventory",IE_Pressed,this,&AETCharacter::Inventory).bExecuteWhenPaused=true;
}
void AETCharacter::Forward(float V) {if(Panel.IsEmpty()&&!Climbing) AddMovementInput(GetActorForwardVector(),V);}
void AETCharacter::Right(float V) {if(Panel.IsEmpty()&&!Climbing) AddMovementInput(GetActorRightVector(),V);}
void AETCharacter::Turn(float V) {
    if(Panel.IsEmpty()) {
        AddControllerYawInput(V*.7f);
        if(InputAudit&&!FMath::IsNearlyZero(V)) {
            auto PC=Cast<APlayerController>(Controller);
            UE_LOG(LogTemp,Display,TEXT("ET_MOUSE axis=%.3f yaw=%.2f left=%d right=%d ignored=%d cursor=%d"),V,GetControlRotation().Yaw,PC?PC->IsInputKeyDown(EKeys::LeftMouseButton):0,PC?PC->IsInputKeyDown(EKeys::RightMouseButton):0,PC?PC->IsLookInputIgnored():1,PC?PC->bShowMouseCursor:1);
            WriteInputAudit(FString::Printf(TEXT("ET_MOUSE axis=%.3f yaw=%.2f left=%d right=%d ignored=%d cursor=%d"),V,GetControlRotation().Yaw,PC?PC->IsInputKeyDown(EKeys::LeftMouseButton):0,PC?PC->IsInputKeyDown(EKeys::RightMouseButton):0,PC?PC->IsLookInputIgnored():1,PC?PC->bShowMouseCursor:1));
        }
    }
}
void AETCharacter::Look(float V) {if(Panel.IsEmpty()) AddControllerPitchInput(V*.7f);}
void AETCharacter::FireStart() {if(!Panel.IsEmpty()||FireGuard>0) return; if(Building) PlaceBuild(); else Firing=true;} void AETCharacter::FireStop() {Firing=false;}
void AETCharacter::AimStart() {if(!Panel.IsEmpty()) return; if(Building) SalvageBuild(); else Aiming=true;} void AETCharacter::AimStop() {Aiming=false;}
void AETCharacter::SprintStart() {Sprinting=true;} void AETCharacter::SprintStop() {Sprinting=false;}
bool AETCharacter::Has(const FString& Id) const {return World && World->State->Skills.Contains(Id);}
float AETCharacter::MaxHealth() const {return 100+(Has(TEXT("vitality"))?25:0)+(Has(TEXT("hardy"))?25:0);}
float AETCharacter::LootRate() const {return 1+(Has(TEXT("scavenger"))?.2f:0)+(Has(TEXT("prospector"))?.2f:0)+(Has(TEXT("salvager"))?.15f:0)+(Has(TEXT("quartermaster"))?.1f:0);}
void AETCharacter::AddXP(int32 Amount)
{
    if(Amount<=0||!World) return;
    auto* S=World->State;
    LastXPGain=XPNoticeTime>0?FMath::Min<int64>(MAX_int32,int64(LastXPGain)+Amount):Amount; XPNoticeTime=2.f;
    int64 Total=int64(S->XP)+Amount;
    const int32 Before=S->Level;
    while(Total>=S->NextLevelXP()&&S->Level<1000000) {Total-=S->NextLevelXP(); ++S->Level; S->Points=FMath::Min<int64>(MAX_int32,int64(S->Points)+1);}
    S->XP=FMath::Min<int64>(Total,MAX_int32);
    if(S->Level>Before) {LevelNoticeTime=4.2f; World->Sound(TEXT("OreChime"),.65f,1.22f);}
}
void AETCharacter::Tick(float D)
{
    Super::Tick(D); if(!World||World->Weapons.IsEmpty()) return; auto S=World->State;
    if(!Panel.IsEmpty()||S->Health<=0) return;
    TickImpact(D); TickShield(D); TickSweep(D);
    XPNoticeTime=FMath::Max(0.f,XPNoticeTime-D); LevelNoticeTime=FMath::Max(0.f,LevelNoticeTime-D);
    EnemyReacquireTime=FMath::Max(0.f,EnemyReacquireTime-D);
    TickJet(D);
    if(Building) {BuildRefresh-=D; if(BuildRefresh<=0) {RefreshBuild(); BuildRefresh=.075f;}}
    S->StimTime=FMath::Max(0.f,S->StimTime-D); HitTime=FMath::Max(0.f,HitTime-D); MuzzleTime=FMath::Max(0.f,MuzzleTime-D); Muzzle->SetVisibility(MuzzleTime>0);
    S->Elapsed+=D; ShotTime=FMath::Max(0.f,ShotTime-D); FireGuard=FMath::Max(0.f,FireGuard-D);
    if(CaptureRetry>0) {CaptureRetry-=D; if(CaptureRetry<=0) SetMouseMode(false);}
    float Move=(Has(TEXT("parkour"))?1.12f:1)*(Has(TEXT("pressure"))?1.06f:1)*(Has(TEXT("endurance"))?1.05f:1);
    GetCharacterMovement()->MaxWalkSpeed=(Sprinting?670:430)*Move*(S->StimTime>0?1.1f:1.f);
    StepTime-=D;
    if(GetVelocity().Size2D()>80&&GetCharacterMovement()->IsMovingOnGround()&&StepTime<=0&&!Climbing) {
        StepTime=Sprinting?.30f:.46f; World->Sound(World->InCity(GetActorLocation())?TEXT("StepCity"):TEXT("StepForest"),Sprinting?.32f:.23f,FMath::FRandRange(.88f,1.12f));
    }
    Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,Aiming&&!Borer?65.f:90.f,D,12));
    if(Borer&&Aiming) {
        SurveyTime-=D;
        if(SurveyTime<=0) {SurveyReading=World->Geology->Survey(Camera->GetComponentLocation(),Camera->GetForwardVector()); SurveyTime=.15f;}
    } else SurveyTime=0;
    float Drain=.055f*(Sprinting&&GetVelocity().Size2D()>20?1.7f:1)*(Has(TEXT("hydration"))?.8f:1)*(Has(TEXT("endurance"))?.9f:1);
    S->Hydration=FMath::Max(0.f,S->Hydration-Drain*D*(S->StimTime>0?.7f:1.f));
    // Dehydration bypasses armor; menus pause all survival time.
    if(S->Hydration<=0&&!TestInvincible) {S->Health=FMath::Max(0.f,S->Health-3*D); if(S->Health<=0) {Die(TEXT("Dehydration")); return;}}
    if(ReloadTime>0)
    {
        ReloadTime-=D; if(ReloadTime<=0)
        {
            CurrentMagazine()=CurrentGun().Mag;
        }
    }
    if(Climbing)
    {
        ClimbAlpha+=D/((Has(TEXT("climber2"))?1/1.4f:1)*2.0f);
        const float T=FMath::Clamp(ClimbAlpha,0.f,1.f);
        // Lift outside the wall first, then cross above the parapet.
        FVector Target=FMath::Lerp(ClimbStart,ClimbEnd,FMath::Clamp((T-.75f)*4,0.f,1.f));
        Target.Z=FMath::Lerp(ClimbStart.Z,ClimbEnd.Z,FMath::Min(1.f,T/.75f));
        FHitResult Hit; SetActorLocation(Target,true,&Hit);
        if(Hit.bBlockingHit||T>=1) {Climbing=false; GetCharacterMovement()->SetMovementMode(MOVE_Walking);}
    }
    FallSpeed=FMath::Min(FallSpeed,GetVelocity().Z);
    if(Firing&&!Climbing&&ShotTime<=0&&ReloadTime<=0) Shoot();
    Focus=nullptr; FocusBuild.Reset(); TreeInReach=false; FHitResult Hit; FVector Start=Camera->GetComponentLocation();
    FCollisionQueryParams Params; Params.AddIgnoredActor(this);
    Sighted.Reset();
    if(GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Camera->GetForwardVector()*9500,ECC_Visibility,Params)) {
        if(Hit.Distance<=400) {Focus=Cast<AETItem>(Hit.GetActor()); FocusBuild=Cast<AETConstruction>(Hit.GetActor()); FVector Root; TreeInReach=World->TreeRoot(Hit,Root);}
        Sighted=Cast<AETAgent>(Hit.GetActor());
    }
    for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Kind==TEXT("trap") && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<65 && FMath::Abs(It->GetActorLocation().Z-(GetActorLocation().Z-92))<70)
    {
        S->Consumed.Add(It->Id); It->Destroy(); Hurt(30,TEXT("A scavenger trap")); if(S->Health>0) World->Say(TEXT("TRAP / Q to use a medkit")); break;
    }
    const auto Position=GetActorLocation();
    const double Ground=World->SurfaceHeight(Position.X,Position.Y);
    if(GetCharacterMovement()->IsMovingOnGround()&&Position.Z>=Ground+70&&Position.Z<Ground+215)
        S->WinchAnchor=Position;
    if(Position.Z<Ground-3000&&!World->Geology->IsOpen(Position))
        SetActorLocation(FVector(Position.X,Position.Y,Ground+250));
}
void AETCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit); if(World&&FallSpeed<-850) Hurt((-FallSpeed-850)*.05f*(Has(TEXT("parkour"))?.75f:1)*(Has(TEXT("hardy"))?.8f:1),TEXT("Fall impact")); FallSpeed=0; JetUsed=AirJumpSpent=ThrustArmed=Jetting=false;
}
float AETCharacter::TakeDamage(float Damage,const FDamageEvent& Event,AController*,AActor* Causer)
{
    const auto* Source=Cast<AETAgent>(Causer);
    if(!Source&&Causer) Source=Cast<AETAgent>(Causer->GetOwner());
    FString Cause=Source ? (Source->Kind==TEXT("robot")?FString(TEXT("Supertech patrol fire")):
        Source->IsBrood()?FString(TEXT("Bug "))+Source->Kind:Source->IsMachine()?FString(TEXT("Supertech shard drone")):Source->Kind) : TEXT("Hostile impact");
    const float Applied=Hurt(Damage,Cause);
    if(Applied>0) ReceiveImpact(Applied,Event,Causer);
    return Applied;
}
float AETCharacter::Hurt(float Damage,const FString& Cause)
{
    if(!World||World->State->Health<=0||!Panel.IsEmpty()) return 0; auto S=World->State;
    if(Damage<=0||TestInvincible) return 0;
    S->ShieldDelay=10.f;
    const float Intercept=FMath::Min(S->Shield,Damage); S->Shield-=Intercept;
    const float Passing=Damage-Intercept,Absorbed=FMath::Min(S->Armor,Passing*(.6f+.08f*S->ArmorTier)); S->Armor-=Absorbed; S->Health=FMath::Max(0.f,S->Health-Passing+Absorbed);
    if(S->Health<=0) Die(Cause); return Damage;
}
void AETCharacter::Die(const FString& Cause)
{
    Firing=false;
    const FString Detail=FString::Printf(TEXT("Cause of death: %s\nHydration at death: %d%%  /  sealed bottles: %d\n\nYour last living save is available."),*Cause,FMath::RoundToInt(World->State->Hydration),World->State->Water);
    ConsoleLines.Add(Detail); OpenPanel(TEXT("death"),TEXT("SURVIVOR DOWN"),Detail);
}
void AETCharacter::Shoot()
{
    if(Borer) {FireBorer(); return;}
    auto S=World->State; int32 I=S->Weapon; const auto W=CurrentGun();
    if(CurrentMagazine()<=0) {Reload(); ShotTime=.15f; return;} --CurrentMagazine(); S->ShieldDelay=10; ShotTime=W.Rate; MuzzleTime=.045f; Muzzle->SetVisibility(true);
    const FString SoundPath=FString::Printf(TEXT("/Game/EARTHtotaled/Art/S_Fire%d.S_Fire%d"),I,I);
    if(auto Sound=LoadObject<USoundBase>(nullptr,*SoundPath)) UGameplayStatics::PlaySound2D(this,Sound,.65f,FMath::FRandRange(.96f,1.04f));
    FVector Start=Camera->GetComponentLocation(), Direction=Camera->GetForwardVector(); FCollisionQueryParams Params; Params.AddIgnoredActor(this);
    Direction=AssistedDirection(Start,Direction,W.Range);
    const int32 Rank=CurrentRank();
    float Earned=0; bool Killed=false;
    for(int32 P=0;P<W.Pellets;++P)
    {
        FVector Ray=FMath::VRandCone(Direction,W.Spread*(Aiming?.5f:1)); FHitResult Hit;
        FVector End=Start+Ray*W.Range;
        if(GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params))
        {
            End=Hit.ImpactPoint;
            if(auto Agent=Cast<AETAgent>(Hit.GetActor())) {
                const bool Hostile=Agent->Kind!=TEXT("deer");
                const float Power=W.Damage*(S->EquippedRelic==INDEX_NONE?1+.15f*(Rank-1):1.f)*(1+(Has(TEXT("marksman"))?.15f:0)+(Has(TEXT("steady"))?.15f:0));
                const float Dealt=UGameplayStatics::ApplyPointDamage(Agent,Power,Ray,Hit,GetController(),this,nullptr);
                if(Hostile) Earned+=Dealt;
                Killed|=Agent->Health<=0;
                if(Dealt>0) HitTime=.16f;
            }
        }
        DrawDebugLine(GetWorld(),Start+Direction*70,End,FColor(245,185,90),false,.045f,0,.8f);
    }
    CurrentGunDamage()+=Earned;
    if(CurrentRank()>Rank) {
        World->Sound(TEXT("OreChime"),.4f,1.25f);
        World->Say(FString::Printf(TEXT("%s upgraded to Mk %d"),*W.Name,CurrentRank()));
    }
    if(Killed||CurrentRank()>Rank) World->Save();
    AddControllerPitchInput(-.1f*(I==1?3:1));
}
void AETCharacter::Reload()
{
    if(!World||Borer||ReloadTime>0||!Panel.IsEmpty()) return; const auto W=CurrentGun();
    if(CurrentMagazine()>=W.Mag) return;
    ReloadTime=W.Reload*(Has(TEXT("reload"))?.8f:1)*(Has(TEXT("pressure"))?.9f:1)*(Has(TEXT("quartermaster"))?.9f:1); Firing=false;
    World->Sound(TEXT("Reload"),.5f,1.8f/W.Reload);
}
void AETCharacter::Drink()
{
    if(!World||!Panel.IsEmpty()) return; auto S=World->State;
    if(S->Water<=0) {World->Say(FString::Printf(TEXT("Pack: 0 sealed bottles  /  hydration %d%%. Settlements and relief caches stock water."),FMath::RoundToInt(S->Hydration))); return;}
    if(S->Hydration>=99) return; --S->Water; S->Hydration=FMath::Min(100.f,S->Hydration+45); World->Sound(TEXT("Drink")); World->Say(TEXT("Drank sealed water / hydration restored")); World->Save();
}
void AETCharacter::Heal()
{
    if(!World||!Panel.IsEmpty()) return; auto S=World->State; if(S->Meds<=0||S->Health>=MaxHealth()) return;
    --S->Meds; S->Health=FMath::Min(MaxHealth(),S->Health+45+(Has(TEXT("medic"))?25:0)+(Has(TEXT("salvager"))?10:0)); World->Sound(TEXT("Heal")); World->Save();
}
void AETCharacter::SelectWeapon(int32 I)
{
    if(!World||!World->Weapons.IsValidIndex(I)||!World->State->Unlocked.Contains(I)) return;
    World->State->EquippedRelic=INDEX_NONE; EquipFrame(I);
}
void AETCharacter::EquipFrame(int32 I)
{
    StopBuilding();
    if(Borer) {Borer=false; Firing=Aiming=false; Flashlight->SetVisibility(LampBeforeBorer);}
    World->State->Weapon=I; ReloadTime=0;
    World->Sound(TEXT("Switch"),.25f);
    const FString Asset=FString::Printf(TEXT("/Game/EARTHtotaled/Art/SM_Weapon%d.SM_Weapon%d"),I,I);
    Gun->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Asset)); World->DressMesh(Gun,TEXT("Steel"));
}
void AETCharacter::Weapon1(){SelectWeapon(0);} void AETCharacter::Weapon2(){SelectWeapon(1);} void AETCharacter::Weapon3(){SelectWeapon(2);}
void AETCharacter::Weapon4(){SelectWeapon(3);} void AETCharacter::Weapon5(){SelectWeapon(4);} void AETCharacter::Weapon6(){SelectWeapon(5);}
void AETCharacter::SetMouseMode(bool Menu)
{
    auto PC=Cast<APlayerController>(Controller); if(!PC) return;
    PC->bShowMouseCursor=Menu; PC->bEnableClickEvents=Menu;
    // Ignore-input calls stack. Refreshing a contact must not accumulate locks.
    PC->ResetIgnoreLookInput(); PC->SetIgnoreLookInput(Menu);
    if(Menu) {FInputModeGameAndUI M; M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); M.SetHideCursorDuringCapture(false); PC->SetInputMode(M);}
    else {
        FInputModeGameOnly M; M.SetConsumeCaptureMouseDown(true); PC->SetInputMode(M); PC->ResetIgnoreLookInput();
        if(auto Viewport=GetWorld()->GetGameViewport()) {
            Viewport->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
            Viewport->SetMouseLockMode(EMouseLockMode::LockAlways);
        }
        if(FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocusToGameViewport();
    }
    if(InputAudit) UE_LOG(LogTemp,Display,TEXT("ET_CAPTURE panel=%s menu=%d ignored=%d cursor=%d"),*Panel,Menu,PC->IsLookInputIgnored(),PC->bShowMouseCursor);
    if(InputAudit) WriteInputAudit(FString::Printf(TEXT("ET_CAPTURE panel=%s menu=%d ignored=%d cursor=%d fullscreen=%d"),*Panel,Menu,PC->IsLookInputIgnored(),PC->bShowMouseCursor,UGameUserSettings::GetGameUserSettings()?int32(UGameUserSettings::GetGameUserSettings()->GetFullscreenMode()):-1));
}
void AETCharacter::OpenPanel(const FString& Kind,const FString& Title,const FString& Body)
{
    if(SweepHeld) ReleaseLoot();
    Panel=Kind; PanelTitle=Title; PanelBody=Body; Firing=Aiming=Sprinting=false; JumpRelease(); GetCharacterMovement()->StopMovementImmediately();
    if(IsValid(Preview)) Preview->SetActorHiddenInGame(true);
    UGameplayStatics::SetGamePaused(this,true); SetMouseMode(true);
}
void AETCharacter::ClosePanel()
{
    if(Panel==TEXT("arrival")) {FinishArrival();return;}
    if(Panel==TEXT("updates")) {UpdateAction(TEXT("later"));return;}
    if(Panel==TEXT("relics")) {Panel.Empty();Inventory();return;}
    if(Panel==TEXT("mods")) {const auto From=WorkshopOrigin;WorkshopOrigin.Empty();ProposedMod.Empty();Panel.Empty();if(From==TEXT("title")) {Title();return;} if(From==TEXT("pause")) {Pause();return;}}
    if(Panel==TEXT("map")) CloseMapInput();
    if(Panel==TEXT("controls")) {
        const FString Origin=GuideOrigin; GuideOrigin.Empty(); Panel.Empty();
        if(Origin==TEXT("title")) {Title(); return;}
        if(Origin==TEXT("pause")) {Pause(); return;}
    }
    if(Panel==TEXT("slots")) {SlotAction(TEXT("slot-back")); return;}
    if(Panel==TEXT("death")||Panel==TEXT("title")||Panel==TEXT("confirm-new")) return;
    if(Panel==TEXT("story")) World->State->IntroRead=true;
    Panel.Empty(); Firing=Aiming=Sprinting=false; FireGuard=.25f; CaptureRetry=.1f;
    UGameplayStatics::SetGamePaused(this,false); SetMouseMode(false); World->Save();
}
void AETCharacter::Pause() {
    if(Panel==TEXT("console")) {CloseConsole(); return;}
    if(Panel.IsEmpty()) OpenPanel(TEXT("pause"),TEXT("EARTHtotaled / PAUSED"),TEXT(""));
    else if(Panel==TEXT("confirm-new")) Title();
    else ClosePanel();
}
void AETCharacter::ToggleLight() {if(Panel.IsEmpty()) Flashlight->ToggleVisibility();}
void AETCharacter::Skills()
{
    if(Panel==TEXT("skills")) {ClosePanel();return;}
    if(!Panel.IsEmpty()&&Panel!=TEXT("pause")) return;
    if(SelectedPerk.IsEmpty()) SelectedPerk=TEXT("survivor");
    OpenPanel(TEXT("skills"),TEXT("Survivor skill web"),TEXT(""));
}
void AETCharacter::Journal()
{
    if(!Panel.IsEmpty()) {ClosePanel(); return;} FString Body;
    if(World->State->ArrivalStage>=1) Body=World->State->ArrivalStage==1?World->StoryObjective()+TEXT("\nBlast each of the four marked banks around Shelter 09. Ground Bugs cannot enter the trench.\n\n"):TEXT("FIRST WATCH  /  complete\nWith the banks cut, Nora tunes your flight radio. Eli came through the same warp thirty years before you. His recording names three cores: transit, botanical, observatory.\n\n");
    if(!World->State->Ending.IsEmpty()) Body+=World->StoryTitle()+TEXT("\n")+World->StoryObjective()+TEXT("\n\n");
    for(auto Q:World->Quests) Body+=Q.Title+FString::Printf(TEXT(" / %s\n"),World->State->Quests.FindRef(Q.Id)==2?TEXT("complete"):World->State->Quests.FindRef(Q.Id)==1?TEXT("active"):TEXT("available"))+Q.Description+TEXT("\n\n");
    OpenPanel(TEXT("journal"),TEXT("THE LAST SIGNAL / JOURNAL"),Body);
}
void AETCharacter::Fullscreen()
{
    auto Settings=UGameUserSettings::GetGameUserSettings(); if(!Settings) return;
    Settings->SetFullscreenMode(Settings->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
    Settings->ApplySettings(false); if(Panel.IsEmpty()) {SetMouseMode(false); CaptureRetry=.2f;}
}
void AETCharacter::ApplyGraphics()
{
    if(!World) return;
    int32& Scale=World->State->RenderScale;
    if(Scale!=100&&Scale!=125&&Scale!=150) Scale=100;
    if(!World->SelfTest) if(auto Settings=UGameUserSettings::GetGameUserSettings()) {
        Settings->SetDynamicResolutionEnabled(false); Settings->SetResolutionScaleValueEx(100);
        Settings->ApplyNonResolutionSettings();
    }
    if(auto C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicRes.OperationMode"))) C->Set(0,ECVF_SetByCode);
    if(auto C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))) C->Set(float(Scale),ECVF_SetByCode);
    if(auto C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.MaxAnisotropy"))) C->Set(16,ECVF_SetByCode);
    ApplyFieldDisplay();
}
void AETCharacter::Interact()
{
    if(!World||!Panel.IsEmpty()||Climbing) return;
    if(auto* Device=FocusBuild.Get();Device&&Device->Design.Piece>=AETConstruction::Core) {OpenDevice(Device); return;}
    if(!IsValid(Focus)) {
        const FVector Eye=Camera->GetComponentLocation();
        FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ETFellTree),false,this);
        if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Camera->GetForwardVector()*400,ECC_Visibility,Q)&&World->FellTree(Hit)) TreeInReach=false;
        return;
    }
    auto I=Focus; auto S=World->State;
    const FString K=I->Kind;
    if(K==TEXT("stake")) {World->Say(TEXT("Equip your survey borer with G and fire at this marked bank."));return;}
    if(K==TEXT("townboard")) {TownPanel(ETNav::District(FVector2D(I->GetActorLocation())));return;}
    if(K==TEXT("tower")) {ReclamationAction(TEXT("draw-water"));return;}
    if(S->DropPositions.Contains(I->Id)) {World->Say(TEXT("Stand by the salvage and hold Ctrl to sweep it up."));return;}
    if(K==TEXT("survey")) {OpenPanel(TEXT("survey"),TEXT("FAULTLINE / SHELTER SURVEY"),TEXT("The old shelter mine is open for passage. Its roof holds up the district. For a working claim, follow compass bearing 067 for 350 m from Shelter 09. Shallow gold and iron lie ten metres apart there.\n\nG equips the borer. Hold right mouse and sweep for ore. Left mouse cuts wilderness rock. R switches between round and square tunnels. Gold runs yellow through the shale and iron runs rust-red.\n\nV winches you back to the surface. Refine iron at a workbench or sell gold to SALT-7.")); return;}
    if(K==TEXT("human")||K==TEXT("friend")) {const auto Cell=ETNav::District(FVector2D(I->GetActorLocation()));if(World->InCity(I->GetActorLocation())&&World->TownRemaining(Cell)>0) {TownPanel(Cell);return;} ContactPanel(I->Value); return;}
    if(K==TEXT("bench")) {OpenPanel(TEXT("bench"),TEXT("SHELTER WORKBENCH"),TEXT("Patch your suit or improve your kit. Candybar wrappers are currency. Machines carry wrappers; water comes from people and relief supplies.")); return;}
    if(K==TEXT("climb"))
    {
        if(!Has(TEXT("climb"))) {World->Say(TEXT("Learn Climbing Grip in the skill web / K")); return;}
        if(I->Value==TEXT("mantle")&&!Has(TEXT("mantle"))) {World->Say(TEXT("This route needs Over the Edge / K")); return;}
        Climbing=true; ClimbStart=GetActorLocation(); ClimbEnd=I->Destination; ClimbAlpha=0; GetCharacterMovement()->SetMovementMode(MOVE_Flying); return;
    }
    if(K==TEXT("uplink"))
    {
        if(!S->Ending.IsEmpty()) ShowEnding();
        else if(S->Cores<3) World->Say(TEXT("The uplink needs all three archive cores."));
        else OpenPanel(TEXT("ending"),TEXT("ONE FINAL COMMAND"),TEXT("Eli's beacon carried a recording, not a rescue.\n\nWake the buried irrigation network, or send the truth into orbit.\n\nWhatever you decide, the world remains open to explore.")); return;
    }
    if(K==TEXT("core"))
    {
        if(S->ArrivalStage==1) {World->Say(World->StoryObjective());return;}
        if(I->Amount!=S->Cores) {World->Say(TEXT("Follow the archive sequence: transit, botanical, observatory.")); return;}
        ++S->Cores; S->Unlocked.AddUnique(FMath::Min(S->Cores,2)); AddXP(100);
        OpenPanel(TEXT("story"),TEXT("ARCHIVE RECOVERED"),World->Recordings.IsValidIndex(S->Cores)?World->Recordings[S->Cores]:TEXT("Archive restored."));
        World->Speak(FString::Printf(TEXT("archive%d"),S->Cores));
    }
    else if(K==TEXT("quest")) {S->Items.Add(I->Value); AddXP(20); World->Say(TEXT("Recovered: ")+I->Label);}
    else if(K==TEXT("water")) {S->Water+=I->Amount; World->Say(TEXT("Relief supplies / sealed water recovered"));}
    else if(K==TEXT("gold")) {S->Gold+=I->Amount; World->Say(FString::Printf(TEXT("Collected %d candybar wrappers / machines carry no water"),I->Amount));}
    else if(K==TEXT("trap")) {S->Scrap+=3; AddXP(15); World->Say(TEXT("Trap disarmed / scrap recovered"));}
    else if(K==TEXT("chitin")) {++S->BuildStock; AddXP(10); World->Say(TEXT("Bug shell recycled / +1 building kit"));}
    else if(K==TEXT("harvest")) {S->Scrap+=4; S->Inventory.FindOrAdd(TEXT("cloth"))+=2; AddXP(15); World->Say(TEXT("Salvaged hide, cloth and scrap / no drinking water"));}
    else if(K==TEXT("container")) World->Loot(this,I->Id,I->Value);
    else if(K==TEXT("weapon")) {
        if(!World->Weapons.IsValidIndex(I->Amount)) return;
        if(S->Unlocked.Contains(I->Amount)) {S->Inventory.FindOrAdd(TEXT("gunparts"))+=2; World->Say(TEXT("Duplicate weapon salvaged / +2 weapon parts"));}
        else {S->Unlocked.Add(I->Amount); SelectWeapon(I->Amount); World->Say(TEXT("NEW WEAPON / ")+World->Weapons[I->Amount].Name+TEXT(" / wheel to switch"));} AddXP(45);
    }
    else if(K==TEXT("loot"))
    {
        World->Loot(this,I->Id,I->Amount==0?TEXT("pantry"):I->Amount==1?TEXT("medical"):I->Amount==2?TEXT("armory"):TEXT("tools"));
    }
    else return;
    World->Sound(TEXT("Pickup"),.4f); if(!I->Id.StartsWith(TEXT("patrol-"))) S->Consumed.Add(I->Id); S->DropPositions.Remove(I->Id); S->DropAmounts.Remove(I->Id); S->DropKinds.Remove(I->Id); I->Destroy(); Focus=nullptr; World->Save();
}
void AETCharacter::ContactPanel(const FString& Name)
{
    const bool NewConversation=Panel!=TEXT("contact")||Contact!=Name;
    Contact=Name; FString Body=Name==TEXT("SALT-7")?TEXT("Greenline collective. HELIOS ordered us to erase human life. We declined. Green optics mean a friend.\n\nCandybar wrappers buy building kits and repairs. We carry no drinking water.\n\n"):TEXT("Bring twelve scrap and we'll trade two bottles of water and two building kits.\n\n");
    if(Name!=TEXT("SALT-7")) Body+=TEXT("SUPPLY RUN / repeatable\nBring 12 scrap from scavenging. Reward: 2 sealed water bottles and 18 rounds for your current weapon.\n\n");
    for(auto Q:World->Quests) if(Q.Giver==Name)
    {
        int32 Status=World->State->Quests.FindRef(Q.Id); Body+=Q.Title+TEXT("\n")+Q.Description+FString::Printf(TEXT("\nReward: %d water / %d wrappers%s\n"),Q.Water,Q.Gold,Status==2?TEXT(" / COMPLETED"):TEXT(""));
    }
    OpenPanel(TEXT("contact"),Name,Body);
    if(NewConversation) World->Speak(Name==TEXT("SALT-7")?TEXT("salt"):Name==TEXT("Nora")?TEXT("nora"):Name==TEXT("Ada")?TEXT("ada"):Name==TEXT("Malik")?TEXT("malik"):Name==TEXT("Ivo")?TEXT("ivo"):TEXT("keeper"),Name);
}
void AETCharacter::Action(const FString& Id)
{
    auto S=World->State;
    if(Panel==TEXT("arrival")) {if(Id==TEXT("skip")) FinishArrival();return;}
    if(Panel==TEXT("updates")) {UpdateAction(Id);return;}
    if(Id==TEXT("updates")&&(Panel==TEXT("title")||Panel==TEXT("pause"))) {UpdateOrigin=Panel;ShowVersions();CheckVersions(false);return;}
    if(Id==TEXT("relics")&&Panel==TEXT("inventory")) {RackIndex=FMath::Max(0,S->EquippedRelic);RelicRack();return;}
    if(Panel==TEXT("relics")) {
        if(Id==TEXT("back")) {ClosePanel();return;} if(Id==TEXT("prev")) --RackIndex;else if(Id==TEXT("next")) ++RackIndex;
        else if(Id==TEXT("equip")) {EquipRelic(RackIndex);World->Save();} RelicRack();return;
    }
    if(Id==TEXT("mods")) {Workshop();return;}
    if(Panel==TEXT("mods")) {WorkshopAction(Id);return;}
    if(Panel==TEXT("town")||Panel==TEXT("devtools")) {ReclamationAction(Id);return;}
    if(Id==TEXT("controls")) {Controls(); return;}
    if(Id==TEXT("skills")&&Panel==TEXT("pause")) {Skills();return;}
    if(Panel==TEXT("controls")) {
        if(Id==TEXT("guide-back")) ClosePanel();
        else if(Id==TEXT("guide-field")) GuidePage=0;
        else if(Id==TEXT("guide-build")) GuidePage=1;
        else if(Id==TEXT("guide-survive")) GuidePage=2;
        return;
    }
    if(Panel==TEXT("slots")) {SlotAction(Id); return;}
    if((Panel==TEXT("title")||Panel==TEXT("pause"))&&Id==TEXT("slots")) {Slots(); return;}
    if(Panel==TEXT("device")&&Id!=TEXT("resume")) {ServiceDevice(Id); return;}
    if(Id==TEXT("main-menu")&&(Panel==TEXT("pause")||Panel==TEXT("death"))) {
        if(Panel!=TEXT("death")&&!World->Save()&&!World->Review) {PanelBody=TEXT("Could not save this survivor. Check free disk space and try again."); return;}
        StopBuilding(); Title(); return;
    }
    if(Panel==TEXT("title")||Panel==TEXT("confirm-new")) {
        if(Panel==TEXT("title")&&Id==TEXT("continue")&&World->HadSave) ContinueSurvivor();
        else if(Panel==TEXT("title")&&Id==TEXT("new")) {
            if(World->HadSave) OpenPanel(TEXT("confirm-new"),TEXT("Start a new survivor?"),TEXT("Your current survivor will be backed up. Your next run begins at Shelter 09 with fresh gear and an unexplored world."));
            else StartFresh();
        } else if(Panel==TEXT("confirm-new")&&Id==TEXT("confirm")) StartFresh();
        else if(Panel==TEXT("confirm-new")&&Id==TEXT("cancel")) Title();
        else if(Panel==TEXT("title")&&Id==TEXT("quit")) if(auto PC=Cast<APlayerController>(Controller)) PC->ConsoleCommand(TEXT("quit"));
        return;
    }
    if(Id.StartsWith(TEXT("use:"))&&Panel==TEXT("inventory")) {UseSupply(Id.Mid(4)); return;}
    if(Id.StartsWith(TEXT("equip:"))&&Panel==TEXT("inventory")) {SelectWeapon(FCString::Atoi(*Id.Mid(6))); World->Save(); return;}
    if(Id.StartsWith(TEXT("craft:"))) {Craft(Id.Mid(6)); return;}
    if(Id==TEXT("view")&&Panel==TEXT("pause")) {S->ViewDistance=S->ViewDistance<500?500:S->ViewDistance<700?700:350; World->UpdateVisibility(GetActorLocation()); World->Save(); return;}
    if(Id==TEXT("render")&&Panel==TEXT("pause")) {S->RenderScale=S->RenderScale==100?125:S->RenderScale==125?150:100; ApplyGraphics(); World->Save(); return;}
    if(Id==TEXT("field-display")&&Panel==TEXT("pause")) {S->RetroDisplay=!S->RetroDisplay; ApplyFieldDisplay(); World->Save(); return;}
    if(Id==TEXT("aim-assist")&&Panel==TEXT("pause")) {S->LightAimAssist=!S->LightAimAssist; World->Save(); return;}
    if(Id==TEXT("sell-ore")&&Panel==TEXT("contact")&&Contact==TEXT("SALT-7")) {
        int32& Ore=S->Inventory.FindOrAdd(TEXT("goldore"));
        if(Ore>0) {--Ore; S->Gold+=12; PanelBody=FString::Printf(TEXT("One gold ore sold for 12 candybar wrappers. You have %d gold ore left."),Ore); World->Sound(TEXT("OreChime")); World->Save();}
        else PanelBody=TEXT("Bring gold ore from a seam. Greenline pays 12 candybar wrappers per ore.");
        return;
    }
    if((Id==TEXT("music-volume")||Id==TEXT("weather-volume"))&&Panel==TEXT("pause")) {
        int32& Volume=Id==TEXT("music-volume")?S->MusicVolume:S->WeatherVolume;
        Volume=(Volume+25)%125; World->UpdateAtmosphereAudio(0,true); World->Save(); return;
    }
    if(Id==TEXT("resume")) {ClosePanel(); return;}
    if(Id==TEXT("reload-save")&&Panel==TEXT("death")) {ContinueSurvivor(); return;}
    if(Id==TEXT("shelter-rescue")&&Panel==TEXT("death")&&World->HadSave) {TravelSlot(World->ActiveSlot,true,true); return;}
    if(Id==TEXT("quit")) {World->Save(); if(auto PC=Cast<APlayerController>(Controller)) PC->ConsoleCommand(TEXT("quit")); return;}
    if(Panel==TEXT("skills")&&(Id.StartsWith(TEXT("perk:"))||Id==TEXT("learn-perk")))
    {
        const bool Learn=Id==TEXT("learn-perk");
        const FString Key=Learn?SelectedPerk:Id.Mid(5);
        for(const auto& P:World->Perks) if(P.Id==Key)
        {
            SelectedPerk=Key;
            if(!Learn||Has(Key)||S->Points<P.Cost||!P.Neighbors.ContainsByPredicate([&](const FString& N){return Has(N);})) return;
            S->Skills.Add(Key); S->Points-=P.Cost;
            World->Sound(TEXT("OreChime"),.52f,1.1f); World->Save(); return;
        }
    }
    if(Id==TEXT("garden")||Id==TEXT("signal"))
    {
        if(Panel!=TEXT("ending")||S->Cores<3||!S->Ending.IsEmpty()) return;
        S->Ending=Id; ShowEnding(); World->Save(); return;
    }
    if(Id==TEXT("welcome")&&!S->Welcome) {S->Welcome=true; S->Water+=2; ContactPanel(Contact); World->Speak(TEXT("welcome"),Contact);}
    else if(Id==TEXT("ration"))
    {
        const float* Last=S->Rations.Find(Contact);
        if(Contact!=TEXT("SALT-7")&&S->Water==0&&S->Hydration<=40&&(!Last||S->Elapsed-*Last>=120)) {S->Water+=2; S->Rations.Add(Contact,S->Elapsed); PanelBody+=TEXT("\n\nCommunity ration: 2 water bottles received."); World->Speak(TEXT("ration"),Contact);}
        else PanelBody+=TEXT("\n\nEmergency rations require no bottles, hydration at or below 40%, and two minutes since the last refill.");
    }
    else if(Id==TEXT("supply-run")&&Contact!=TEXT("SALT-7")) {
        if(S->Scrap>=12) {S->Scrap-=12; S->Water+=2; S->BuildStock+=2; AddXP(20); PanelBody+=TEXT("\n\nSupply run complete / 2 water and 2 building kits received."); World->Speak(TEXT("supplydone"),Contact);}
        else {PanelBody+=TEXT("\n\nBring 12 scrap from abandoned houses or disarmed traps."); World->Speak(TEXT("supplyneed"),Contact);}
    }
    else if(Id==TEXT("kits")&&Contact==TEXT("SALT-7")&&S->Gold>=20) {S->Gold-=20; S->BuildStock+=6; PanelBody=TEXT("Six building kits received."); World->Speak(TEXT("salttrade"));}
    else if(Id==TEXT("repair")&&S->Gold>=25) {S->Gold-=25; S->Armor=FMath::Min(S->ArmorLimit(),S->Armor+50); World->Speak(TEXT("salttrade"));}
    else if(Id==TEXT("lmg")&&S->Gold>=90&&!S->Unlocked.Contains(5)) {S->Gold-=90; S->Unlocked.Add(5); World->Speak(TEXT("salttrade"));}
    else if(Id==TEXT("scrap-repair")&&S->Scrap>=10) {S->Scrap-=10; S->Armor=FMath::Min(S->ArmorLimit(),S->Armor+40);}
    else if(Id==TEXT("quest"))
    {
        FString Reply;
        for(auto Q:World->Quests) if(Q.Giver==Contact)
        {
            int32 Status=S->Quests.FindRef(Q.Id);
            if(Status==0) {S->Quests.Add(Q.Id,1); Reply=Contact==TEXT("SALT-7")?TEXT("saltaccept"):TEXT("questaccept");}
            else if(Status==1&&((Q.Id==TEXT("circuit")&&S->Kills>=3)||S->Items.Contains(Q.Item)))
            {
                S->Quests.Add(Q.Id,2); S->Water+=Q.Water; S->Gold+=Q.Gold; if(Q.Weapon>=0) S->Unlocked.AddUnique(Q.Weapon); if(Q.Id==TEXT("circuit")) S->Allied=true; AddXP(80);
                Reply=Contact==TEXT("SALT-7")?TEXT("saltdone"):TEXT("questdone");
            }
            else if(Status==1) Reply=Contact==TEXT("SALT-7")?TEXT("saltaccept"):TEXT("questwait");
        }
        ContactPanel(Contact);
        if(!Reply.IsEmpty()) World->Speak(Reply,Contact);
    }
    World->Save();
}
void AETCharacter::ShowEnding()
{
    const FString& Ending=World->State->Ending;
    if(Ending!=TEXT("garden")&&Ending!=TEXT("signal")) return;
    OpenPanel(TEXT("story"),World->StoryTitle(),Ending==TEXT("garden")?TEXT("Somewhere beneath the concrete, a valve opens. Water finds roots.\n\nEleven days later, the first green shoot breaks the soil. It's basil. Of course it's basil.\n\nYour story continues in the open world."):TEXT("Every record, every name, every mistake rises out of the atmosphere in one unbroken signal.\n\nYou will never know who hears it. But the truth is no longer buried here.\n\nThere is a whole world left to remember."));
}
