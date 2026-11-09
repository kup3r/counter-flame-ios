#pragma once
#include <algorithm>
#include "FlamePatch.hpp"
// Experimental controls: all addresses and field locations are resolved by IL2CPP.
namespace Extra {
using P=Runtime::P;
struct State : FlamePatch::Memory {
    bool ready=false;
    P (*field)(P,const char*)=nullptr;
    P (*field_type)(P)=nullptr;
    uint32_t (*field_flags)(P)=nullptr;
    void (*read)(P,P,P)=nullptr;
    void (*write)(P,P,P)=nullptr;
    P (*string_new)(const char*)=nullptr;
    P (*class_from_type)(P)=nullptr;
    bool (*class_is_value)(P)=nullptr;
    P arms=nullptr,lock=nullptr,current=nullptr,recoil=nullptr,isKnife=nullptr;
    P euler=nullptr,setEuler=nullptr,goTransform=nullptr,lookLimit=nullptr;
    P controlsClass=nullptr,shootButton=nullptr,pressed=nullptr;
    P fpsLocal=nullptr,parametersClass=nullptr,findAll=nullptr,weaponId=nullptr,loadParameters=nullptr,createWeapon=nullptr,setSkin=nullptr;
    P rotation=nullptr,playerBody=nullptr;
    FlamePatch::Store patches;
    uint32_t originalKnife=0,knifeArms=0;
    int expectedKnife=-1,lastActualKnife=-1;double verifyAfter=0;
    P movement=nullptr,speedGet=nullptr,ammoCurrent=nullptr,ammoConfig=nullptr,ammoMag=nullptr,ammoReserve=nullptr,ammoSet=nullptr;
    P nextSpread=nullptr,air=nullptr,fovGet=nullptr,fovSet=nullptr,unityAlive=nullptr,currentController=nullptr,equippedSkin=nullptr,displayName=nullptr;
    uint32_t fovOwner=0;float fovBefore=NAN,fovApplied=NAN;
    float spreadObserved=NAN,speedObserved=NAN;int ammoObserved=-1,ammoMaximum=-1;
    int observedWeapon=-1;NSString* observedName=@"Нет активного оружия";NSString* observedSkin=@"";
    NSString* expectedSkin=nil;
    struct SkinSave {uint32_t arms,parameters;int weapon;NSString* skin;};
    std::vector<SkinSave> skinHistory;
    NSString* status=@"Экспериментальные функции выключены";
    bool init(){
        using namespace Runtime;
        void* h=dlopen("@rpath/UnityFramework.framework/UnityFramework",RTLD_LAZY|RTLD_NOLOAD);if(!h)return false;
#define EXTRA_LOAD(v,n) v=reinterpret_cast<decltype(v)>(dlsym(h,"il2cpp_" n))
        EXTRA_LOAD(field,"class_get_field_from_name");EXTRA_LOAD(field_type,"field_get_type");EXTRA_LOAD(field_flags,"field_get_flags");
        EXTRA_LOAD(read,"field_get_value");EXTRA_LOAD(write,"field_set_value");EXTRA_LOAD(string_new,"string_new");
        EXTRA_LOAD(class_from_type,"class_from_type");EXTRA_LOAD(class_is_value,"class_is_valuetype");
#undef EXTRA_LOAD
        dlclose(h);if(!field||!field_type||!field_flags||!read||!write||!string_new||!class_from_type||!class_is_value)return false;
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
        if(!parametersMatch(createWeapon,{"WeaponParameters","System.String","StatTrak","System.Boolean","<reference>","System.Int32","System.Int32"}))createWeapon=nullptr;
        if(!parametersMatch(setSkin,{"System.String","StatTrak"}))setSkin=nullptr;
        patches.bind(this);
        initExtended();
        ready=arms&&current&&isKnife&&recoil&&weaponId;return ready;
    }
    bool parametersMatch(P method,std::initializer_list<const char*> expected){
        if(!method||Runtime::method_get_param_count(method)!=expected.size())return false;
        int i=0;for(const char* type:expected){P param=Runtime::method_get_param(method,i++);char* actual=Runtime::type_get_name(param);bool same=actual&&!strcmp(actual,type);
            if(!strcmp(type,"<reference>")){P cls=class_from_type(param);same=actual&&cls&&!class_is_value(cls)&&!strchr(actual,'&')&&!strchr(actual,'*');}
            if(actual)Runtime::free_(actual);if(!same)return false;}return true;
    }
    void initExtended(){
        using namespace Runtime;P pc=cls("","PlayerController"),ac=cls("","ArmsController"),am=cls("","Ammunition");
        movement=method(pc,"get_MovementController",0);speedGet=method(cls("","PlayerMovement"),"get_Speed",0);
        ammoCurrent=method(ac,"get_CurrentAmmunition",0);ammoConfig=method(parametersClass,"get_Ammunition",0);
        ammoMag=method(am,"get_MagazineCapacity",0);ammoReserve=method(am,"get_Capacity",0);ammoSet=method(am,"Set",2,"System.Int32");
        if(!parametersMatch(ammoSet,{"System.Int32","System.Int32"}))ammoSet=nullptr;
        nextSpread=method(ac,"get_NextShotSpread",0);air=method(parametersClass,"get_AirInaccuracy",0);
        P camera=cls("UnityEngine","Camera");fovGet=method(camera,"get_fieldOfView",0);fovSet=method(camera,"set_fieldOfView",1,"System.Single");
        unityAlive=method(cls("UnityEngine","Object"),"op_Implicit",1,"UnityEngine.Object");
        currentController=method(ac,"get_CurrentController",0);equippedSkin=method(cls("ScarletWork.Weapon","WeaponController"),"get_EquippedSkin",0);
        displayName=method(parametersClass,"get_DisplayName",0);
    }
    static NSString* text(P object){
        if(!object)return @"";int length=Runtime::string_length(object);if(length<0||length>2048)return @"";
        return [[NSString alloc]initWithCharacters:(const unichar*)Runtime::string_chars(object) length:length]?:@"";
    }
    static int shortValue(P box){return box?*static_cast<int16_t*>(Runtime::object_unbox(box)):-1;}
    bool alive(P object,bool& ok){if(!object||!unityAlive)return false;P args[]={object};P value=Runtime::invoke(unityAlive,nullptr,args,ok);return value&&*static_cast<bool*>(Runtime::object_unbox(value));}
    void noSpread(P ac,bool& ok){
        using namespace Runtime;P knife=invoke(isKnife,ac,nullptr,ok);if(!knife||*static_cast<bool*>(object_unbox(knife)))return;
        P p=invoke(current,ac,nullptr,ok);if(!p||!ok)return;
        P acClass=cls("","ArmsController");
        for(const char* f:{"_accuracyMult","_accuracyAdditive","_airPenalty"})savedFloat(ac,acClass,f,0,2);
        P rf=typedField(acClass,"_recoilData","RecoilData");P data=objectField(ac,rf);
        if(data){savedFloat(data,cls("","RecoilData"),"AngleDispertion",0,2);savedVector(data,cls("","RecoilData"),"RelativeDispersion",2);}
        savedFloat(p,parametersClass,"_pelletSpread",0,2);
        P ai=air?invoke(air,p,nullptr,ok):nullptr;
        if(ai){P aiClass=cls("","AirInaccuracy");for(const char* f:{"_stand","_jump","_land","_ladder"})savedVector(ai,aiClass,f,2);savedFloat(ai,aiClass,"_jumpInitial",0,2);}
        if(nextSpread)spreadObserved=scalar(invoke(nextSpread,ac,nullptr,ok));
        if(!std::isfinite(spreadObserved)){restoreSpread();status=@"No Spread: значение не подтверждено";}else status=[NSString stringWithFormat:@"No Spread: движок сообщает %.4f",spreadObserved];
    }
    void speed(P pc,float factor,bool& ok){
        using namespace Runtime;if(!movement||!speedGet)return;P controller=invoke(movement,pc,nullptr,ok);if(!controller)return;
        P f=typedField(cls("","PlayerMovement"),"_speed","System.Single");
        if(f&&patches.scale(controller,f,std::clamp(factor,1.0f,3.0f),3))speedObserved=scalar(invoke(speedGet,controller,nullptr,ok));
    }
    void unlimitedAmmo(P ac,bool& ok){
        using namespace Runtime;if(!ammoCurrent||!ammoConfig||!ammoSet||!ammoMag||!ammoReserve)return;
        P p=invoke(current,ac,nullptr,ok);if(!p)return;
        P config=invoke(ammoConfig,p,nullptr,ok),live=invoke(ammoCurrent,ac,nullptr,ok);
        if(!config||!live||config==live){status=@"Ammo: отдельный счётчик боезапаса не найден";return;}
        int magazine=shortValue(invoke(ammoMag,config,nullptr,ok)),reserve=shortValue(invoke(ammoReserve,config,nullptr,ok));
        int currentMag=shortValue(invoke(ammoMag,live,nullptr,ok)),currentReserve=shortValue(invoke(ammoReserve,live,nullptr,ok));
        if(!ok||magazine<=0||magazine>1000||reserve<0||currentMag<0||currentReserve<0)return;
        if(currentMag<magazine||currentReserve<reserve){int total=std::max(reserve,currentReserve);P args[]={&magazine,&total};invoke(ammoSet,live,args,ok);}
        ammoObserved=shortValue(invoke(ammoMag,live,nullptr,ok));ammoMaximum=magazine;
    }
    void fieldOfView(P camera,float value,bool& ok){
        using namespace Runtime;if(!fovGet||!fovSet||!alive(camera,ok))return;
        float now=scalar(invoke(fovGet,camera,nullptr,ok));if(!ok||!std::isfinite(now)||now<1||now>179)return;
        if(!fovOwner||gchandle_get_target(fovOwner)!=camera){restoreFov();fovOwner=gchandle_new(camera,true);fovBefore=now;fovApplied=NAN;}
        if(std::isfinite(fovApplied)&&fabsf(now-fovApplied)>0.01f)fovBefore=now;
        fovApplied=std::clamp(value,60.0f,120.0f);P args[]={&fovApplied};invoke(fovSet,camera,args,ok);
    }
    void restoreFov(){
        using namespace Runtime;if(!fovOwner)return;RootScope roots;bool ok=true;P camera=gchandle_get_target(fovOwner);
        if(alive(camera,ok)&&fovGet&&fovSet&&std::isfinite(fovBefore)){
            float now=scalar(invoke(fovGet,camera,nullptr,ok));if(ok&&std::isfinite(now)&&fabsf(now-fovApplied)<0.01f){P args[]={&fovBefore};invoke(fovSet,camera,args,ok);}
        }gchandle_free(fovOwner);fovOwner=0;fovBefore=NAN;fovApplied=NAN;
    }
    P parametersFor(int requested,NSString* name,P arrayGet,bool& ok){
        using namespace Runtime;if(requested<0||requested>255)return nullptr;
        P result=nullptr;if(findAll){P type=type_get_object(class_get_type(parametersClass));P args[]={type};P list=invoke(findAll,nullptr,args,ok);
            if(list&&array_length(list)<=4096)for(int i=0;i<(int)array_length(list)&&ok;i++){P idx[]={&i};P p=invoke(arrayGet,list,idx,ok);if(p&&id(invoke(weaponId,p,nullptr,ok))==requested){result=p;break;}}
        }
        if(!result&&ok&&loadParameters&&name.length){P key=string_new(name.UTF8String);if(roots)roots->hold(key);P args[]={key};result=invoke(loadParameters,nullptr,args,ok);}
        return result&&ok&&id(invoke(weaponId,result,nullptr,ok))==requested?result:nullptr;
    }
    void observe(P ac,bool& ok){
        using namespace Runtime;P p=invoke(current,ac,nullptr,ok);observedWeapon=p?id(invoke(weaponId,p,nullptr,ok)):-1;
        if(p&&displayName)observedName=text(invoke(displayName,p,nullptr,ok));
        P weapon=currentController?invoke(currentController,ac,nullptr,ok):nullptr;
        observedSkin=weapon&&equippedSkin?text(invoke(equippedSkin,weapon,nullptr,ok)):@"";
    }
    void rememberSkin(P ac,P parameters,int weapon){
        using namespace Runtime;for(const SkinSave& save:skinHistory)if(save.weapon==weapon&&gchandle_get_target(save.arms)==ac)return;
        skinHistory.push_back({gchandle_new(ac,true),gchandle_new(parameters,true),weapon,observedSkin});
    }
    void discardSkins(){for(const SkinSave& s:skinHistory){Runtime::gchandle_free(s.arms);Runtime::gchandle_free(s.parameters);}skinHistory.clear();expectedSkin=nil;}
    void skinAction(P ac,NSString* action,NSDictionary* weapon,NSString* key,P arrayGet,bool& ok){
        using namespace Runtime;observe(ac,ok);P p=invoke(current,ac,nullptr,ok);if(!p||!ok)return;
        int selected=[weapon[@"id"] intValue];
        if([action isEqualToString:@"model"]){
            P nowKnife=invoke(isKnife,ac,nullptr,ok);
            if(![weapon[@"category"] isEqualToString:@"knives"]||!nowKnife||!*static_cast<bool*>(object_unbox(nowKnife))){status=@"Замена модели доступна, когда нож в руках (CT/TR)";return;}
            P desired=parametersFor(selected,weapon[@"name"],arrayGet,ok);if(!desired||!createWeapon){status=@"Конфигурация выбранной модели не найдена";return;}
            if(!originalKnife||gchandle_get_target(knifeArms)!=ac){discardKnife();originalKnife=gchandle_new(p,true);knifeArms=gchandle_new(ac,true);}
            equip(ac,desired,ok);if(ok){expectedKnife=selected;verifyAfter=NSProcessInfo.processInfo.systemUptime+1;status=@"Выбранная конфигурация ножа передана игре";}return;
        }
        if([action isEqualToString:@"restoreModel"]){knife(ac,2,arrayGet,ok);return;}
        if([action isEqualToString:@"restoreSkin"]){
            for(const SkinSave& save:skinHistory)if(save.weapon==observedWeapon&&gchandle_get_target(save.arms)==ac){
                if(save.skin.length&&setSkin){P name=string_new(save.skin.UTF8String);if(roots)roots->hold(name);P args[]={name,nullptr};invoke(setSkin,ac,args,ok);expectedSkin=save.skin;verifyAfter=NSProcessInfo.processInfo.systemUptime+1;status=@"Исходный материал запрошен";}
                else status=@"Исходный скин был стандартным; восстанови модель или смени раунд";return;
            }status=@"Для текущего оружия ещё нет сохранённого материала";return;
        }
        if(![action isEqualToString:@"skin"]||!key.length)return;
        if(selected!=observedWeapon){status=@"Возьми в руки оружие, выбранное в каталоге";return;}
        if(!setSkin){status=@"Метод применения материала недоступен";return;}
        rememberSkin(ac,p,observedWeapon);P name=string_new(key.UTF8String);if(roots)roots->hold(name);P args[]={name,nullptr};invoke(setSkin,ac,args,ok);
        if(ok){expectedSkin=key;verifyAfter=NSProcessInfo.processInfo.systemUptime+1;status=@"Материал запрошен; ожидаю ответ движка";}
    }
    void verifySkin(P ac,bool& ok){
        if(!expectedSkin||NSProcessInfo.processInfo.systemUptime<verifyAfter)return;observe(ac,ok);
        if(ok)status=[observedSkin isEqualToString:expectedSkin]?@"Движок подтвердил ключ выбранного материала":[NSString stringWithFormat:@"Запрошен %@; движок сообщает %@",expectedSkin,observedSkin.length?observedSkin:@"стандартный"];
        expectedSkin=nil;
    }
    NSString* summary(){
        NSMutableArray* parts=[NSMutableArray array];
        if(std::isfinite(spreadObserved))[parts addObject:[NSString stringWithFormat:@"Spread %.4f",spreadObserved]];
        if(std::isfinite(speedObserved))[parts addObject:[NSString stringWithFormat:@"Speed %.2f",speedObserved]];
        if(ammoObserved>=0)[parts addObject:[NSString stringWithFormat:@"Ammo %d/%d",ammoObserved,ammoMaximum]];
        return [parts componentsJoinedByString:@"  ·  "];
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
    uint32_t retain(P p) override{return Runtime::gchandle_new(p,true);}
    P resolve(uint32_t h) override{return Runtime::gchandle_get_target(h);}
    void release(uint32_t h) override{Runtime::gchandle_free(h);}
    void readValue(P owner,P f,P value,size_t) override{read(owner,f,value);}
    void writeValue(P owner,P f,const void* value,size_t) override{write(owner,f,const_cast<void*>(value));}
    bool savedFloat(P owner,P cls,const char* name,float value,int group){
        if(!owner||!std::isfinite(value))return false;P f=typedField(cls,name,"System.Single");if(!f)return false;
        float before=NAN;read(owner,f,&before);if(!std::isfinite(before))return false;return patches.set(owner,f,&value,4,group);
    }
    void zeroSaved(P owner,P cls,const char* name){savedFloat(owner,cls,name,0,1);}
    bool savedVector(P owner,P cls,const char* name,int group){
        if(!owner)return false;P f=typedField(cls,name,"UnityEngine.Vector2");if(!f)return false;
        uint32_t align=0;P v=Runtime::cls("UnityEngine","Vector2");if(!v||Runtime::class_value_size(v,&align)!=8)return false;
        float before[2]={NAN,NAN};read(owner,f,before);if(!std::isfinite(before[0])||!std::isfinite(before[1]))return false;
        float zero[2]={0,0};return patches.set(owner,f,zero,8,group);
    }
    void zeroTransient(P owner,P cls,const char* name){if(!owner)return;P f=typedField(cls,name,"System.Single");if(f){float zero=0;write(owner,f,&zero);}}
    void restoreRecoil(){if(ready)patches.restore(1);}
    void restoreSpread(){if(ready)patches.restore(2);}
    void restoreSpeed(){if(ready)patches.restore(3);}
    void restoreGameplay(){if(ready)patches.restore();restoreFov();}
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

