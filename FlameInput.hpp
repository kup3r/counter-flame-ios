#pragma once

namespace FlameInput {
using Runtime::P;

struct State {
    P canShoot=nullptr,allowed=nullptr,shoot=nullptr,isGrenade=nullptr,isBomb=nullptr;
    P trace=nullptr,hitCollider=nullptr,hitActor=nullptr,forward=nullptr,openHitboxes=nullptr,maskField=nullptr;
    P findExit=nullptr,hitPoint=nullptr,hitTriangle=nullptr,surfaceGet=nullptr,isOpaque=nullptr;
    P weaponCurrent=nullptr,weaponId=nullptr,weaponRange=nullptr,powerGet=nullptr,lossGet=nullptr;
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
        P bulletSurface=cls("ScarletWork.Hit","BulletSurface");
        P penetration=cls("ScarletWork.Hit","BulletPenetration"),parameters=cls("","WeaponParameters");
        P idType=cls("","WeaponId");
        if(trace&&bulletSurface&&class_value_size(bulletSurface,&alignment)==8&&idType&&class_value_size(idType,&alignment)==1){
            findExit=scene.checked(extra,arms,"TryFindExit","System.Boolean",
                {"UnityEngine.RaycastHit","UnityEngine.Vector3","System.Int32","UnityEngine.RaycastHit&"});
            hitPoint=scene.checked(extra,rayHit,"get_point","UnityEngine.Vector3");
            hitTriangle=scene.checked(extra,rayHit,"get_triangleIndex","System.Int32");
            surfaceGet=scene.checked(extra,penetration,"Surface","ScarletWork.Hit.BulletSurface",
                {"UnityEngine.Collider","System.Int32","System.Boolean&"});
            isOpaque=scene.checked(extra,penetration,"Opaque","System.Boolean",{"ScarletWork.Hit.BulletSurface&"});
            weaponCurrent=scene.checked(extra,arms,"get_CurrentParamters","WeaponParameters");
            weaponId=scene.checked(extra,parameters,"get_WeaponId","WeaponId");
            weaponRange=scene.checked(extra,parameters,"get_Range","System.Single");
            powerGet=scene.checked(extra,penetration,"Power","System.Single",{"WeaponId"});
            lossGet=scene.checked(extra,penetration,"Loss","System.Single",
                {"System.Single","System.Single","ScarletWork.Hit.BulletSurface&","ScarletWork.Hit.BulletSurface&","System.Single"});
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
    bool surface(FlameScene::State& scene,P collider,P impact,uint64_t& result,bool& ok){
        int triangle=-1;bool mapped=false,opaque=true;
        if(!scene.value(hitTriangle,impact,nullptr,triangle,ok))return false;
        P lookup[]={collider,&triangle,&mapped};
        if(!scene.value(surfaceGet,nullptr,lookup,result,ok))return false;
        P args[]={&result};
        return scene.value(isOpaque,nullptr,args,opaque,ok)&&!opaque;
    }
    int behindSurface(Extra::State& extra,FlameScene::State& scene,P arms,P collider,V3 origin,V3 direction,int mask,bool& ok){
        using namespace Runtime;
        if(!weaponCurrent||!findExit)return -1;
        P parameters=invoke(weaponCurrent,arms,nullptr,ok);
        if(!extra.alive(parameters,ok))return -1;
        uint8_t id=0;float power=0,range=0;
        if(!scene.value(weaponId,parameters,nullptr,id,ok)||!scene.value(weaponRange,parameters,nullptr,range,ok)||
            !(range>0&&std::isfinite(range)))return -1;
        P powerArgs[]={&id};
        if(!scene.value(powerGet,nullptr,powerArgs,power,ok)||!(power>0&&std::isfinite(power)))return -1;
        uint64_t entranceSurface=0,exitSurface=0;
        if(!surface(scene,collider,hit,entranceSurface,ok))return -1;
        alignas(8) unsigned char exit[44]{};
        int target=0;
        P exitArgs[]={hit,&direction,&target,exit};
        bool found=false;
        if(!scene.value(findExit,arms,exitArgs,found,ok)||!found)return -1;
        P exitCollider=invoke(hitCollider,exit,nullptr,ok);
        if(!extra.alive(exitCollider,ok)||!surface(scene,exitCollider,exit,exitSurface,ok))return -1;
        V3 entryPoint{},exitPoint{};
        if(!scene.value(hitPoint,hit,nullptr,entryPoint,ok)||!scene.value(hitPoint,exit,nullptr,exitPoint,ok)||
            !finiteV3(entryPoint)||!finiteV3(exitPoint))return -1;
        float thickness=(exitPoint.x-entryPoint.x)*direction.x+(exitPoint.y-entryPoint.y)*direction.y+(exitPoint.z-entryPoint.z)*direction.z;
        if(!(thickness>.002f&&thickness<=2.25f))return -1;
        float cost=0,minimumDamage=20.f;
        P lossArgs[]={&minimumDamage,&power,&entranceSurface,&exitSurface,&thickness};
        if(!scene.value(lossGet,nullptr,lossArgs,cost,ok)||!(cost>=0&&cost<minimumDamage-1.f))return -1;
        float travelled=sqrtf((exitPoint.x-origin.x)*(exitPoint.x-origin.x)+(exitPoint.y-origin.y)*(exitPoint.y-origin.y)+(exitPoint.z-origin.z)*(exitPoint.z-origin.z));
        float left=std::min(75.f,range)-travelled-.025f;
        if(!(left>0))return -1;
        V3 beyond={exitPoint.x+.025f*direction.x,exitPoint.y+.025f*direction.y,exitPoint.z+.025f*direction.z};
        int query=0;bool collided=false;
        memset(hit,0,sizeof(hit));
        P traceArgs[]={&beyond,&direction,hit,&left,&mask,&query};
        if(!scene.value(trace,nullptr,traceArgs,collided,ok)||!collided)return -1;
        P targetCollider=invoke(hitCollider,hit,nullptr,ok);
        if(!extra.alive(targetCollider,ok))return -1;
        int actor=-1;P lookup[]={targetCollider};
        return scene.value(hitActor,arms,lookup,actor,ok)?actor:-1;
    }
    int actor(Extra::State& extra,FlameScene::State& scene,P arms,P camera,V3 origin,double now,bool direct,bool wallShot){
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
        if(actor>0)return direct?actor:-1;
        return wallShot?behindSurface(extra,scene,arms,collider,origin,direction,mask,ok):-1;
    }
    void fire(P arms,int actor,double now,double delay){
        if(actor!=lastActor){lastActor=actor;firstSeen=now;}
        if((now-firstSeen)*1000<delay)return;
        bool ok=true;Runtime::invoke(shoot,arms,nullptr,ok);
        if(!ok){shoot=nullptr;status=@"TriggerBot: вызов выстрела остановлен";}
    }
};
}
