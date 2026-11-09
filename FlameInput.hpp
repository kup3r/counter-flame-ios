#pragma once

namespace FlameInput {
using Runtime::P;

struct State {
    P canShoot=nullptr,allowed=nullptr,shoot=nullptr,isGrenade=nullptr,isBomb=nullptr;
    P trace=nullptr,hitCollider=nullptr,hitActor=nullptr,forward=nullptr,openHitboxes=nullptr,maskField=nullptr;
    P bunnyAuto=nullptr,bunnyEnabled=nullptr,holdRejumps=nullptr;
    void (*staticRead)(P,P)=nullptr;
    void (*staticWrite)(P,P)=nullptr;
    bool ready=false,bunnySaved=false,autoBefore=false,enabledBefore=false;
    alignas(8) unsigned char hit[64]{};
    double nextProbe=0,firstSeen=0;
    int lastActor=-1;
    NSString* status=@"";

    void init(Extra::State& extra,FlameScene::State& scene){
        using namespace Runtime;
        ready=true;
        P arms=cls("","ArmsController");
        P movement=cls("","PlayerMovement");
        P physics=cls("UnityEngine","Physics"),rayHit=cls("UnityEngine","RaycastHit");
        canShoot=scene.checked(extra,arms,"get_CanShoot","System.Boolean");
        allowed=scene.checked(extra,arms,"get_IsFireAllowed","System.Boolean");
        shoot=scene.checked(extra,arms,"Shoot","System.Void");
        isGrenade=scene.checked(extra,arms,"get_isGrenade","System.Boolean");
        isBomb=scene.checked(extra,arms,"get_isBomb","System.Boolean");
        hitActor=scene.checked(extra,arms,"GetHitTargetId","System.Int32",{"UnityEngine.Collider"});
        hitCollider=scene.checked(extra,rayHit,"get_collider","UnityEngine.Collider");
        forward=scene.checked(extra,cls("UnityEngine","Transform"),"get_forward","UnityEngine.Vector3");
        openHitboxes=scene.checked(extra,cls("ScarletWork.Game","PlayerSkeleton"),"OpenHitboxes","System.Void",
            {"UnityEngine.Vector3","UnityEngine.Vector3","System.Single"});
        uint32_t alignment=0;
        P layer=cls("UnityEngine","LayerMask"),query=cls("UnityEngine","QueryTriggerInteraction");
        if(layer&&class_value_size(layer,&alignment)==sizeof(int))maskField=extra.typedField(arms,"mask","UnityEngine.LayerMask");
        if(physics&&rayHit&&query&&class_value_size(query,&alignment)==sizeof(int)&&class_value_size(rayHit,&alignment)==44){
            P it=nullptr;
            while(P m=class_get_methods(physics,&it)){
                if(!strcmp(method_get_name(m),"Raycast")&&extra.returns(m,"System.Boolean")&&
                    extra.parametersMatch(m,{"UnityEngine.Vector3","UnityEngine.Vector3","UnityEngine.RaycastHit&",
                        "System.Single","System.Int32","UnityEngine.QueryTriggerInteraction"})){trace=m;break;}
            }
        }
        holdRejumps=scene.checked(extra,movement,"get_HoldRejumps","System.Boolean");
        void* h=dlopen("@rpath/UnityFramework.framework/UnityFramework",RTLD_LAZY|RTLD_NOLOAD);
        if(!h)return;
        staticRead=reinterpret_cast<decltype(staticRead)>(dlsym(h,"il2cpp_field_static_get_value"));
        staticWrite=reinterpret_cast<decltype(staticWrite)>(dlsym(h,"il2cpp_field_static_set_value"));
        dlclose(h);
        bunnyAuto=staticBool(extra,movement,"AutoBunnyHopping");
        bunnyEnabled=staticBool(extra,movement,"EnableBunnyHopping");
    }
    P staticBool(Extra::State& extra,P cls,const char* name){
        using namespace Runtime;
        P field=cls?extra.field(cls,name):nullptr;
        if(!field||(extra.field_flags(field)&0x50)!=0x10)return nullptr;
        char* type=type_get_name(extra.field_type(field));
        bool match=type&&!strcmp(type,"System.Boolean");
        if(type)free_(type);
        return match?field:nullptr;
    }
    bool bunny(FlameScene::State& scene){
        if(!staticRead||!staticWrite||!bunnyAuto||!bunnyEnabled||!holdRejumps)return false;
        bool autoNow=false,enabledNow=false;
        staticRead(bunnyAuto,&autoNow);staticRead(bunnyEnabled,&enabledNow);
        if(!bunnySaved||!autoNow)autoBefore=autoNow;
        if(!bunnySaved||!enabledNow)enabledBefore=enabledNow;
        bool on=true;
        if(!autoNow)staticWrite(bunnyAuto,&on);
        if(!enabledNow)staticWrite(bunnyEnabled,&on);
        bunnySaved=true;
        bool ok=true,held=false;
        return scene.value(holdRejumps,nullptr,nullptr,held,ok)&&held;
    }
    void resetTrigger(){lastActor=-1;firstSeen=0;}
    void clear(){
        if(bunnySaved){
            bool current=false;
            staticRead(bunnyAuto,&current);if(current)staticWrite(bunnyAuto,&autoBefore);
            staticRead(bunnyEnabled,&current);if(current)staticWrite(bunnyEnabled,&enabledBefore);
            bunnySaved=false;
        }
        resetTrigger();status=@"";
    }
    int actor(Extra::State& extra,FlameScene::State& scene,P arms,P camera,V3 origin,double now){
        using namespace Runtime;
        if(now<nextProbe)return -2;
        nextProbe=now+1./30;
        if(!camera||!trace||!maskField||!forward||!openHitboxes||!hitCollider||!hitActor||
            !canShoot||!allowed||!shoot||!isGrenade||!isBomb)return -1;
        bool ok=true,readyToShoot=false,blocked=false;
        if(!scene.value(canShoot,arms,nullptr,readyToShoot,ok)||!readyToShoot||
            !scene.value(allowed,arms,nullptr,readyToShoot,ok)||!readyToShoot||
            !scene.value(extra.isKnife,arms,nullptr,blocked,ok)||blocked||
            !scene.value(isGrenade,arms,nullptr,blocked,ok)||blocked||
            !scene.value(isBomb,arms,nullptr,blocked,ok)||blocked)return -1;
        int mask=0;extra.read(arms,maskField,&mask);
        V3 direction{};
        if(!mask||!scene.value(forward,camera,nullptr,direction,ok)||!finiteV3(direction))return -1;
        float distance=200;
        P openArgs[]={&origin,&direction,&distance};invoke(openHitboxes,nullptr,openArgs,ok);
        int query=0;
        memset(hit,0,sizeof(hit));
        P args[]={&origin,&direction,hit,&distance,&mask,&query};
        bool collided=false;
        if(!scene.value(trace,nullptr,args,collided,ok)||!collided)return -1;
        P collider=invoke(hitCollider,hit,nullptr,ok);
        if(!extra.alive(collider,ok))return -1;
        int actor=-1;P lookup[]={collider};
        if(!scene.value(hitActor,arms,lookup,actor,ok))return -1;
        return actor;
    }
    void fire(P arms,int actor,double now,double delay){
        if(actor!=lastActor){lastActor=actor;firstSeen=now;}
        if((now-firstSeen)*1000<delay)return;
        bool ok=true;Runtime::invoke(shoot,arms,nullptr,ok);
        if(!ok){shoot=nullptr;status=@"TriggerBot: вызов выстрела остановлен";}
    }
};
}
