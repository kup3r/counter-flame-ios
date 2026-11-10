#pragma once

namespace FlameGloves {
using Runtime::P;

struct State {
    struct Applied {uint32_t owner;int request;bool ct;};
    std::vector<Applied> applied;
    P controllerType=nullptr,component=nullptr,children=nullptr,resolve=nullptr,load=nullptr,refresh=nullptr,requestField=nullptr;
    P rendererMaterials=nullptr,materialName=nullptr,arrayGet=nullptr,isLocalCt=nullptr;
    NSDictionary* desired=nil;
    uint32_t pendingRenderer=0;
    double nextProbe=0,verifyAt=0,verifyUntil=0;
    bool initialized=false;
    NSString* status=@"";

    void init(Extra::State& extra,FlameScene::State& checked){
        using namespace Runtime;
        initialized=true;
        P controller=cls("ScarletWork.Main","GlovesController");
        if(!controller)return;
        controllerType=class_get_type(controller);
        P c=cls("UnityEngine","Component");
        component=checked.checked(extra,c,"GetComponent","UnityEngine.Component",{"System.Type"});
        children=checked.checked(extra,c,"GetComponentInChildren","UnityEngine.Component",{"System.Type","System.Boolean"});
        resolve=checked.checked(extra,controller,"Resolve","UnityEngine.SkinnedMeshRenderer",{"ScarletWork.Main.GlovesType","System.Boolean"});
        load=checked.checked(extra,controller,"LoadGloves","System.Void",{"System.String","UnityEngine.SkinnedMeshRenderer","System.Int32"});
        refresh=checked.checked(extra,controller,"RefreshMaterials","System.Void");
        requestField=extra.typedField(controller,"_request","System.Int32");
        rendererMaterials=checked.checked(extra,cls("UnityEngine","Renderer"),"get_sharedMaterials","UnityEngine.Material[]");
        arrayGet=checked.checked(extra,cls("System","Array"),"GetValue","System.Object",{"System.Int32"});
        isLocalCt=checked.checked(extra,controller,"IsLocalCt","System.Boolean");
        materialName=checked.checked(extra,cls("UnityEngine","Object"),"get_name","System.String");
    }
    void clearPending(){
        if(pendingRenderer)Runtime::gchandle_free(pendingRenderer);
        pendingRenderer=0;
    }
    void clear(Extra::State& extra,bool restore){
        using namespace Runtime;
        RootScope roots;
        clearPending();
        for(const Applied& entry:applied){
            P owner=gchandle_get_target(entry.owner);
            bool ok=true;
            if(restore&&refresh&&requestField&&extra.alive(owner,ok)){
                int current=0;extra.read(owner,requestField,&current);
                if(current==entry.request)invoke(refresh,owner,nullptr,ok);
            }
            gchandle_free(entry.owner);
        }
        applied.clear();nextProbe=0;
    }
    void verify(Extra::State& extra,double now){
        using namespace Runtime;
        if(!pendingRenderer||now<verifyAt||!desired)return;
        bool ok=true;P renderer=gchandle_get_target(pendingRenderer);
        P materials=rendererMaterials&&extra.alive(renderer,ok)?invoke(rendererMaterials,renderer,nullptr,ok):nullptr;
        NSString* key=[desired[@"key"] lowercaseString];
        bool confirmed=false;
        if(materials&&arrayGet&&materialName&&array_length(materials)<=12){
            for(int i=0;i<(int)array_length(materials)&&ok;i++){
                P args[]={&i};P material=invoke(arrayGet,materials,args,ok);
                NSString* label=material?extra.text(invoke(materialName,material,nullptr,ok)):@"";
                if([[label lowercaseString] containsString:key]){confirmed=true;break;}
            }
        }
        if(confirmed){
            status=[NSString stringWithFormat:@"Перчатки: %@ · материал подтверждён",desired[@"label"]];
            clearPending();
        }else if(now>=verifyUntil){
            status=@"Перчатки: материал отправлен, подтверждение не получено";
            clearPending();
        }else verifyAt=now+.3;
    }
    void tick(Extra::State& extra,FlameScene::State& checked,P arms,double now){
        using namespace Runtime;
        if(!desired||now<nextProbe)return;
        nextProbe=now+.25;
        if(!initialized)init(extra,checked);
        if(!controllerType||!component||!resolve||!load||!requestField||!refresh||!extra.currentController){
            status=@"Перчатки: API игры недоступен";return;
        }
        verify(extra,now);
        bool ok=true,ct=false;
        if(!checked.value(isLocalCt,nullptr,nullptr,ct,ok))return;
        for(auto it=applied.begin();it!=applied.end();){
            bool entryOK=true;
            if(!extra.alive(gchandle_get_target(it->owner),entryOK)){
                gchandle_free(it->owner);it=applied.erase(it);
            }else ++it;
        }
        P weapon=invoke(extra.currentController,arms,nullptr,ok);
        if(!extra.alive(weapon,ok))return;
        P type=type_get_object(controllerType);
        if(!type)return;
        if(roots)roots->hold(type);
        bool inactive=true;P lookup[]={type,&inactive};
        P gloves=invoke(component,weapon,lookup,ok);
        if(!extra.alive(gloves,ok)&&ok&&children)gloves=invoke(children,weapon,lookup,ok);
        if(!extra.alive(gloves,ok)){status=@"Перчатки: у этого оружия нет контроллера рук";return;}
        int generation=0;extra.read(gloves,requestField,&generation);
        auto old=std::find_if(applied.begin(),applied.end(),[&](const Applied& item){return gchandle_get_target(item.owner)==gloves;});
        if(old!=applied.end()&&old->request==generation&&old->ct==ct)return;
        if(generation<0||generation>=INT32_MAX-1)return;
        int kind=[desired[@"type"] intValue];
        if(kind<1||kind>5)return;
        P resolveArgs[]={&kind,&ct};P renderer=invoke(resolve,gloves,resolveArgs,ok);
        if(!extra.alive(renderer,ok)){status=@"Перчатки: эта модель недоступна для текущего оружия";return;}
        NSString* key=desired[@"key"];
        P material=extra.string_new(key.UTF8String);
        if(!material)return;
        if(roots)roots->hold(material);
        ++generation;
        extra.write(gloves,requestField,&generation);
        P loadArgs[]={material,renderer,&generation};invoke(load,gloves,loadArgs,ok);
        if(old==applied.end())applied.push_back({gchandle_new(gloves,true),generation,ct});
        else {old->request=generation;old->ct=ct;}
        if(!ok){status=Runtime::lastException?:@"Перчатки: ошибка загрузки";return;}
        clearPending();pendingRenderer=gchandle_new(renderer,true);
        verifyAt=now+.4;verifyUntil=now+3;
        status=[NSString stringWithFormat:@"Перчатки: загружаю %@",desired[@"label"]];
    }
};
}
