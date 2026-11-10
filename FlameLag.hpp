#pragma once

namespace FlameLag {
using namespace Runtime;

struct State {
    bool enabled=false,initialized=false,changedSwitch=false;
    int milliseconds=120,originalLag=0,appliedLag=0;
    P networkClient=nullptr,peerGet=nullptr,settingsGet=nullptr,enabledGet=nullptr,enabledSet=nullptr,lagGet=nullptr,lagSet=nullptr;
    void (*staticRead)(P,P)=nullptr;
    uint32_t owner=0,settingsOwner=0;
    double nextTick=0;
    NSString* status=@"";

    void init(Extra::State& extra,FlameScene::State& c){
        initialized=true;
        P photon=cls("Photon.Pun","PhotonNetwork"),client=cls("Photon.Realtime","LoadBalancingClient");
        P peer=cls("ExitGames.Client.Photon","PhotonPeer"),settings=cls("ExitGames.Client.Photon","NetworkSimulationSet");
        networkClient=photon?extra.field(photon,"NetworkingClient"):nullptr;
        if(networkClient){
            char* type=type_get_name(extra.field_type(networkClient));
            bool valid=(extra.field_flags(networkClient)&0x10)&&type&&!strcmp(type,"Photon.Realtime.LoadBalancingClient");
            if(type)free_(type);
            if(!valid)networkClient=nullptr;
        }
        if(!c.initialized)c.init(extra);
        staticRead=c.staticRead;
        peerGet=c.checked(extra,client,"get_LoadBalancingPeer","Photon.Realtime.LoadBalancingPeer");
        settingsGet=c.checked(extra,peer,"get_NetworkSimulationSettings","ExitGames.Client.Photon.NetworkSimulationSet");
        enabledGet=c.checked(extra,peer,"get_IsSimulationEnabled","System.Boolean");
        enabledSet=c.checked(extra,peer,"set_IsSimulationEnabled","System.Void",{"System.Boolean"});
        lagGet=c.checked(extra,settings,"get_OutgoingLag","System.Int32");
        lagSet=c.checked(extra,settings,"set_OutgoingLag","System.Void",{"System.Int32"});
    }
    void clear(FlameScene::State& c){
        if(owner){
            P peer=gchandle_get_target(owner),settings=gchandle_get_target(settingsOwner);
            bool ok=true,on=false;
            int current=-1;
            if(settings&&c.value(lagGet,settings,nullptr,current,ok)&&current==appliedLag){
                P args[]={&originalLag};invoke(lagSet,settings,args,ok);
                if(peer&&changedSwitch&&c.value(enabledGet,peer,nullptr,on,ok)&&on){
                    bool off=false;P change[]={&off};invoke(enabledSet,peer,change,ok);
                }
            }
            gchandle_free(owner);gchandle_free(settingsOwner);
        }
        owner=settingsOwner=0;changedSwitch=false;nextTick=0;status=@"";
    }
    void tick(Extra::State& extra,FlameScene::State& c,FlameRoundPractice::State& practice,double now){
        if(!enabled)return;
        if(now<nextTick)return;
        if(!practice.initialized)practice.init(extra,c);
        bool ok=true,offline=true,host=false,privateRoom=false,creator=false,cheats=false;
        if(!c.value(practice.offline,nullptr,nullptr,offline,ok)||offline||
            !c.value(practice.master,nullptr,nullptr,host,ok)||!host||
            !c.value(practice.privateRoom,nullptr,nullptr,privateRoom,ok)||!privateRoom||
            !c.value(practice.roomCreator,nullptr,nullptr,creator,ok)||!creator||
            !c.value(practice.cheatsEnabled,nullptr,nullptr,cheats,ok)||!cheats){
            clear(c);nextTick=now+.25;status=@"Fake Lag: нужна своя закрытая онлайн-комната с читами";return;
        }
        nextTick=now+.25;
        if(!initialized)init(extra,c);
        if(!networkClient||!staticRead||!peerGet||!settingsGet||!enabledGet||!enabledSet||!lagGet||!lagSet){
            status=@"Fake Lag: API Photon недоступен";return;
        }
        P client=nullptr;staticRead(networkClient,&client);
        if(client&&roots)roots->hold(client);
        P peer=client?invoke(peerGet,client,nullptr,ok):nullptr;
        P settings=peer?invoke(settingsGet,peer,nullptr,ok):nullptr;
        if(owner&&(gchandle_get_target(owner)!=peer||gchandle_get_target(settingsOwner)!=settings))clear(c);
        if(!settings)return;
        int before=0;bool alreadyOn=false;
        if(!c.value(lagGet,settings,nullptr,before,ok)||!c.value(enabledGet,peer,nullptr,alreadyOn,ok))return;
        if(!owner){
            owner=gchandle_new(peer,true);settingsOwner=gchandle_new(settings,true);
            if(!owner||!settingsOwner){
                if(owner)gchandle_free(owner);
                if(settingsOwner)gchandle_free(settingsOwner);
                owner=settingsOwner=0;return;
            }
            originalLag=appliedLag=before;
        }else if(before!=appliedLag)originalLag=appliedLag=before;
        if(before!=milliseconds){
            int delay=milliseconds;
            P args[]={&delay};invoke(lagSet,settings,args,ok);
            if(ok)appliedLag=delay;
        }
        if(ok&&!alreadyOn){
            bool on=true;P args[]={&on};invoke(enabledSet,peer,args,ok);
            if(ok)changedSwitch=true;
        }
        int actual=-1;bool on=false;
        if(c.value(lagGet,settings,nullptr,actual,ok)&&c.value(enabledGet,peer,nullptr,on,ok)&&on)
            status=[NSString stringWithFormat:@"Photon: исходящая задержка %d мс",actual];
        else {clear(c);nextTick=now+.25;status=@"Fake Lag: настройка Photon не подтверждена";}
    }
};
}
