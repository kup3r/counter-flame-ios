#pragma once

namespace FlameEffects {
using namespace Runtime;
using FlameScene::Color;

struct State {
    bool antiFlash=false,antiSmoke=false,initialized=false;
    P managerType=nullptr,find=nullptr,overlayField=nullptr,canvasField=nullptr,smokesField=nullptr;
    P enabledGet=nullptr,enabledSet=nullptr,floatGet=nullptr,floatSet=nullptr,has=nullptr,colorGet=nullptr,colorSet=nullptr;
    std::array<P,10> materials{};
    uint32_t managerOwner=0,canvasOwner=0;
    struct Material {
        uint32_t owner;
        const char* property;
        Color original;
        bool color;
    };
    std::vector<Material> changes;
    double nextProbe=0;
    NSString* status=@"";

    void init(Extra::State& extra,FlameScene::State& c){
        initialized=true;
        const char* ns="ScarletWork.Game.Grenades";
        P manager=cls(ns,"GrenadeManager"),overlay=cls(ns,"FlashOverlay"),smoke=cls(ns,"SmokeField");
        if(manager)managerType=class_get_type(manager);
        find=c.checked(extra,cls("UnityEngine","Object"),"FindObjectOfType","UnityEngine.Object",{"System.Type"});
        overlayField=extra.typedField(manager,"_overlay","ScarletWork.Game.Grenades.FlashOverlay");
        canvasField=extra.typedField(overlay,"_canvas","UnityEngine.Canvas");
        smokesField=extra.typedField(manager,"_smokes","ScarletWork.Game.Grenades.SmokeField");
        const char* names[]={"_outside","_outsideTunnels","_puffs","_puffsTunnels","_puffsFar","_puffsFarTunnels",
                             "_puffsInside","_puffsInsideTunnels","_overlay","_overlaySolid"};
        for(size_t i=0;i<materials.size();++i)materials[i]=extra.typedField(smoke,names[i],"UnityEngine.Material");
        P behaviour=cls("UnityEngine","Behaviour");
        enabledGet=c.checked(extra,behaviour,"get_enabled","System.Boolean");
        enabledSet=c.checked(extra,behaviour,"set_enabled","System.Void",{"System.Boolean"});
        P material=cls("UnityEngine","Material");
        has=c.checked(extra,material,"HasProperty","System.Boolean",{"System.String"});
        floatGet=c.checked(extra,material,"GetFloat","System.Single",{"System.String"});
        floatSet=c.checked(extra,material,"SetFloat","System.Void",{"System.String","System.Single"});
        uint32_t alignment=0;
        P color=cls("UnityEngine","Color");
        if(color&&class_value_size(color,&alignment)==sizeof(Color)){
            colorGet=c.checked(extra,material,"GetColor","UnityEngine.Color",{"System.String"});
            colorSet=c.checked(extra,material,"SetColor","System.Void",{"System.String","UnityEngine.Color"});
        }
    }
    void restoreFlash(Extra::State& extra,FlameScene::State& c){
        if(!canvasOwner)return;
        P canvas=gchandle_get_target(canvasOwner);
        bool ok=true,current=true;
        if(extra.alive(canvas,ok)&&c.value(enabledGet,canvas,nullptr,current,ok)&&!current){
            bool enabled=true;
            P args[]={&enabled};invoke(enabledSet,canvas,args,ok);
        }
        gchandle_free(canvasOwner);canvasOwner=0;
    }
    void restoreSmoke(Extra::State& extra,FlameScene::State& c){
        for(auto& entry:changes){
            bool ok=true;
            P material=gchandle_get_target(entry.owner);
            if(extra.alive(material,ok)){
                P key=extra.string_new(entry.property);
                if(key&&roots)roots->hold(key);
                P args[]={key,&entry.original};
                if(key&&entry.color){
                    Color current{},applied=entry.original;applied.a=0;
                    if(c.value(colorGet,material,args,current,ok)&&current.same(applied))
                        invoke(colorSet,material,args,ok);
                }else if(key){
                    float current=0;
                    if(c.value(floatGet,material,args,current,ok)&&current==0)
                        invoke(floatSet,material,args,ok);
                }
            }
            gchandle_free(entry.owner);
        }
        changes.clear();
    }
    void clear(Extra::State& extra,FlameScene::State& c){
        restoreFlash(extra,c);restoreSmoke(extra,c);
        if(managerOwner){gchandle_free(managerOwner);managerOwner=0;}
        nextProbe=0;status=@"";
    }
    bool flash(Extra::State& extra,FlameScene::State& c,P manager){
        bool ok=true;
        P overlay=extra.objectField(manager,overlayField);
        P canvas=extra.alive(overlay,ok)?extra.objectField(overlay,canvasField):nullptr;
        if(canvasOwner&&canvas!=gchandle_get_target(canvasOwner))restoreFlash(extra,c);
        bool enabled=false;
        if(!enabledSet||!extra.alive(canvas,ok)||!c.value(enabledGet,canvas,nullptr,enabled,ok))return false;
        if(enabled){
            uint32_t handle=canvasOwner?canvasOwner:gchandle_new(canvas,true);
            if(!handle)return false;
            bool hidden=false;
            P args[]={&hidden};invoke(enabledSet,canvas,args,ok);
            if(ok)canvasOwner=handle;
            else if(!canvasOwner)gchandle_free(handle);
        }
        return ok;
    }
    void hideProperty(Extra::State& extra,FlameScene::State& c,P material,const char* property,bool color){
        P setter=color?colorSet:floatSet;
        if(!setter)return;
        bool ok=true,supported=false;
        P key=extra.string_new(property);
        if(!key)return;
        if(roots)roots->hold(key);
        P args[]={key,nullptr};
        if(!c.value(has,material,args,supported,ok)||!supported)return;
        auto existing=std::find_if(changes.begin(),changes.end(),[&](const Material& item){
            return item.property==property&&gchandle_get_target(item.owner)==material;
        });
        Color before{},desired{};
        if(color){
            if(!c.value(colorGet,material,args,before,ok)||!before.valid()||before.a==0)return;
            desired=before;desired.a=0;
        }else if(!c.value(floatGet,material,args,before.r,ok)||!std::isfinite(before.r)||before.r==0)return;
        uint32_t handle=existing==changes.end()?gchandle_new(material,true):existing->owner;
        if(!handle)return;
        args[1]=&desired;
        invoke(setter,material,args,ok);
        if(ok){
            if(existing==changes.end())changes.push_back({handle,property,before,color});
            else existing->original=before;
        }else if(existing==changes.end())gchandle_free(handle);
    }
    void tick(Extra::State& extra,FlameScene::State& c,double now){
        if(!antiFlash&&!antiSmoke){if(managerOwner||canvasOwner||!changes.empty())clear(extra,c);return;}
        if(now<nextProbe)return;
        nextProbe=now+.25;
        if(!initialized)init(extra,c);
        bool ok=true;
        P manager=managerOwner?gchandle_get_target(managerOwner):nullptr;
        if(!extra.alive(manager,ok)){
            if(managerOwner)clear(extra,c);
            if(!find||!managerType)return;
            P type=type_get_object(managerType);
            if(type&&roots)roots->hold(type);
            P args[]={type};
            manager=type?invoke(find,nullptr,args,ok):nullptr;
            if(!extra.alive(manager,ok))return;
            managerOwner=gchandle_new(manager,true);
        }
        NSMutableArray* labels=[NSMutableArray array];
        if(antiFlash)[labels addObject:flash(extra,c,manager)?@"Flash: Canvas":@"Flash: ожидание Canvas"];
        if(antiSmoke){
            P smoke=extra.objectField(manager,smokesField);
            for(P field:materials){
                P material=extra.objectField(smoke,field);
                if(!extra.alive(material,ok))continue;
                hideProperty(extra,c,material,"_Color",true);
                hideProperty(extra,c,material,"_Opacity",false);
                hideProperty(extra,c,material,"_Solid",false);
                hideProperty(extra,c,material,"_ZWrite",false);
            }
            [labels addObject:changes.empty()?@"Smoke: ожидание материалов":@"Smoke: материалы"];
        }
        status=[labels componentsJoinedByString:@" · "];
    }
};
}
