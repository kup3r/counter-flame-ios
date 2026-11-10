#pragma once

namespace FlameRoundPractice {
using Runtime::P;

enum class Action {None,Plant,Defuse,Win};

struct State {
    static constexpr int ActiveRound=4,RoundEnd=5,BombPlanted=6;
    P master=nullptr,offline=nullptr,room=nullptr,clock=nullptr,playerCount=nullptr;
    P privateRoom=nullptr,roomCreator=nullptr,cheatsEnabled=nullptr;
    P find=nullptr,matchType=nullptr,bombManagerType=nullptr,phase=nullptr,round=nullptr,requestPhase=nullptr;
    P ground=nullptr,position=nullptr,rotation=nullptr,transform=nullptr;
    P owner=nullptr,isPlanted=nullptr,plantAt=nullptr,announcePlant=nullptr,plantedBomb=nullptr,bombActive=nullptr,bombTime=nullptr;
    P markDetonated=nullptr,defuseBomb=nullptr,showDefused=nullptr,defusedFlag=nullptr,defuserActor=nullptr,bomberActor=nullptr;
    P resolveGround=nullptr,hitPoint=nullptr;
    bool initialized=false,enabled=false;
    Action pending=Action::None;
    int seenRound=-1,attemptedRound=-1;
    double nextTick=0,winAfter=0;
    NSString* status=@"";

