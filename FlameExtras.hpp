#pragma once
#include <algorithm>
// Experimental controls: all addresses and field locations are resolved by IL2CPP.
namespace Extra {
using P=Runtime::P;
struct Snapshot {uint32_t object;P field;float before;};
struct State {
    bool ready=false;
    P (*field)(P,const char*)=nullptr;
    P (*field_type)(P)=nullptr;
    uint32_t (*field_flags)(P)=nullptr;
    void (*read)(P,P,P)=nullptr;
    void (*write)(P,P,P)=nullptr;
    P (*string_new)(const char*)=nullptr;
    P arms=nullptr,lock=nullptr,current=nullptr,recoil=nullptr,isKnife=nullptr;
    P euler=nullptr,setEuler=nullptr,goTransform=nullptr,lookLimit=nullptr;
    P controlsClass=nullptr,shootButton=nullptr,pressed=nullptr;
    P fpsLocal=nullptr,parametersClass=nullptr,findAll=nullptr,weaponId=nullptr,loadParameters=nullptr,createWeapon=nullptr,setSkin=nullptr;
    P rotation=nullptr,playerBody=nullptr;
    std::vector<Snapshot> snapshots;
    uint32_t originalKnife=0,knifeArms=0;
    int expectedKnife=-1,lastActualKnife=-1;double verifyAfter=0;
    NSString* status=@"Экспериментальные функции выключены";
    bool init(){
        using namespace Runtime;
        void* h=dlopen("@rpath/UnityFramework.framework/UnityFramework",RTLD_LAZY|RTLD_NOLOAD);if(!h)return false;
#define EXTRA_LOAD(v,n) v=reinterpret_cast<decltype(v)>(dlsym(h,"il2cpp_" n))
        EXTRA_LOAD(field,"class_get_field_from_name");EXTRA_LOAD(field_type,"field_get_type");EXTRA_LOAD(field_flags,"field_get_flags");
        EXTRA_LOAD(read,"field_get_value");EXTRA_LOAD(write,"field_set_value");EXTRA_LOAD(string_new,"string_new");
#undef EXTRA_LOAD
        dlclose(h);if(!field||!field_type||!field_flags||!read||!write||!string_new)return false;
        P pc=cls("","PlayerController"),ac=cls("","ArmsController"),pl=cls("ScarletWork.Game","PlayerLock");
        arms=method(pc,"get_ArmsController",0);lock=method(pc,"get_PlayerLock",0);
        current=method(ac,"get_CurrentParamters",0);isKnife=method(ac,"get_isKnife",0);
        parametersClass=cls("","WeaponParameters");recoil=method(parametersClass,"get_RecoilParameters",0);weaponId=method(parametersClass,"get_WeaponId",0);
        P transform=cls("UnityEngine","Transform");euler=method(transform,"get_eulerAngles",0);setEuler=method(transform,"set_eulerAngles",1,"UnityEngine.Vector3");
        goTransform=method(cls("UnityEngine","GameObject"),"get_transform",0);lookLimit=method(pl,"get_LookXLimit",0);
        rotation=typedField(pl,"rotationX","System.Single");playerBody=typedField(pl,"_playerBody","UnityEngine.GameObject");
        controlsClass=cls("","ControlsManager");shootButton=method(controlsClass,"get_ShootButton",0);
        pressed=method(cls("Axlebolt.Standoff.Controls","InteractableButton"),"get_IsPressed",0);
        fpsLocal=method(cls("ScarletWork.Game","PlayerFPSCamera"),"get_Local",0);
        findAll=method(cls("UnityEngine","Resources"),"FindObjectsOfTypeAll",1,"System.Type");
        loadParameters=method(cls("ScarletWork.Utility","ResourceUtility"),"LoadWeaponParameters",1,"System.String");
        createWeapon=method(ac,"CreateWeapon",7,"WeaponParameters");setSkin=method(ac,"SetSkin",2,"System.String");
        if(!parametersMatch(createWeapon,{"WeaponParameters","System.String","StatTrak","System.Boolean","System.Byte[]","System.Int32","System.Int32"}))createWeapon=nullptr;
        if(!parametersMatch(setSkin,{"System.String","StatTrak"}))setSkin=nullptr;
        ready=arms&&current&&isKnife&&recoil&&weaponId;return ready;
    }
    bool parametersMatch(P method,std::initializer_list<const char*> expected){
        if(!method||Runtime::method_get_param_count(method)!=expected.size())return false;
        int i=0;for(const char* type:expected){char* actual=Runtime::type_get_name(Runtime::method_get_param(method,i++));bool same=actual&&!strcmp(actual,type);if(actual)Runtime::free_(actual);if(!same)return false;}return true;
    }
    P typedField(P cls,const char* name,const char* expected){
        if(!cls)return nullptr;P f=field(cls,name);if(!f||(field_flags(f)&(0x10|0x40)))return nullptr;
        char* type=Runtime::type_get_name(field_type(f));bool same=type&&!strcmp(type,expected);if(type)Runtime::free_(type);return same?f:nullptr;
    }
    static float scalar(P box){float value=NAN;if(box)memcpy(&value,Runtime::object_unbox(box),4);return value;}
    static int id(P box){
        // WeaponId's underlying type in this supplied metadata is System.Byte.
        return box?*static_cast<uint8_t*>(Runtime::object_unbox(box)):-1;
    }
    P objectField(P owner,P f){P value=nullptr;if(owner&&f)read(owner,f,&value);if(value&&Runtime::roots)Runtime::roots->hold(value);return value;}
    void zeroSaved(P owner,P cls,const char* name){
        using namespace Runtime;if(!owner)return;P f=typedField(cls,name,"System.Single");if(!f)return;
        for(const Snapshot& s:snapshots)if(gchandle_get_target(s.object)==owner&&s.field==f){float zero=0;write(owner,f,&zero);return;}
        float before=NAN;read(owner,f,&before);if(!std::isfinite(before))return;
        snapshots.push_back({gchandle_new(owner,true),f,before});float zero=0;write(owner,f,&zero);
    }
    void zeroTransient(P owner,P cls,const char* name){if(!owner)return;P f=typedField(cls,name,"System.Single");if(f){float zero=0;write(owner,f,&zero);}}
    void restoreRecoil(){
        if(!ready)return;
        for(const Snapshot& s:snapshots){P object=Runtime::gchandle_get_target(s.object);if(object){float now=NAN;read(object,s.field,&now);if(now==0)write(object,s.field,(void*)&s.before);}Runtime::gchandle_free(s.object);}snapshots.clear();
    }
    void discardKnife(){if(originalKnife)Runtime::gchandle_free(originalKnife);if(knifeArms)Runtime::gchandle_free(knifeArms);originalKnife=0;knifeArms=0;expectedKnife=-1;lastActualKnife=-1;}
    void noRecoil(P ac,bool& ok){
        using namespace Runtime;P knife=invoke(isKnife,ac,nullptr,ok);if(!knife||*static_cast<bool*>(object_unbox(knife)))return;
        P p=invoke(current,ac,nullptr,ok);P rp=p?invoke(recoil,p,nullptr,ok):nullptr;if(!ok||!rp)return;
        P rc=cls("","RecoilParameters");
        for(const char* f:{"_horizontalRange","_verticalRange","cameraDeviationCoeff","_viewKickAngle"})zeroSaved(rp,rc,f);
        P armsClass=cls("","ArmsController");
        for(const char* f:{"_camRecoilV","_camRecoilH","_recoilBaseV","_recoilBaseH"})zeroTransient(ac,armsClass,f);
        if(fpsLocal){P fp=invoke(fpsLocal,nullptr,nullptr,ok);P fc=cls("ScarletWork.Game","PlayerFPSCamera");if(fp){
            for(const char* f:{"profileViewKick","punchShotPitch","kickBackPerShot"})zeroSaved(fp,fc,f);
            for(const char* f:{"_punch","_punchShot","_kickTargetZ"})zeroTransient(fp,fc,f);
        }}
        status=@"No Recoil: параметры применены (тест)";
    }
    bool fireHeld(P controls,bool& ok){
        if(!controls||!shootButton||!pressed)return false;
        P button=Runtime::invoke(shootButton,controls,nullptr,ok);P value=button?Runtime::invoke(pressed,button,nullptr,ok):nullptr;
        return value&&*static_cast<bool*>(Runtime::object_unbox(value));
    }
    static float angle(float value){value=fmodf(value+180,360);if(value<0)value+=360;return value-180;}
    void aim(P pc,P cameraTransform,V3 cameraPos,V3 target,float alpha,bool& ok){
        using namespace Runtime;
        if(!lock||!rotation||!playerBody||!euler||!setEuler||!goTransform){status=@"AIM: нужные методы недоступны";return;}
        P controller=invoke(lock,pc,nullptr,ok);if(!controller)return;
        P body=objectField(controller,playerBody);if(!body)return;P bodyTr=invoke(goTransform,body,nullptr,ok);if(!bodyTr)return;
        P cb=invoke(euler,cameraTransform,nullptr,ok),bb=invoke(euler,bodyTr,nullptr,ok);if(!cb||!bb||!ok)return;
        V3 cam,bodyAngles;memcpy(&cam,object_unbox(cb),sizeof(cam));memcpy(&bodyAngles,object_unbox(bb),sizeof(bodyAngles));
        float pitch=NAN;read(controller,rotation,&pitch);if(!std::isfinite(pitch)||!finiteV3(cam)||!finiteV3(bodyAngles))return;
        double dx=target.x-cameraPos.x,dy=target.y-cameraPos.y,dz=target.z-cameraPos.z;
        double horizontal=hypot(dx,dz);if(horizontal<0.001)return;
        float wantYaw=atan2(dx,dz)*180/M_PI,wantPitch=-atan2(dy,horizontal)*180/M_PI;
        float camPitch=angle(cam.x);
        // Check the internal pitch convention against the current camera before writing it.
        float sign=fabsf(pitch-camPitch)<=fabsf(pitch+camPitch)?1:-1;
        if(fminf(fabsf(pitch-camPitch),fabsf(pitch+camPitch))>12){status=@"AIM: система углов не подтверждена";return;}
        float limit=lookLimit?scalar(invoke(lookLimit,controller,nullptr,ok)):89;
        if(!std::isfinite(limit)||limit<1||limit>90)limit=89;
        float next=std::clamp(pitch+sign*angle(wantPitch-camPitch)*alpha,-limit,limit);
        bodyAngles.y+=angle(wantYaw-cam.y)*alpha;
        if(!ok)return;write(controller,rotation,&next);P args[]={&bodyAngles};invoke(setEuler,bodyTr,args,ok);
        status=@"AIM: наведение при удержании огня (тест)";
    }
    P butterfly(P arrayGet,bool& ok){
        using namespace Runtime;P result=nullptr;
        if(findAll){P type=type_get_object(class_get_type(parametersClass));P args[]={type};P list=invoke(findAll,nullptr,args,ok);
            if(list&&array_length(list)<=4096)for(int i=0;i<(int)array_length(list)&&ok;i++){
                P idx[]={&i};P p=invoke(arrayGet,list,idx,ok);if(p&&id(invoke(weaponId,p,nullptr,ok))==8){result=p;break;}
            }
        }
        if(!result&&ok&&loadParameters){P name=string_new("Butterfly");if(roots)roots->hold(name);P args[]={name};result=invoke(loadParameters,nullptr,args,ok);}
        if(!result||!ok||id(invoke(weaponId,result,nullptr,ok))!=8)return nullptr;return result;
    }
    void equip(P ac,P parameters,bool& ok){
        bool pickup=false;int seed=0,owner=0;
        P args[]={parameters,nullptr,nullptr,&pickup,nullptr,&seed,&owner};
        Runtime::invoke(createWeapon,ac,args,ok);
    }
    void knife(P ac,int action,P arrayGet,bool& ok){
        using namespace Runtime;
        if(!createWeapon){status=@"Нож: сигнатура CreateWeapon не подтверждена";return;}
        P knifeNow=invoke(isKnife,ac,nullptr,ok);
        if(!knifeNow||!*static_cast<bool*>(object_unbox(knifeNow))){status=@"Сначала возьми нож в руки";return;}
        P currentParams=invoke(current,ac,nullptr,ok);if(!currentParams)return;
        if(action==2){
            if(!originalKnife||gchandle_get_target(knifeArms)!=ac){status=@"Нет сохранённого ножа для этого персонажа";return;}
            P original=gchandle_get_target(originalKnife);int originalId=id(invoke(weaponId,original,nullptr,ok));equip(ac,original,ok);if(ok){discardKnife();expectedKnife=originalId;verifyAfter=NSProcessInfo.processInfo.systemUptime+1;status=@"Исходная конфигурация ножа передана игре";}return;
        }
        if(action==3||action==4){
            if(!setSkin||id(invoke(weaponId,currentParams,nullptr,ok))!=8){status=@"Сначала примени Butterfly и возьми её в руки";return;}
            const char* key=action==3?"butterfly_vanilla":"butterfly_fade";
            P name=string_new(key);if(roots)roots->hold(name);P args[]={name,nullptr};invoke(setSkin,ac,args,ok);
            if(ok)status=action==3?@"Запрошен материал Butterfly Vanilla (тест)":@"Запрошен материал Butterfly Fade (тест)";return;
        }
        P desired=butterfly(arrayGet,ok);if(!desired){status=@"Конфигурация Butterfly не загружена; замена отменена";return;}
        if(!originalKnife||gchandle_get_target(knifeArms)!=ac){discardKnife();originalKnife=gchandle_new(currentParams,true);knifeArms=gchandle_new(ac,true);}
        equip(ac,desired,ok);if(ok){expectedKnife=8;verifyAfter=NSProcessInfo.processInfo.systemUptime+1;status=@"Конфигурация Butterfly (ID 8) передана игре";}
    }
    void verifyKnife(P ac,bool& ok){
        if(expectedKnife<0||NSProcessInfo.processInfo.systemUptime<verifyAfter)return;
        P p=Runtime::invoke(current,ac,nullptr,ok);int actual=p?id(Runtime::invoke(weaponId,p,nullptr,ok)):-1;
        lastActualKnife=actual;
        if(ok)status=actual==expectedKnife?[NSString stringWithFormat:@"Текущий ID %d. Проверь модель и анимации ножа",actual]:[NSString stringWithFormat:@"Ожидался ID %d, сейчас %d. Переключись на нож",expectedKnife,actual];
        expectedKnife=-1;
    }
};
}

