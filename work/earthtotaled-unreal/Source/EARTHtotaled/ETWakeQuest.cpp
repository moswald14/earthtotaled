#include "ETWake.h"
#include "ETGame.h"
#include "ETWakeVehicle.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace ETWake;
namespace {
    const TCHAR* Banter[]={TEXT("Thrum: The juice doesn't make you less you. It just gives your body a new answer."),
        TEXT("SALT-7: The frame gives you a different answer. A reliable one. With menus."),
        TEXT("Thrum: That sounds boring."),TEXT("SALT-7: It is extremely useful.")};
    const TCHAR* SideName(int Side){return Side==Bug?TEXT("Bug"):TEXT("Supertech");}
    const TCHAR* RivalName(int Side){return SideName(Side==Bug?Frame:Bug);}
    bool Near(FVector A,FVector B,double Radius){return FVector::DistSquared(A,B)<Radius*Radius;}
    FString Outcome(const FETWakeSave& S) {
        const TCHAR* Land=S.EndingHerds==9?TEXT("All nine herds have come home. Their trails now connect every refuge."):
            S.EndingHerds>=7?TEXT("A chain of quiet glades reaches the far orchards. Fawns graze around the new shelter."):
            TEXT("The herds you saved graze around the crossing. Their hoofprints mark the first safe trails out of Shelter 09.");
        return FString::Printf(TEXT("The Sunseed flares over the empty Crown array. The pulse falls silent. Kit takes your hand. Across the road, a doe pauses, listens, and walks on.\n\n%s\n\n%s\n\n%d of 9 herds restored. Kit, Thrum and SALT-7 survived. Your vehicles, buildings and the open world are yours."),Land,
            S.Body==Bug?TEXT("Thrum starts a living nursery beside Shelter 09. SALT-7 brings the first load of soil in your roadster."):TEXT("SALT-7 opens a fabrication hall to rebuild the shelter. Thrum reviews its twelve menus and declares one of them tolerable."),S.EndingHerds);
    }
}
void AETCharacter::WakeMessage(FString Title,FString Body,const FString& Cue) {
    OpenPanel(TEXT("wake"),Title,Body);if(!Cue.IsEmpty())World->Speak(Cue);World->Save();
}
void AETCharacter::WakeJournal() {
    if(Panel=="wake"&&WakePanelNode=="journal"){ClosePanel();return;}
    auto& S=World->State->Wake;WakePanelNode="journal";WakeOffer=0;
    FString Body=Objective(S)+TEXT("\n\n");
    if(S.Chapter==Missing)Body+=TEXT("Mara Venn's son Kit vanished during the pulse. The shelter lists him among the missing. Thrum felt the deer repeat the same three steps. SALT-7 is reading their signal.");
    else if(S.Chapter<Dispatch)Body+=TEXT("The search for Kit begins on the evacuation road. Thrum hears MegaWAKE in the soil. SALT-7 is stripping its instructions out of the local machinery.");
    else if(S.Chapter<Runway)Body+=TEXT("Bug crews silence relays so the land can recover. Supertech crews retune them to preserve the rebuilding network. Both restore deer. Three completed jobs bring a warning. The fourth makes the other faction hostile. Shared work earns no faction marks.");
    else if(S.Chapter<Home)Body+=TEXT("fraudCORP funded the original Supertech program, supplied its machines and kept a backdoor. The company forged attack orders on both sides. Kit was taken alive as leverage. Thrum and SALT-7 are still with you.");
    else Body+=Outcome(S);
    if(S.TaskSide)Body+=FString::Printf(TEXT("\n\nActive herd: %s / %s method."),Herd(S.TaskHerd).Name,SideName(S.TaskSide));
    if(S.Chapter==FirstHerd)Body+=TEXT("\n\nField signs: inspect the paired prints at the crossing and the folded foil beside the bent ferns. E examines a clue.");
    if(S.Chapter==Runway||S.Chapter==Hangar)Body+=TEXT("\n\nAt Hollow Field, look for paw marks beside the south rock shelf. The hangar is beyond it.");
    if(S.Chapter==Cell)Body+=TEXT("\n\nCheck the lift grille beside Kit's cell for a folded square.");
    if(S.Chapter==Home&&S.TrailFound<(1<<ClueCount)-1)Body+=TEXT("\n\nMissing recordings can still be found at the shelter crossing, the quiet glade, Hollow Field and the custody lift.");
    if(S.Chapter>=Surveys&&S.Chapter<Home&&!S.TaskSide&&NextHerd(S))Body+=FString::Printf(TEXT("\n\nNext herd: %s. Optional restorations protect more ground and change the homecoming."),Herd(NextHerd(S)).Name);
    WakeMessage(S.Chapter==Home?TEXT("FIELD JOURNAL  /  KIT IS HOME"):TEXT("FIELD JOURNAL  /  FIND KIT"),Body);
}
void AETCharacter::WakeUse(FName Key) {
    auto& S=World->State->Wake;const auto* Object=World->WakeObjects.FindRef(Key);
    if(!IsValid(Object)||Object->IsHidden()||!Near(GetActorLocation(),Object->GetActorLocation(),600)||S.Seat!=Foot)return;
    WakePanelNode=Key;WakeOffer=0;
    const int ClueIndex=Key=="tracks"?Hooves:Key=="fernfold"?FernFold:Key=="paws"?PawSign:Key=="liftfold"?LiftFold:INDEX_NONE;
    if(ClueIndex!=INDEX_NONE){
        World->FindTrailClue(ClueIndex);ReadTrail(ClueIndex);return;
    }
    auto Advance=[&](int Chapter,const TCHAR* Title,const TCHAR* Text,const TCHAR* Cue=TEXT("")){
        S.Chapter=Chapter;World->RefreshWake();WakeMessage(Title,Text,Cue);
    };
    if(Key=="thrum"&&S.Chapter==Missing) {
        Advance(Triage,TEXT("THRUM  /  THE THREE STEPS"),TEXT("Kit was in the evacuation lane when the pulse hit. The shelter marked him lost.\n\nThrum: I heard every deer take the same three steps. Something told them there was a predator. There wasn't. Walk with me, Mara.\n\nSALT-7: I'll trace the relay. Look for Kit's wristband near the red medical case on the road."),TEXT("wakethrum"));return;
    }
    if(Key=="band"&&S.Chapter==Triage) {
        World->FindTrailClue(CaseFold);
        Advance(FirstHerd,TEXT("A WRISTBAND UNDER THE TARP"),TEXT("KIT VENN. The medical ink is dry. A second scanner stamp reads TRANSFER. Somebody moved him after the crash.\n\nA doe shies from an empty lane. Behind her, two more repeat the same turn. Meet Thrum at the roadblock."),TEXT("wakeband"));return;
    }
    if(Key=="herd"&&S.Chapter==FirstHerd) {
        S.SharedLead=true;WakeMessage(TEXT("THRUM  /  WALK THEM HOME"),TEXT("Stay near the deer and walk to the green glade marker. Give the last doe time to follow. Press E at the marker when two deer reach it.\n\nSALT-7 will hold the relay's carrier steady while Thrum guides their instincts back."),TEXT("wakeguide"));return;
    }
    const bool Shared=Key=="glade0"&&S.Chapter==FirstHerd&&S.SharedLead;
    const int I=Object->Amount;
    if(Shared||(S.TaskSide&&Key.ToString().StartsWith(TEXT("glade"))&&S.TaskHerd==I&&S.ReadSource)) {
        if(!Shared&&S.TaskSide==Frame){
            WakeMessage(TEXT("GLADE RECEIVER  /  SET CARRIER"),TEXT("Match the band SALT-7 read at the source. A matching carrier removes the ghost predator without shutting down the network. Your field journal retains the reading."));return;}
        int Gathered=0;for(auto* D:World->WakeDeer)if(IsValid(D)&&D->WakeHerd==I&&FVector2D::Distance(FVector2D(D->GetActorLocation()),Glade(I))<850)++Gathered;
        if(Gathered<2){World->Say(FString::Printf(TEXT("Thrum hears %d / 2 deer here. Walk back toward the herd, then lead them in."),Gathered));return;}
        if(Shared){S.SharedLead=false;AddXP(75);World->State->Water+=2;Advance(Garage,TEXT("THREE ORDINARY DEER"),TEXT("The doe stops repeating her turn. She tests the grass. The other two begin to graze. This glade is safe to camp and build in. Two sealed bottles and a clue arrive from the relief crew.\n\nThe transfer truck went east. SALT-7 has your car running beside the crossing. This shared rescue counted for neither faction."),TEXT("wakeherd"));}
        else FinishWakeHerd();return;
    }
    if(Key=="garage"){
        if(S.Chapter==FirstHerd){WakeMessage(TEXT("SALT-7 / UNDER THE HOOD"),TEXT("I've got the engine in pieces. Help Thrum get those deer off the road while I set the ignition."));return;}
        if(S.Chapter==Garage){Advance(Driving,TEXT("SALT-7  /  FIRST DRIVE"),TEXT("Carburetor clear. Road clear. The roadster is yours.\n\nStand beside it and press E to enter, R to start. W accelerates, A and D steer, S brakes, Space is the handbrake, Ctrl+S reverses. Hold S until slow and press E to exit.\n\nFollow the straight shelter road through the amber gate to East Dispatch. The crew is keeping this first drive clear."),TEXT("wakecar"));return;}
        if(S.HasCar()){World->WakeCar->ParkAt(0);World->Say(TEXT("SALT-7 parked your roadster in the lane. E beside it to enter."));World->Save();}return;
    }
    if(Key=="dispatch"&&S.Chapter==Driving&&S.DriveGate)S.Chapter=Dispatch;
    if(Key=="dispatch"&&S.Chapter==Dispatch) {
        Advance(Surveys,TEXT("DISPATCH  /  THE EMPTY MANIFEST"),TEXT("No death certificate. No return journey. An erased destination sits under Kit's transfer number. To read it, you need the relays along the road.\n\nThrum can sever their grip on the deer. SALT-7 can retune the same relays and keep their lights on. The two friends agree about Kit. Their factions disagree about what to save. Choose a field job here or in J."),TEXT("wakemanifest"));return;
    }
    if(Key.ToString().StartsWith(TEXT("source"))&&S.TaskSide&&I==S.TaskHerd) {
        S.ReadSource=true;
        if(S.TaskSide==Bug)WakeMessage(FString(Herd(I).Name)+TEXT("  /  SILENCE"),TEXT("Thrum lays an antenna against the earth. The herd turns toward your footsteps. Walk them to the marked glade and use E to sever the relay once they are clear."),TEXT("wakeguide"));
        else WakeMessage(FString(Herd(I).Name)+TEXT("  /  RETUNE"),FString::Printf(TEXT("SALT-7 reads the drifting carrier: %s. Walk to the receiver in the glade, press E and select that band. The old network can carry the herd's own instincts again."),Frequency(I)),TEXT("waketune"));return;
    }
    if(Key=="runway"&&S.Chapter==Runway){
        Advance(Hangar,TEXT("THRUM  /  HOLLOW FIELD"),TEXT("You pull the root mat clear together. Thrum hears the last taproot release the runway and coils it off the strip. SALT-7 is waiting beside the survey plane. Crown Field is too far across the ravines to reach Kit in time on foot."),TEXT("wakestrip"));return;
    }
    if(Key=="hangar"&&S.Chapter==Runway){WakeMessage(TEXT("SALT-7 / HOLLOW FIELD"),TEXT("The magneto is on the bench. Help Thrum clear the roots from the strip while I finish the repairs."));return;}
    if(Key=="hangar"&&S.Chapter==Hangar){
        S.Airfields|=1;Advance(Flight,TEXT("SALT-7  /  FIRST FLIGHT"),TEXT("Magneto repaired. The survey plane is yours. E boards, R starts, W builds runway speed. Hold Space above 47 km/h to lift off. A/D turn. Space climbs, Ctrl descends. Release W to cruise.\n\nFly east to Crown Field. The pulse towers stand beyond the broken towns. Line up over the lit strip, descend with Ctrl, brake with S below 68 km/h to land. Stop and press E. Both discovered airfields can call the plane."),TEXT("wakeflight"));return;
    }
    if((Key=="hangar"||Key=="field1")&&S.HasPlane()){
        const int FieldIndex=Key=="hangar"?0:1;if(S.Airfields&(1<<FieldIndex)){World->WakePlane->ParkAt(FieldIndex);World->Say(TEXT("Survey plane ready on this runway. E to board."));World->Save();}return;
    }
    if(Key=="vault"&&S.Chapter==Vault) {
        if(S.VaultLine<4){WakeMessage(TEXT("MEGAWAKE VAULT  /  TWO ANSWERS"),FString(TEXT("The floor jolts. A green jar glows in Thrum's claws. SALT-7 braces an empty frame beside the console. Kit's voice crackles over the custody channel.\n\n"))+Banter[S.VaultLine],FString::Printf(TEXT("wakevault%d"),S.VaultLine));}
        else WakeMessage(TEXT("TWO ANSWERS"),TEXT("Walk to the glowing jar or the empty frame. Inspect either offer before choosing. Both friends will help you bring Kit home."));return;
    }
    if((Key=="juice"||Key=="frame")&&S.Chapter==Vault) {
        if(S.VaultLine<4){World->Say(TEXT("Listen to both friends at the vault console first"));return;}
        WakeOffer=Key=="juice"?Bug:Frame;
        WakeMessage(Key=="juice"?TEXT("MORPHING JUICE  /  PERMANENT CHOICE"):TEXT("SUPERTECH FRAME  /  PERMANENT CHOICE"),
            Key=="juice"?TEXT("Drink and physically become a Bug. Your ordinary inventory will be sealed safely in this save. Water need ends. Use claws, grow living buildings and learn MOLT. You can still drive and fly. Your body will open a buried path to Kit."):
            TEXT("Enter the frame. Water need ends. Keep full access to your pack. Fabricate structures without material costs, unlock Supertech buildings and learn OVERRIDE. Your credentials will open the freight route to Kit. Thrum counts the menus."));return;
    }
    if(S.Chapter==Passage&&((S.Body==Bug&&Key=="burrow")||(S.Body==Frame&&Key=="gate"))){
        const auto Destination=World->WakeSpot("cell")+FVector(-215,S.Body==Bug?-155:155,97);
        SetActorLocation(Destination);if(Controller)Controller->SetControlRotation(FRotator(0,0,0));
        World->Sound(S.Body==Bug?TEXT("JetThrust"):TEXT("FieldLatch"));
        Advance(Cell,S.Body==Bug?TEXT("MOLT  /  UNDER THE WALL"):TEXT("OVERRIDE  /  FREIGHT ACCESS"),
            S.Body==Bug?TEXT("You fold your shell and follow a root seam under the custody wall. Thrum's tapping guides you up inside. Through the slats you hear Kit counting the seconds."):
            TEXT("Your frame signs a freight authorization. The corporate gate admits one extremely well-documented parent. SALT-7 opens the inner hatch. Kit looks up from the cot."));return;
    }
    if(Key=="kit"&&S.Chapter==Cell) {
        S.EscortPosition=Object->GetActorLocation();Advance(Escort,TEXT("KIT VENN"),TEXT("Mum? They told me you'd gone. I kept counting your three knocks.\n\nYou open the lock. Kit takes off the company wristband and follows you. Walk him to the evacuation beacon. If you get too far ahead, return for him."),TEXT("wakekit"));return;
    }
    if(Key=="evac"&&S.Chapter==Escort) {
        auto* Kit=World->WakeObjects.FindRef("kit");if(!Kit||!Near(Kit->GetActorLocation(),Object->GetActorLocation(),950)){World->Say(TEXT("Kit is still behind you. Bring him within sight of the beacon."));return;}
        AddXP(200);Advance(Strike,TEXT("KIT IS CLEAR"),TEXT("SALT-7 confirms Kit is aboard the evacuation lift. Thrum feels both his feet leave the ground. The workers are clear. Only fraudCORP's repeating relay remains. The Sunseed console is yours."),TEXT("wakesafe"));return;
    }
    if(Key=="strike"&&S.Chapter==Strike) {WakeMessage(TEXT("SUNSEED  /  FINAL CONFIRMATION"),TEXT("Kit and the crews are safe. Release the fictional Sunseed nuke against the abandoned MegaWAKE array. This ends the operation and records the herds you restored. You can still leave and help the remaining herds before committing."));return;}
    if(Key=="evac"&&S.Chapter==Home){WakeAction(TEXT("wake-homecoming"));return;}
    if(S.Chapter==Home&&Key=="homebug"){WakeMessage(TEXT("THRUM"),TEXT("Kit counted the does this morning. Each one stopped somewhere different. He wants to name the glades. I think we can let him."));return;}
    if(S.Chapter==Home&&Key=="homesalt"){WakeMessage(TEXT("SALT-7"),S.Body==Frame?TEXT("The first fabrication hall now belongs to the shelter. Kit made me remove eleven menus. Efficiency has improved."):TEXT("Thrum requested soil for the nursery. The roadster is full of it. So, regrettably, am I."));return;}
    if(Key=="home"&&S.Chapter==Home){WakeMessage(TEXT("THE MORNING AFTER"),Outcome(S),TEXT("wakehome"));return;}
    if(Key.ToString().StartsWith(TEXT("glade"))&&I>0&&((S.Silenced|S.Retuned)&(1<<(I-1)))){World->Say(TEXT("Normal deer trails / safe ground for a camp or building"));return;}
    WakeJournal();
}
void AETCharacter::FinishWakeHerd() {
    auto& S=World->State->Wake;const int Side=S.TaskSide,I=S.TaskHerd;if(!I||!Side||!S.ReadSource)return;
    const int Bit=1<<(I-1);if((S.Silenced|S.Retuned)&Bit)return;
    (Side==Bug?S.Silenced:S.Retuned)|=Bit;const int N=S.Count(Side);S.TaskSide=S.TaskHerd=0;S.ReadSource=false;
    if(S.Chapter==Surveys)World->FindTrailClue(Carrier);
    World->State->Gold+=24;World->State->Water+=2;AddXP(90);
    FString Text=FString::Printf(TEXT("The %s herd breaks its loop and grazes. Its glade is a safe building site, marked on your map. The relief crew pays 24 wrappers and two bottles.\n\n"),Herd(I).Name);
    if(S.Count(Bug)+S.Count(Frame)==2)Text+=TEXT("You recover the founding charter. fraudCORP created and funded the original Supertech program and supplied the machines. Its service backdoor can forge orders with either side's signature. SALT-7's community grew real lives around stolen machinery.\n\n");
    if(N==3){S.WarningSeen|=1<<(Side-1);Text+=FString::Printf(TEXT("FACTION WARNING / 3 of 4. Finish one more %s job and %s patrols will turn hostile. You may still help the other side. Thrum and SALT-7 stay with you."),SideName(Side),RivalName(Side));}
    if(N==4&&S.Chapter==Surveys){
        S.Chapter=Runway;if(Side==Bug&&World->State->RaidStage)World->EndRaid(false);
        Text+=FString::Printf(TEXT("%s command has declared you hostile. Your two friends remain with you.\n\nThe fourth relay opens a fraudCORP recording: 'Hold Kit Venn at Crown until Mara cooperates.' They staged attacks on Bugs and Supertechs to keep both factions fighting. Kit is alive. Reach Hollow Field and fly to him."),RivalName(Side));}
    if(S.Chapter>=Runway&&N!=4)Text+=TEXT(" Another safe trail will be waiting when Kit comes home.");
    WakePanelNode="report";World->RefreshWake();WakeMessage(N==3?TEXT("THIRD MARK  /  WARNING"):N==4?TEXT("THE CUSTODY RECORDING"):TEXT("HERD RESTORED"),Text,N==4?TEXT("wakeleverage"):N==3?TEXT("wakewarn"):TEXT("wakeherd"));
}
void AETCharacter::WakeAction(const FString& Id) {
    auto& S=World->State->Wake;
    if(Id=="wake-journal"){WakeJournal();return;}
    if(Id=="wake-homecoming"&&S.Chapter==Home&&(WakePanelNode=="blast"||WakePanelNode=="evac")){
        auto Landing=World->WakeSpot("home")+FVector(-245,0,96);World->StreamAround(Landing,true);SetActorLocation(Landing);
        if(Controller)Controller->SetControlRotation(FRotator::ZeroRotator);WakePanelNode="ending";
        WakeMessage(TEXT("THE MORNING AFTER  /  KIT IS HOME"),Outcome(S),TEXT("wakehome"));return;}
    if(Panel!="wake")return;
    if(Id=="trail-log"&&S.TrailFound){ReadTrail(FMath::FloorLog2(uint32(S.TrailFound)));return;}
    if(WakePanelNode=="trail"&&(Id=="trail-prev"||Id=="trail-next"||Id=="trail-replay")){
        int Page=TrailPage;
        if(Id!="trail-replay")do{Page=(Page+(Id=="trail-next"?1:ClueCount-1))%ClueCount;}while(!(S.TrailFound&(1<<Page)));
        ReadTrail(Page);return;
    }
    if(Id=="resume"){ClosePanel();return;}
    if(Id=="archives"){Panel.Empty();ArchiveJournal();return;}
    if(Id=="wake-next"&&WakePanelNode=="vault"&&S.Chapter==Vault&&S.VaultLine<4){++S.VaultLine;WakeUse("vault");return;}
    if(Id=="job-bug"||Id=="job-frame"){
        if(S.Chapter<Surveys||S.Chapter>=Home||S.TaskSide||!NextHerd(S))return;
        const int Side=Id=="job-bug"?Bug:Frame;if(S.Winner()&&S.Winner()!=Side)return;
        WakeOffer=Side;WakePanelNode="job";
        FString Info=FString::Printf(TEXT("%s at %s. %s"),SideName(Side),Herd(NextHerd(S)).Name,Side==Bug?TEXT("Guide the animals into a quiet glade and sever the relay."):TEXT("Read the failing source and tune the glade receiver to its surviving band."));
        if(S.Count(Side)==3)Info+=FString::Printf(TEXT("\n\nFOURTH MARK. Completing this job makes %s patrols hostile. You can still turn back before completing it."),RivalName(Side));WakeMessage(TEXT("FIELD DISPATCH"),Info);return;
    }
    if(Id=="accept"&&WakePanelNode=="job"&&WakeOffer&&!S.TaskSide&&NextHerd(S)&&(!S.Winner()||S.Winner()==WakeOffer)){
        S.TaskSide=WakeOffer;S.TaskHerd=NextHerd(S);S.ReadSource=false;ClosePanel();World->RefreshWake();World->Say(Objective(S));World->Save();return;
    }
    if(Id=="leave-job"&&WakePanelNode=="journal"&&S.TaskSide){S.TaskSide=S.TaskHerd=0;S.ReadSource=false;WakePanelNode=NAME_None;WakeJournal();return;}
    if(Id.StartsWith(TEXT("tune-"))&&S.TaskSide==Frame&&S.ReadSource&&WakePanelNode==FName(*FString::Printf(TEXT("glade%d"),S.TaskHerd))){
        if(Id.Mid(5)==Frequency(S.TaskHerd))FinishWakeHerd();else {PanelBody=TEXT("The receiver slips out of phase. The herd stays put. Try the band recorded in your journal: ")+FString(Frequency(S.TaskHerd));World->Sound(TEXT("Switch"));}return;
    }
    if(Id=="transform"&&S.Chapter==Vault&&S.VaultLine==4&&((WakePanelNode=="juice"&&WakeOffer==Bug)||(WakePanelNode=="frame"&&WakeOffer==Frame))){
        S.Body=WakeOffer;S.Chapter=Passage;Camera->SetRelativeLocation({0,0,72});World->SetWakePerks();World->State->Points+=2;StopBuilding();Borer=Firing=Aiming=false;ReloadTime=0;ApplyWakeBody();
        AddXP(120);World->RefreshWake();WakePanelNode="transformed";
        WakeMessage(S.Body==Bug?TEXT("MOLT  /  YOUR NEW BODY"):TEXT("OVERRIDE  /  FRAME ONLINE"),S.Body==Bug?
            TEXT("The jar empties. Your forearms split into amber claws. You hear Kit through the rock. Left mouse strikes. K opens MOLT; B grows your faction buildings. Your pack is stored. Your body needs no water. Find the root seam."):
            TEXT("The frame seals. Your suit's water alarm goes silent. B builds without spending materials. K opens OVERRIDE. Your pack and controls are yours. Thrum says the menus have menus. Find the freight gate."),S.Body==Bug?TEXT("wakemolt"):TEXT("wakeframe"));return;
    }
    if(Id=="sunseed"&&WakePanelNode=="strike"&&S.Chapter==Strike&&S.Rescued()){
        S.EndingHerds=S.Restored();S.Chapter=Home;S.TaskSide=S.TaskHerd=0;S.ReadSource=false;
        World->Sound(TEXT("WeatherThunder"),.85f,.52f);World->WakeFlash=3.5f;World->RefreshWake();
        ClosePanel();const FVector Array=World->WakeSpot("cell")+FVector(730,-950,420);
        if(Controller)Controller->SetControlRotation((Array-Camera->GetComponentLocation()).Rotation());AddXP(400);WakePanelNode="blast";
        World->Say(TEXT("Kit is safe. The Crown array has gone silent."));World->Save();return;
    }
}
TArray<TPair<FString,FString>> AETCharacter::WakeChoices() const {
    TArray<TPair<FString,FString>> Out;const auto& S=World->State->Wake;
    auto Add=[&](const TCHAR* Id,const TCHAR* Label){Out.Emplace(Id,Label);};
    if((WakePanelNode=="journal"||WakePanelNode=="dispatch"||WakePanelNode=="report")&&S.Chapter>=Surveys&&S.Chapter<Home&&!S.TaskSide&&NextHerd(S)){
        if(S.Winner()!=Frame)Add(TEXT("job-bug"),TEXT("Bug / silence a relay"));
        if(S.Winner()!=Bug)Add(TEXT("job-frame"),TEXT("Supertech / retune a relay"));}
    if(WakePanelNode=="journal"){if(S.TrailFound)Add(TEXT("trail-log"),TEXT("Kit's trail / field recordings"));if(S.TaskSide)Add(TEXT("leave-job"),TEXT("Return unfinished job"));Add(TEXT("archives"),TEXT("HELIOS and settlement jobs"));}
    if(WakePanelNode=="trail"){
        if(FMath::CountBits(uint32(S.TrailFound))>1){Add(TEXT("trail-prev"),TEXT("Earlier recording"));Add(TEXT("trail-next"),TEXT("Later recording"));}
        Add(TEXT("trail-replay"),TEXT("Play this recording"));Add(TEXT("wake-journal"),TEXT("Current objective"));
    }
    if(WakePanelNode=="job")Add(TEXT("accept"),S.Count(WakeOffer)==3?TEXT("Accept fourth-mark job"):TEXT("Accept field job"));
    if(WakePanelNode=="vault"&&S.Chapter==Vault&&S.VaultLine<4)Add(TEXT("wake-next"),S.VaultLine==3?TEXT("Examine the two offers"):TEXT("Listen"));
    if((WakePanelNode=="juice"||WakePanelNode=="frame")&&WakeOffer&&S.Chapter==Vault)Add(TEXT("transform"),WakeOffer==Bug?TEXT("Drink / become a Bug"):TEXT("Enter the Supertech frame"));
    if(WakePanelNode=="strike"&&S.Chapter==Strike)Add(TEXT("sunseed"),TEXT("Release Sunseed"));
    if(S.TaskSide==Frame&&S.ReadSource&&WakePanelNode==FName(*FString::Printf(TEXT("glade%d"),S.TaskHerd))){
        Add(TEXT("tune-Amber"),TEXT("Amber"));Add(TEXT("tune-Violet"),TEXT("Violet"));Add(TEXT("tune-Cyan"),TEXT("Cyan"));}
    Add(TEXT("resume"),TEXT("Return to the world"));return Out;
}
