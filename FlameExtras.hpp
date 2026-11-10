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
    P (*object_new)(P)=nullptr;
    P (*array_new)(P,uintptr_t)=nullptr;
    P (*value_box)(P,P)=nullptr;
    P statTrakClass=nullptr,statTrakCtor=nullptr;
    P (*method_return_type)(P)=nullptr;
    bool (*class_is_value)(P)=nullptr;
    P arms=nullptr,lock=nullptr,current=nullptr,recoil=nullptr,isKnife=nullptr;
    P euler=nullptr,setEuler=nullptr,goTransform=nullptr,lookLimit=nullptr;
    P controlsClass=nullptr,shootButton=nullptr,pressed=nullptr;
    P fpsLocal=nullptr,parametersClass=nullptr,findAll=nullptr,weaponId=nullptr,loadParameters=nullptr,createWeapon=nullptr,setSkin=nullptr;
    P rotation=nullptr,playerBody=nullptr;
    FlamePatch::Store patches;
    uint32_t originalKnife=0,cosmeticArms=0,cosmeticWeapon=0;
    int attemptedKnife=-1,expectedKnife=-1;
    double verifyKnifeAfter=0,verifyKnifeDeadline=0,verifySkinAfter=0;
    NSString* originalKnifeSkin=nil;
    NSDictionary* desiredKnife=nil;
    NSMutableDictionary<NSNumber*,NSString*>* desiredSkins=nil;
    NSString* attemptedSkin=nil;
    int skinWeapon=-1;
    bool skinVerified=false;
    bool knifeMaterialFallback=false,knifeMaterialTried=false;
    P movement=nullptr,speedGet=nullptr,horizontalGet=nullptr,speedField=nullptr,movementTuning=nullptr,maxSpeedField=nullptr;
    P ammoConfig=nullptr,ammoMag=nullptr,ammoReserve=nullptr,ammoSet=nullptr,liveAmmo=nullptr,showAmmo=nullptr,magField=nullptr,reserveField=nullptr;
    P nextSpread=nullptr,air=nullptr,fovGet=nullptr,fovSet=nullptr,unityAlive=nullptr,currentController=nullptr,equippedSkin=nullptr,displayName=nullptr;
    uint32_t fovOwner=0;float fovBefore=NAN,fovApplied=NAN;
    float spreadBefore=NAN,spreadObserved=NAN,speedObserved=NAN,horizontalObserved=NAN;int spreadFields=0;
    uint32_t spreadParameters=0;
    bool spreadVector=false;
    double spreadSampleAfter=0;int ammoObserved=-1,ammoMaximum=-1;
    int observedWeapon=-1;NSString* observedName=@"Нет активного оружия";NSString* observedSkin=@"";
    NSString* expectedSkin=nil;
    struct SkinSave {uint32_t parameters;int weapon;NSString* skin;};
    std::vector<SkinSave> skinHistory;
    NSString* status=@"Экспериментальные функции выключены";
    bool init(){
        using namespace Runtime;
        void* h=dlopen("@rpath/UnityFramework.framework/UnityFramework",RTLD_LAZY|RTLD_NOLOAD);if(!h)return false;
#define EXTRA_LOAD(v,n) v=reinterpret_cast<decltype(v)>(dlsym(h,"il2cpp_" n))
        EXTRA_LOAD(field,"class_get_field_from_name");EXTRA_LOAD(field_type,"field_get_type");EXTRA_LOAD(field_flags,"field_get_flags");
        EXTRA_LOAD(read,"field_get_value");EXTRA_LOAD(write,"field_set_value");EXTRA_LOAD(string_new,"string_new");
        EXTRA_LOAD(method_return_type,"method_get_return_type");
        EXTRA_LOAD(object_new,"object_new");EXTRA_LOAD(array_new,"array_new");EXTRA_LOAD(value_box,"value_box");
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
        statTrakClass=cls("","StatTrak");
        statTrakCtor=method(statTrakClass,".ctor",2,"System.Boolean");
        if(!returns(statTrakCtor,"System.Void")||!parametersMatch(statTrakCtor,{"System.Boolean","System.Int32"}))statTrakCtor=nullptr;
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
    bool returns(P method,const char* expected){
        if(!method||!method_return_type)return false;
        P type=method_return_type(method);
        char* name=type?Runtime::type_get_name(type):nullptr;
        bool same=name&&!strcmp(name,expected);
        if(name)Runtime::free_(name);
        return same;
    }
    void initExtended(){
        using namespace Runtime;P pc=cls("","PlayerController"),ac=cls("","ArmsController"),am=cls("","Ammunition");
        P pm=cls("","PlayerMovement"),wc=cls("ScarletWork.Weapon","WeaponController");
        movement=method(pc,"get_MovementController",0);
        speedGet=method(pm,"get_MaxPlayerSpeed",0);horizontalGet=method(pm,"get_HorizontalSpeed",0);
        speedField=typedField(pm,"_speed","System.Single");
        movementTuning=typedField(pm,"_movement","SourceMovementTuning");
        maxSpeedField=typedField(cls("","SourceMovementTuning"),"MaxSpeed","System.Single");
        ammoConfig=method(parametersClass,"get_Ammunition",0);
        liveAmmo=method(wc,"get_Ammunation",0);showAmmo=method(wc,"ShowAmmo",1,"System.Int32");
        magField=typedField(ac,"MagazineCapacity","System.Int16");
        reserveField=typedField(ac,"Capacity","System.Int16");
        ammoMag=method(am,"get_MagazineCapacity",0);ammoReserve=method(am,"get_Capacity",0);ammoSet=method(am,"Set",2,"System.Int32");
        if(!parametersMatch(ammoSet,{"System.Int32","System.Int32"}))ammoSet=nullptr;
        nextSpread=method(ac,"get_NextShotSpread",0);
        uint32_t alignment=0;
        P vector=cls("UnityEngine","Vector2");
        spreadVector=returns(nextSpread,"UnityEngine.Vector2")&&vector&&class_value_size(vector,&alignment)==8;
        if(!spreadVector&&!returns(nextSpread,"System.Single"))nextSpread=nullptr;
        air=method(parametersClass,"get_AirInaccuracy",0);
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
    float sampleSpread(P ac){
        bool ok=true;
        P box=Runtime::invoke(nextSpread,ac,nullptr,ok);
        if(!ok||!box)return NAN;
        if(!spreadVector)return scalar(box);
        float value[2];
        memcpy(value,Runtime::object_unbox(box),sizeof(value));
        return hypotf(value[0],value[1]);
    }
    void noSpread(P ac,double now,bool& ok){
        using namespace Runtime;
        P knife=invoke(isKnife,ac,nullptr,ok);
        if(!knife||*static_cast<bool*>(object_unbox(knife))){restoreSpread();return;}
        P p=invoke(current,ac,nullptr,ok);
        if(!p||!ok){restoreSpread();return;}
        P acClass=cls("","ArmsController");
        if(!spreadParameters||gchandle_get_target(spreadParameters)!=p){
            restoreSpread();spreadParameters=gchandle_new(p,true);
            if(nextSpread)spreadBefore=sampleSpread(ac);
        }
        int changed=0;
        for(const char* f:{"_accuracyMult","_accuracyAdditive","_airPenalty"})changed+=savedFloat(ac,acClass,f,0,2);
        P recoilClass=cls("","RecoilData");
        P rf=recoilClass&&!class_is_value(recoilClass)?typedField(acClass,"_recoilData","RecoilData"):nullptr;
        if(P data=objectField(ac,rf)){
            changed+=savedFloat(data,recoilClass,"AngleDispertion",0,2);
            changed+=savedVector(data,recoilClass,"RelativeDispersion",2);
        }
        changed+=savedFloat(p,parametersClass,"_pelletSpread",0,2);
        P aiClass=cls("","AirInaccuracy");
        if(air&&aiClass&&!class_is_value(aiClass)&&returns(air,"AirInaccuracy")){
            if(P ai=invoke(air,p,nullptr,ok)){
                for(const char* f:{"_stand","_jump","_land","_ladder"})changed+=savedVector(ai,aiClass,f,2);
                changed+=savedFloat(ai,aiClass,"_jumpInitial",0,2);
            }
        }
        spreadFields=changed;
        if(nextSpread&&changed&&now>=spreadSampleAfter){
            spreadObserved=sampleSpread(ac);spreadSampleAfter=now+.25;
        }
    }
    void speed(P pc,float factor,bool& ok){
        using namespace Runtime;
        if(!movement||!speedField||!movementTuning||!maxSpeedField||!returns(speedGet,"System.Single"))return;
        P controller=invoke(movement,pc,nullptr,ok);
        P tuning=controller?objectField(controller,movementTuning):nullptr;
        if(!tuning||!ok)return;
        factor=std::clamp(factor,1.0f,3.0f);
        if(!patches.scale(controller,speedField,factor,3)||!patches.scale(tuning,maxSpeedField,factor,3)){
            restoreSpeed();return;
        }
        speedObserved=scalar(invoke(speedGet,controller,nullptr,ok));
        if(returns(horizontalGet,"System.Single"))horizontalObserved=scalar(invoke(horizontalGet,controller,nullptr,ok));
    }
    bool ammoCounts(P ac,int16_t& magazine,int16_t& reserve){
        if(!magField||!reserveField)return false;
        read(ac,magField,&magazine);read(ac,reserveField,&reserve);
        return magazine>=0&&reserve>=0;
    }
    bool syncAmmo(P ac,P config,int16_t magazine,int16_t reserve,bool& ok){
        using namespace Runtime;
        if(!magField||!reserveField||!ammoSet||!returns(liveAmmo,"Ammunition"))return false;
        P weapon=currentController?invoke(currentController,ac,nullptr,ok):nullptr;
        P live=weapon?invoke(liveAmmo,weapon,nullptr,ok):nullptr;
        if(!live||live==config||!ok)return false;
        int mag=magazine,total=reserve;P args[]={&mag,&total};
        invoke(ammoSet,live,args,ok);
        if(!ok)return false;
        write(ac,magField,&magazine);write(ac,reserveField,&reserve);
        if(returns(showAmmo,"System.Void")){
            bool modelOK=true;P displayArgs[]={&mag};invoke(showAmmo,weapon,displayArgs,modelOK);
        }
        return true;
    }
    void unlimitedAmmo(P ac,bool& ok){
        using namespace Runtime;
        ammoObserved=-1;
        if(!ammoConfig||!ammoMag||!ammoReserve)return;
        P p=invoke(current,ac,nullptr,ok);
        P config=p?invoke(ammoConfig,p,nullptr,ok):nullptr;
        if(!config||!ok)return;
        int magazine=shortValue(invoke(ammoMag,config,nullptr,ok)),reserve=shortValue(invoke(ammoReserve,config,nullptr,ok));
        int16_t currentMag=0,currentReserve=0;
        if(!ok||magazine<=0||magazine>1000||reserve<0||!ammoCounts(ac,currentMag,currentReserve))return;
        if(currentMag<magazine||currentReserve<reserve){
            if(!syncAmmo(ac,config,std::max<int>(currentMag,magazine),std::max<int>(currentReserve,reserve),ok))return;
        }
        if(ammoCounts(ac,currentMag,currentReserve)){ammoObserved=currentMag;ammoMaximum=magazine;}
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
    void resetCosmetics(){
        using namespace Runtime;
        for(const SkinSave& save:skinHistory)gchandle_free(save.parameters);
        skinHistory.clear();
        if(originalKnife)gchandle_free(originalKnife);
        if(cosmeticArms)gchandle_free(cosmeticArms);
        if(cosmeticWeapon)gchandle_free(cosmeticWeapon);
        originalKnife=cosmeticArms=cosmeticWeapon=0;
        attemptedKnife=expectedKnife=-1;
        knifeMaterialFallback=knifeMaterialTried=false;
        attemptedSkin=expectedSkin=originalKnifeSkin=nil;
        observedWeapon=skinWeapon=-1;
        observedName=observedSkin=@"";
        skinVerified=false;
    }
    void rememberSkin(P parameters){
        for(const SkinSave& save:skinHistory)if(save.weapon==observedWeapon)return;
        skinHistory.push_back({Runtime::gchandle_new(parameters,true),observedWeapon,observedSkin});
    }
    P newStatTrak(bool& ok){
        using namespace Runtime;
        if(!statTrakCtor||!object_new)return nullptr;
        P data=object_new(statTrakClass);
        if(!data)return nullptr;
        if(roots)roots->hold(data);
        bool enabled=false;int kills=0;
        P args[]={&enabled,&kills};invoke(statTrakCtor,data,args,ok);
        return ok?data:nullptr;
    }
    void cancelCosmeticRetry(){
        if(observedWeapon>=0)[desiredSkins removeObjectForKey:@(observedWeapon)];
        desiredKnife=nil;expectedKnife=-1;expectedSkin=nil;
    }
    void requestSkin(P ac,NSString* key,double now,bool& ok){
        using namespace Runtime;
        if(!setSkin){status=@"Метод применения материала недоступен";return;}
        P name=string_new(key.UTF8String);
        if(!name)return;
        if(roots)roots->hold(name);
        P stat=newStatTrak(ok);
        if(!stat){status=@"Скин: не удалось создать настройки StatTrak";return;}
        P args[]={name,stat};
        invoke(setSkin,ac,args,ok);
        if(ok){
            expectedSkin=key;skinWeapon=observedWeapon;verifySkinAfter=now+1;
            status=@"Материал запрошен; проверяю ключ в игре";
        }
    }
    void restoreSkin(P ac,P parameters,bool resetCustomKnife,double now,bool& ok){
        for(auto it=skinHistory.begin();it!=skinHistory.end();++it){
            if(it->weapon!=observedWeapon)continue;
            if(it->skin.length)requestSkin(ac,it->skin,now,ok);
            else if(createWeapon){
                P original=Runtime::gchandle_get_target(it->parameters);
                if(original)equipPreservingAmmo(ac,original,ok);
                if(ok){expectedSkin=@"";skinWeapon=observedWeapon;verifySkinAfter=now+1;status=@"Оружие пересоздано; проверяю стандартный материал";}
            }else {status=@"Штатное восстановление материала недоступно";return;}
            if(ok){Runtime::gchandle_free(it->parameters);skinHistory.erase(it);}
            return;
        }
        if(resetCustomKnife&&createWeapon){
            equip(ac,parameters,ok);
            if(ok){expectedSkin=@"";skinWeapon=observedWeapon;verifySkinAfter=now+1;status=@"Запрошен стандартный материал этой модели ножа";}
            return;
        }
        status=@"Автоприменение материала выключено для текущего оружия";
    }
    void sticker(P weapon,NSInteger kit,NSInteger slot,P arrayGet,bool& ok){
        using namespace Runtime;
        if(!weapon||kit<=0||kit>65535||slot<0||slot>4||!array_new||!value_box)return;
        P controller=cls("ScarletWork.Weapon","WeaponController");
        P renderers=cls("ScarletWork.Stickers","StickerRenderers");
        P codec=cls("ScarletWork.Stickers","StickerCodec");
        P show=method(controller,"ShowStickers",2),getOld=method(renderers,"get_Stickers",0);
        P encode=method(codec,"Encode",2,"System.Int32"),decode=method(codec,"TryDecode",3,"System.Byte[]");
        P arraySet=method(cls("System","Array"),"SetValue",2,"System.Object"),byteClass=cls("System","Byte");
        P stickersField=typedField(controller,"_stickers","ScarletWork.Stickers.StickerRenderers");
        if(!returns(show,"System.Void")||!parametersMatch(show,{"<reference>","System.Int32"})||
            !getOld||!returns(encode,"System.Byte[]")||!returns(decode,"System.Boolean")||
            !parametersMatch(arraySet,{"System.Object","System.Int32"})||!stickersField||!byteClass){
            status=@"Наклейки: игровой кодек недоступен";return;
        }
        P outputType=method_get_param(decode,2);
        char* typeName=type_get_name(outputType);
        bool typed=typeName&&strstr(typeName,"System.Collections.Generic.List")&&
            strstr(typeName,"ScarletWork.Stickers.StickerData")&&strchr(typeName,'&');
        if(typeName)free_(typeName);
        char* seedType=type_get_name(method_get_param(decode,1));
        typed=typed&&seedType&&!strcmp(seedType,"System.Int32&");
        if(seedType)free_(seedType);
        if(!typed){status=@"Наклейки: сигнатура кодека не совпала";return;}
        // StickerCodec.Write: ushort seed, byte count, then ten bytes per sticker.
        std::array<uint8_t,53> payload{};
        P active=objectField(weapon,stickersField);
        P old=active?invoke(getOld,active,nullptr,ok):nullptr;
        int seed=0,count=0;
        if(old){
            P args[]={&seed,old};
            P bytes=invoke(encode,nullptr,args,ok);
            if(!bytes||!ok)return;
            uintptr_t length=array_length(bytes);
            if(length<3||length>payload.size())return;
            for(int i=0;i<(int)length&&ok;i++){
                P idx[]={&i};P item=invoke(arrayGet,bytes,idx,ok);
                if(!item)return;
                payload[i]=*static_cast<uint8_t*>(object_unbox(item));
            }
            if(!ok||payload[2]>5||length!=3u+10u*payload[2])return;
            count=payload[2];
        }
        int index=0;
        while(index<count&&payload[3+10*index]!=slot)++index;
        if(index==count&&count==5){status=@"Все пять слотов заняты";return;}
        if(index==count)payload[2]=++count;
        uint8_t* record=payload.data()+3+10*index;
        memset(record,0,10);
        record[0]=(uint8_t)slot;record[1]=(uint8_t)kit;record[2]=(uint8_t)(kit>>8);
        int length=3+10*count;
        P bytes=array_new(byteClass,length);
        if(!bytes)return;
        if(roots)roots->hold(bytes);
        for(int i=0;i<length&&ok;i++){
            P box=value_box(byteClass,&payload[i]);
            if(!box)return;
            if(roots)roots->hold(box);
            P args[]={box,&i};invoke(arraySet,bytes,args,ok);
        }
        P list=nullptr;
        P args[]={bytes,&seed,&list};
        P decoded=invoke(decode,nullptr,args,ok);
        if(!decoded||!*static_cast<bool*>(object_unbox(decoded))||!list){
            status=@"Игра не прочитала данные наклейки";return;
        }
        if(roots)roots->hold(list);
        int mip=1;P apply[]={list,&mip};invoke(show,weapon,apply,ok);
        if(ok)status=[NSString stringWithFormat:@"Игра приняла наклейку для слота %ld",(long)slot+1];
    }
    bool cosmetics(P ac,NSDictionary* request,P arrayGet,double now,bool& ok){
        using namespace Runtime;
        if(!cosmeticArms||gchandle_get_target(cosmeticArms)!=ac){
            resetCosmetics();cosmeticArms=gchandle_new(ac,true);
        }
        P p=invoke(current,ac,nullptr,ok);
        P weapon=currentController?invoke(currentController,ac,nullptr,ok):nullptr;
        if(!p||!weapon||!ok)return false;
        int held=id(invoke(weaponId,p,nullptr,ok));
        if(!ok||held<0)return false;
        bool newWeapon=held!=observedWeapon||!cosmeticWeapon||gchandle_get_target(cosmeticWeapon)!=weapon;
        if(newWeapon){
            if(cosmeticWeapon)gchandle_free(cosmeticWeapon);
            cosmeticWeapon=gchandle_new(weapon,true);
            attemptedSkin=nil;skinVerified=false;
            if(held!=observedWeapon&&expectedKnife<0){attemptedKnife=-1;knifeMaterialTried=false;}
        }
        observedWeapon=held;
        bool nameOK=true,skinReadOK=true;
        if(newWeapon&&displayName)observedName=text(invoke(displayName,p,nullptr,nameOK));
        observedSkin=equippedSkin?text(invoke(equippedSkin,weapon,nullptr,skinReadOK)):@"";
        Runtime::lastException=nil;
        if(!desiredSkins)desiredSkins=[NSMutableDictionary dictionary];

        NSString* action=request[@"action"];
        NSDictionary* selected=request[@"weapon"];
        NSNumber* selectedID=selected[@"id"];
        NSString* key=request[@"skin"][@"key"];
        if([action isEqual:@"sticker"]){
            P knife=invoke(isKnife,ac,nullptr,ok);
            if(!knife||*static_cast<bool*>(object_unbox(knife)))status=@"Выбери огнестрельное оружие";
            else sticker(weapon,[request[@"skin"][@"id"] integerValue],[request[@"skin"][@"slot"] integerValue],arrayGet,ok);
            return true;
        }

        if([action isEqual:@"restoreSkin"]){
            bool customKnife=desiredSkins[@(held)]&&desiredKnife&&held==[desiredKnife[@"id"] intValue];
            [desiredSkins removeObjectForKey:@(held)];
            expectedSkin=nil;attemptedSkin=nil;skinVerified=false;
            restoreSkin(ac,p,customKnife,now,ok);
            return true;
        }
        if([action isEqual:@"restoreModel"]){
            desiredKnife=nil;attemptedKnife=expectedKnife=-1;expectedSkin=nil;knifeMaterialFallback=false;
            P knife=invoke(isKnife,ac,nullptr,ok);
            if(originalKnife&&createWeapon&&knife&&*static_cast<bool*>(object_unbox(knife))){
                P original=gchandle_get_target(originalKnife);
                if(original){
                    equip(ac,original,ok,originalKnifeSkin);
                    if(ok){gchandle_free(originalKnife);originalKnife=0;originalKnifeSkin=nil;status=@"Исходный нож запрошен";}
                }
            }else status=@"Автозамена ножа выключена. Исходная модель вернётся при возрождении.";
            return true;
        }
        if([action isEqual:@"skin"]||[action isEqual:@"model"]){
            if(!selectedID)return true;
            if([selected[@"category"] isEqual:@"knives"]){
                desiredKnife=selected;attemptedKnife=expectedKnife=-1;knifeMaterialFallback=knifeMaterialTried=false;
            }else if([selectedID intValue]!=held){
                status=@"Возьми в руки выбранное оружие";return true;
            }
            if(key.length)desiredSkins[selectedID]=key;
            expectedSkin=nil;attemptedSkin=nil;skinVerified=false;
        }

        if(expectedKnife>=0){
            if(now<verifyKnifeAfter)return true;
            if(held!=expectedKnife&&now<verifyKnifeDeadline)return true;
            int expected=expectedKnife;expectedKnife=-1;
            if(held!=expected){
                knifeMaterialFallback=false;
                status=[NSString stringWithFormat:@"Запрошен нож ID %d, игра показывает %d",expected,held];
                return true;
            }
            status=[NSString stringWithFormat:@"Игра подтвердила ID ножа %d",held];
        }
        if(desiredKnife&&held!=[desiredKnife[@"id"] intValue]&&attemptedKnife!=held){
            P knife=invoke(isKnife,ac,nullptr,ok);
            if(knife&&*static_cast<bool*>(object_unbox(knife))){
                attemptedKnife=held;
                int desired=[desiredKnife[@"id"] intValue];
                P parameters=createWeapon?parametersFor(desired,desiredKnife[@"name"],arrayGet,ok):nullptr;
                if(!parameters){status=@"Конфигурация выбранного ножа недоступна";return true;}
                if(!originalKnife){originalKnife=gchandle_new(p,true);originalKnifeSkin=observedSkin;}
                NSString* material=desiredSkins[@(desired)];
                equip(ac,parameters,ok,material);
                if(ok){
                    expectedKnife=desired;verifyKnifeAfter=now+1;verifyKnifeDeadline=now+3;knifeMaterialFallback=knifeMaterialTried=material.length>0;
                    status=@"Модель и материал ножа переданы игре";
                }
                return true;
            }
        }
        if(expectedSkin){
            if(held!=skinWeapon){expectedSkin=nil;}
            else {
                if(now<verifySkinAfter)return true;
                skinVerified=[observedSkin isEqualToString:expectedSkin];
                status=skinVerified?@"Игра подтвердила ключ материала":[NSString stringWithFormat:@"Запрошен %@; игра сообщает %@",expectedSkin,observedSkin.length?observedSkin:@"стандартный"];
                expectedSkin=nil;
                return true;
            }
        }
        NSString* material=desiredSkins[@(held)];
        if(!material.length)return true;
        if([observedSkin isEqualToString:material]){
            attemptedSkin=material;skinVerified=true;knifeMaterialFallback=false;
            return true;
        }
        if([attemptedSkin isEqualToString:material]&&!skinVerified&&!knifeMaterialFallback)return true;
        attemptedSkin=material;skinVerified=false;
        rememberSkin(p);
        if(!knifeMaterialFallback&&!knifeMaterialTried&&createWeapon&&desiredKnife&&held==[desiredKnife[@"id"] intValue]){
            knifeMaterialTried=true;
            equip(ac,p,ok,material);
            if(ok){expectedKnife=held;verifyKnifeAfter=now+1;verifyKnifeDeadline=now+3;knifeMaterialFallback=true;}
        }else{
            knifeMaterialFallback=false;
            requestSkin(ac,material,now,ok);
        }
        return true;
    }
    NSString* summary(){
        NSMutableArray* parts=[NSMutableArray array];
        if(spreadFields&&std::isfinite(spreadObserved)){
            [parts addObject:[NSString stringWithFormat:@"Spread %.4f → %.4f (%d)",spreadBefore,spreadObserved,spreadFields]];
        }else if(spreadFields)[parts addObject:[NSString stringWithFormat:@"Spread: %d полей, без показания движка",spreadFields]];
        if(std::isfinite(speedObserved))[parts addObject:[NSString stringWithFormat:@"Speed %.2f · лимит %.2f",horizontalObserved,speedObserved]];
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
    void clearSpreadSample(){
        if(spreadParameters)Runtime::gchandle_free(spreadParameters);
        spreadParameters=0;spreadBefore=spreadObserved=NAN;spreadFields=0;spreadSampleAfter=0;
    }
    void restoreSpread(){if(ready)patches.restore(2);clearSpreadSample();}
    void restoreSpeed(){if(ready)patches.restore(3);speedObserved=horizontalObserved=NAN;}
    void restorePlayer(){
        if(ready)patches.restore();
        clearSpreadSample();speedObserved=horizontalObserved=NAN;ammoObserved=-1;ammoMaximum=-1;
    }
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
    }
    bool fireHeld(P controls,bool& ok){
        if(!controls||!shootButton||!pressed)return false;
        P button=Runtime::invoke(shootButton,controls,nullptr,ok);P value=button?Runtime::invoke(pressed,button,nullptr,ok):nullptr;
        return value&&*static_cast<bool*>(Runtime::object_unbox(value));
    }
    static float angle(float value){value=fmodf(value+180,360);if(value<0)value+=360;return value-180;}
    void nextAgent(P skeleton,P arrayGet,bool& ok){
        using namespace Runtime;
        P type=cls("ScarletWork.Game","AgentDefinition"),skeletonClass=cls("ScarletWork.Game","PlayerSkeleton");
        P get=method(skeletonClass,"get_Agent",0),set=method(skeletonClass,"SetAgent",1,"ScarletWork.Game.AgentDefinition");
        P team=method(type,"get_Team",0);
        uint32_t alignment=0;P teamClass=cls("","Team");
        if(!type||!findAll||!teamClass||class_value_size(teamClass,&alignment)!=sizeof(int)||
            !returns(get,"ScarletWork.Game.AgentDefinition")||!returns(set,"System.Void")||!returns(team,"Team")){
            status=@"Замена агента: игровой API недоступен";return;
        }
        P currentAgent=invoke(get,skeleton,nullptr,ok);
        P side=currentAgent?invoke(team,currentAgent,nullptr,ok):nullptr;
        int sideValue=side?*static_cast<int*>(object_unbox(side)):0;
        if(!ok||sideValue<=0){status=@"Замена агента: текущая команда не определена";return;}
        P typeObject=type_get_object(class_get_type(type));if(roots)roots->hold(typeObject);
        P args[]={typeObject};P list=invoke(findAll,nullptr,args,ok);
        if(!list||array_length(list)>256)return;
        P first=nullptr,next=nullptr;bool passed=false;
        for(int i=0;i<(int)array_length(list)&&ok;i++){
            P index[]={&i};P agent=invoke(arrayGet,list,index,ok);
            if(agent==currentAgent){passed=true;continue;}
            P side=agent?invoke(team,agent,nullptr,ok):nullptr;
            if(!side||*static_cast<int*>(object_unbox(side))!=sideValue)continue;
            if(!first)first=agent;
            if(passed){next=agent;break;}
        }
        if(!next)next=first;
        if(!next){status=@"Другие агенты этой команды ещё не загружены";return;}
        P change[]={next};invoke(set,skeleton,change,ok);
        if(ok)status=@"Модель агента передана локальному скелету";
    }
    void aim(P pc,P cameraTransform,V3 cameraPos,V3 target,float alpha,float maxStep,bool& ok){
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
        float pitchStep=angle(wantPitch-camPitch)*alpha,yawStep=angle(wantYaw-cam.y)*alpha;
        float magnitude=hypotf(pitchStep,yawStep);
        if(magnitude>maxStep){pitchStep*=maxStep/magnitude;yawStep*=maxStep/magnitude;}
        float next=std::clamp(pitch+sign*pitchStep,-limit,limit);
        bodyAngles.y+=yawStep;
        if(!ok)return;write(controller,rotation,&next);P args[]={&bodyAngles};invoke(setEuler,bodyTr,args,ok);
    }
    void equip(P ac,P parameters,bool& ok,NSString* skin=nil){
        using namespace Runtime;
        P material=skin.length?string_new(skin.UTF8String):nullptr;
        if(material&&roots)roots->hold(material);
        P stat=newStatTrak(ok);
        if(!stat){ok=false;status=@"Нож: не удалось создать настройки StatTrak";return;}
        bool pickup=false;int seed=0,owner=0;
        P args[]={parameters,material,stat,&pickup,nullptr,&seed,&owner};
        invoke(createWeapon,ac,args,ok);
    }
    void equipPreservingAmmo(P ac,P parameters,bool& ok){
        using namespace Runtime;
        int16_t magazine=0,reserve=0;
        bool hasAmmo=ammoCounts(ac,magazine,reserve);
        equip(ac,parameters,ok);
        if(ok&&hasAmmo&&ammoConfig){
            P config=invoke(ammoConfig,parameters,nullptr,ok);
            if(config)syncAmmo(ac,config,magazine,reserve,ok);
        }
    }

};
}