    void init(Extra::State& extra,FlameScene::State& c){
        using namespace Runtime;
        initialized=true;
        P photon=cls("Photon.Pun","PhotonNetwork"),r=cls("Photon.Realtime","Room");
        P gate=cls("ScarletWork.Game.Console","ConsoleGate");
        master=c.checked(extra,photon,"get_IsMasterClient","System.Boolean");
        offline=c.checked(extra,photon,"get_OfflineMode","System.Boolean");
        room=c.checked(extra,photon,"get_CurrentRoom","Photon.Realtime.Room");
        clock=c.checked(extra,photon,"get_Time","System.Double");
        playerCount=c.checked(extra,r,"get_PlayerCount","System.Int32");
        privateRoom=c.checked(extra,gate,"get_IsPrivateRoom","System.Boolean");
        roomCreator=c.checked(extra,gate,"get_IsRoomCreator","System.Boolean");
        cheatsEnabled=c.checked(extra,gate,"get_CheatsEnabled","System.Boolean");
        find=c.checked(extra,cls("UnityEngine","Object"),"FindObjectOfType","UnityEngine.Object",{"System.Type"});
        P match=cls("ScarletWork.Game.Match","MatchStateMachine"),bombs=cls("","BombManager"),bomb=cls("","Bomb");
        P pm=cls("","PlayerMovement"),tr=cls("UnityEngine","Transform");
        if(match)matchType=class_get_type(match);
        if(bombs)bombManagerType=class_get_type(bombs);
        P phaseClass=cls("ScarletWork.Game","MatchState");
        uint32_t alignment=0;
        if(phaseClass&&class_value_size(phaseClass,&alignment)==sizeof(int)){
            phase=c.checked(extra,match,"get_Current","ScarletWork.Game.MatchState");
            requestPhase=c.checked(extra,match,"RequestPhase","System.Void",{"ScarletWork.Game.MatchState"});
        }
        round=c.checked(extra,cls("ScarletWork.Game.Match","MatchPhaseProps"),"get_Round","System.Int32");
        ground=c.checked(extra,pm,"get_IsGround","System.Boolean");
        position=c.checked(extra,tr,"get_position","UnityEngine.Vector3");
        P quaternion=cls("UnityEngine","Quaternion");
        if(quaternion&&class_value_size(quaternion,&alignment)==4*sizeof(float))
            rotation=c.checked(extra,tr,"get_rotation","UnityEngine.Quaternion");
        transform=c.checked(extra,cls("UnityEngine","Component"),"get_transform","UnityEngine.Transform");
        owner=c.checked(extra,bombs,"get_BombOwnerId","System.Int32");
        isPlanted=c.checked(extra,bombs,"IsBombPlanted","System.Boolean");
        plantAt=c.checked(extra,bombs,"PlantBombAt","System.Void",{"System.Int32","UnityEngine.Vector3","UnityEngine.Quaternion","System.Double"});
        announcePlant=c.checked(extra,bombs,"OnBombPlanted","System.Void",{"System.Int32"});
        plantedBomb=c.checked(extra,bombs,"get_PlantedBomb","Bomb");
        bombActive=c.checked(extra,bomb,"get_IsActive","System.Boolean");
        bombTime=c.checked(extra,cls("ScarletWork.Game.Defuse","DefuseController"),"get_BombTime","System.Single");
        markDetonated=c.checked(extra,bombs,"MarkDetonated","System.Void");
        defuseBomb=c.checked(extra,bomb,"Defuse","System.Void");
        showDefused=c.checked(extra,bomb,"ShowDefused","System.Void");
        defusedFlag=extra.typedField(bombs,"_defusedThisRound","System.Boolean");
        defuserActor=extra.typedField(bombs,"_finalDefuserActorNumber","System.Int32");
        bomberActor=extra.typedField(bombs,"_finalBomberActorNumber","System.Int32");
        P hit=cls("UnityEngine","RaycastHit");
        if(hit&&class_value_size(hit,&alignment)==44){
            resolveGround=c.checked(extra,bombs,"TryResolveGround","System.Boolean",{"UnityEngine.Vector3","UnityEngine.RaycastHit&"});
            hitPoint=c.checked(extra,hit,"get_point","UnityEngine.Vector3");
        }
    }
    bool gate(FlameScene::State& c){
        using namespace Runtime;
        bool ok=true,isMaster=false,isOffline=false,privateGame=false,creator=false,cheats=false;
        if(!room||!c.value(master,nullptr,nullptr,isMaster,ok)||!isMaster)return false;
        P current=invoke(room,nullptr,nullptr,ok);
        int count=0;
        if(!current||!c.value(playerCount,current,nullptr,count,ok)||count!=1)return false;
        if(c.value(offline,nullptr,nullptr,isOffline,ok)&&isOffline)return true;
        return c.value(privateRoom,nullptr,nullptr,privateGame,ok)&&privateGame&&
            c.value(roomCreator,nullptr,nullptr,creator,ok)&&creator&&c.value(cheatsEnabled,nullptr,nullptr,cheats,ok)&&cheats;
    }
    P locate(P type,bool& ok){
        using namespace Runtime;
        if(!type||!find)return nullptr;
        P boxed=type_get_object(type);if(boxed&&roots)roots->hold(boxed);
        P args[]={boxed};return boxed?invoke(find,nullptr,args,ok):nullptr;
    }
    void clear(){
        pending=Action::None;seenRound=attemptedRound=-1;nextTick=winAfter=0;status=@"";
    }
    void queue(Action action){pending=action;nextTick=0;}
    bool phaseTo(FlameScene::State& c,P machine,int target,bool& ok){
        P args[]={&target};
        Runtime::invoke(requestPhase,machine,args,ok);
        int actual=-1;
        return c.value(phase,machine,nullptr,actual,ok)&&actual==target;
    }
    bool practicePlant(Extra::State& extra,FlameScene::State& c,P machine,P manager,P player,int actor,bool& ok){
        using namespace Runtime;
        if(!plantAt||!announcePlant||!extra.movement||!transform)return false;
        int carrier=-1;bool planted=false,onGround=false;
        if(!c.value(owner,manager,nullptr,carrier,ok)||carrier!=actor){
            status=@"Plant Anywhere: подбери C4";return false;
        }
        if(!c.value(isPlanted,manager,nullptr,planted,ok)||planted)return false;
        P movement=invoke(extra.movement,player,nullptr,ok);
        if(!movement||!c.value(ground,movement,nullptr,onGround,ok)||!onGround){
            status=@"Plant Anywhere: встань на землю";return false;
        }
        P origin=invoke(transform,player,nullptr,ok);
        V3 point{};std::array<float,4> quaternion{};
        double time=0;
        if(!origin||!c.value(position,origin,nullptr,point,ok)||!finiteV3(point)||
            !c.value(rotation,origin,nullptr,quaternion,ok)||
            !std::all_of(quaternion.begin(),quaternion.end(),[](float q){return std::isfinite(q);})||
            !c.value(clock,nullptr,nullptr,time,ok)||!std::isfinite(time))return false;
        // The normal phase duration comes from the same DefuseController configuration.
        P controllerClass=cls("ScarletWork.Game.Defuse","DefuseController");
        P controller=controllerClass?locate(class_get_type(controllerClass),ok):nullptr;
        float seconds=0;
        if(!controller||!c.value(bombTime,controller,nullptr,seconds,ok)||seconds<5||seconds>180)return false;
        if(!resolveGround||!hitPoint)return false;
        alignas(8) unsigned char hit[44]{};
        bool floor=false;P traceArgs[]={&point,hit};
        if(!c.value(resolveGround,manager,traceArgs,floor,ok)||!floor||
            !c.value(hitPoint,hit,nullptr,point,ok)||!finiteV3(point)){
            status=@"Plant Anywhere: поверхность под игроком не найдена";return false;
        }
        point.y+=.002f;double ends=time+seconds;
        P args[]={&actor,&point,&quaternion,&ends};
        invoke(plantAt,manager,args,ok);
        if(!c.value(isPlanted,manager,nullptr,planted,ok)||!planted)return false;
        P announceArgs[]={&actor};invoke(announcePlant,manager,announceArgs,ok);
        return phaseTo(c,machine,BombPlanted,ok);
    }
    bool practiceWin(Extra::State& extra,FlameScene::State& c,P machine,P manager,int actor,bool asCT,bool bombRequired,bool& ok){
        using namespace Runtime;
        if(!plantedBomb||!defuseBomb||!showDefused||!defusedFlag||!defuserActor||!bomberActor||!markDetonated)return false;
        P bomb=invoke(plantedBomb,manager,nullptr,ok);
        bool active=false;
        if(extra.alive(bomb,ok))c.value(bombActive,bomb,nullptr,active,ok);
        if(!ok||(bombRequired&&!active))return false;
        if(active){
            invoke(defuseBomb,bomb,nullptr,ok);
            if(asCT&&ok)invoke(showDefused,bomb,nullptr,ok);
        }
        if(!ok)return false;
        if(asCT){
            bool defused=true;
            extra.write(manager,defusedFlag,&defused);extra.write(manager,defuserActor,&actor);
        }else{
            if(!active)extra.write(manager,bomberActor,&actor);
            invoke(markDetonated,manager,nullptr,ok);
        }
        return phaseTo(c,machine,RoundEnd,ok);
    }
    void tick(Extra::State& extra,FlameScene::State& c,P player,int actor,NSString* team,double now){
        if((!enabled&&pending==Action::None)||now<nextTick)return;
        nextTick=now+.20;
        if(!initialized)init(extra,c);
        Action action=pending;pending=Action::None;
        if(!gate(c)){
            status=@"Раунд: только одиночная тренировка или своя комната с читами";
            seenRound=attemptedRound=-1;return;
        }
        if(!player||actor<=0||(![team isEqual:@"ct"]&&![team isEqual:@"tr"])){
            if(action!=Action::None)status=@"Раунд: требуется живой игрок CT или TR";
            return;
        }
        bool ok=true;int current=-1,roundNumber=-1;
        P machine=locate(matchType,ok),manager=locate(bombManagerType,ok);
        if(!extra.alive(machine,ok)||!extra.alive(manager,ok)||!requestPhase||
            !c.value(phase,machine,nullptr,current,ok)||!c.value(round,nullptr,nullptr,roundNumber,ok)){
            status=@"Раунд: открой матч с бомбой";return;
        }
        if(current!=ActiveRound&&current!=BombPlanted){
            seenRound=-1;
            if(action!=Action::None)status=@"Раунд: дождись активной фазы";
            return;
        }
        if(seenRound!=roundNumber){seenRound=roundNumber;winAfter=now+.8;}
        if(action==Action::None&&enabled&&roundNumber!=attemptedRound&&now>=winAfter)action=Action::Win;
        if(action==Action::None)return;
        bool asCT=[team isEqual:@"ct"],confirmed=false;
        status=@"";
        if(action==Action::Plant){
            if(asCT||current!=ActiveRound){status=@"Plant Anywhere: только TR в активном раунде";return;}
            confirmed=practicePlant(extra,c,machine,manager,player,actor,ok);
        }else if(action==Action::Defuse){
            if(!asCT||current!=BombPlanted){status=@"Defuse Anywhere: только CT с заложенной C4";return;}
            confirmed=practiceWin(extra,c,machine,manager,actor,true,true,ok);
        }else{
            attemptedRound=roundNumber;
            confirmed=practiceWin(extra,c,machine,manager,actor,asCT,false,ok);
        }
        if(confirmed)status=action==Action::Plant?@"C4 заложена":action==Action::Defuse?@"C4 обезврежена":@"Раунд завершён";
        else if(!status.length)status=@"Раунд: действие отправлено, переход ещё не подтверждён";
    }
};
}
