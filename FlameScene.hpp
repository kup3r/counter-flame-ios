#pragma once

// Local render overrides. A write is restored only while its original owner and value still match.
namespace FlameScene {
using Runtime::P;

struct Color {
    float r,g,b,a;
    bool valid() const {return std::isfinite(r)&&std::isfinite(g)&&std::isfinite(b)&&std::isfinite(a);}
    bool same(Color other) const {
        return fabsf(r-other.r)<.001f&&fabsf(g-other.g)<.001f&&fabsf(b-other.b)<.001f&&fabsf(a-other.a)<.001f;
    }
};
static Color hue(float degrees,float saturation=.78f,float brightness=1.f){
    CGFloat r=0,g=0,b=0,a=1;
    [[UIColor colorWithHue:fmodf(degrees,360.f)/360.f saturation:saturation brightness:brightness alpha:1] getRed:&r green:&g blue:&b alpha:&a];
    return {(float)r,(float)g,(float)b,(float)a};
}

struct State {
    struct Tint {uint32_t owner=0;const char* property=nullptr;Color before{},applied{};};
    P renderSky=nullptr,ambientGet=nullptr,ambientSet=nullptr,ambientModeGet=nullptr,ambientModeSet=nullptr;
    P sunGet=nullptr,transformGet=nullptr,eulerGet=nullptr,eulerSet=nullptr;
    P hasColor=nullptr,colorGet=nullptr,colorSet=nullptr,materialGet=nullptr;
    bool initialized=false,ambientSaved=false;
    Color ambientBefore{},ambientApplied{};
    int modeBefore=-1;
    uint32_t sunOwner=0;
    V3 sunBefore{},sunApplied{};
    Tint sky{},body{},lod{};
    double nextTick=0;
    NSString* status=@"";

    P checked(Extra::State& extra,P cls,const char* name,const char* returns,std::initializer_list<const char*> args={}){
        P method=Runtime::method(cls,name,(uint32_t)args.size(),args.size()?*args.begin():nullptr);
        return extra.returns(method,returns)&&extra.parametersMatch(method,args)?method:nullptr;
    }
    void init(Extra::State& extra){
        using namespace Runtime;
        initialized=true;
        uint32_t alignment=0;
        P color=cls("UnityEngine","Color");
        if(!color||class_value_size(color,&alignment)!=sizeof(Color))return;
        P render=cls("UnityEngine","RenderSettings");
        P material=cls("UnityEngine","Material");
        P renderer=cls("UnityEngine","Renderer");
        P component=cls("UnityEngine","Component");
        P transform=cls("UnityEngine","Transform");
        P mode=cls("UnityEngine.Rendering","AmbientMode");
        renderSky=checked(extra,render,"get_skybox","UnityEngine.Material");
        ambientGet=checked(extra,render,"get_ambientLight","UnityEngine.Color");
        ambientSet=checked(extra,render,"set_ambientLight","System.Void",{"UnityEngine.Color"});
        if(mode&&class_value_size(mode,&alignment)==sizeof(int)){
            ambientModeGet=checked(extra,render,"get_ambientMode","UnityEngine.Rendering.AmbientMode");
            ambientModeSet=checked(extra,render,"set_ambientMode","System.Void",{"UnityEngine.Rendering.AmbientMode"});
        }
        sunGet=checked(extra,render,"get_sun","UnityEngine.Light");
        transformGet=checked(extra,component,"get_transform","UnityEngine.Transform");
        eulerGet=checked(extra,transform,"get_eulerAngles","UnityEngine.Vector3");
        eulerSet=checked(extra,transform,"set_eulerAngles","System.Void",{"UnityEngine.Vector3"});
        hasColor=checked(extra,material,"HasProperty","System.Boolean",{"System.String"});
        colorGet=checked(extra,material,"GetColor","UnityEngine.Color",{"System.String"});
        colorSet=checked(extra,material,"SetColor","System.Void",{"System.String","UnityEngine.Color"});
        materialGet=checked(extra,renderer,"get_material","UnityEngine.Material");
    }
    template<class T> bool value(P method,P owner,P* args,T& out,bool& ok){
        if(!method)return false;
        P box=Runtime::invoke(method,owner,args,ok);
        if(!box)return false;
        P raw=Runtime::object_unbox(box);
        if(!raw)return false;
        memcpy(&out,raw,sizeof(T));
        return true;
    }
    void restoreTint(Extra::State& extra,Tint& entry,bool restore){
        using namespace Runtime;
        if(!entry.owner)return;
        P owner=gchandle_get_target(entry.owner);
        bool ok=true;
        if(restore&&extra.alive(owner,ok)){
            P key=extra.string_new(entry.property);if(roots)roots->hold(key);
            P args[]={key,&entry.before};
            Color current{};
            if(key&&value(colorGet,owner,args,current,ok)&&current.same(entry.applied))invoke(colorSet,owner,args,ok);
        }
        gchandle_free(entry.owner);entry={};
    }
    bool tint(Extra::State& extra,Tint& entry,P material,Color desired,bool skybox){
        using namespace Runtime;
        if(!hasColor||!colorGet||!colorSet)return false;
        bool ok=true;
        if(entry.owner&&gchandle_get_target(entry.owner)!=material)restoreTint(extra,entry,true);
        if(!extra.alive(material,ok))return false;
        const char* skyProps[]={"_SkyTint","_Tint","_Color"};
        const char* playerProps[]={"_BaseColor","_Color","_Tint"};
        const char* property=entry.property;
        P key=nullptr;
        if(!property){
            for(int i=0;i<3&&ok;i++){
                const char* candidate=(skybox?skyProps:playerProps)[i];
                key=extra.string_new(candidate);if(roots)roots->hold(key);
                bool has=false;P args[]={key};
                if(key&&value(hasColor,material,args,has,ok)&&has){property=candidate;break;}
            }
        }else{
            key=extra.string_new(property);if(roots)roots->hold(key);
        }
        if(!property||!key||!ok)return false;
        Color current{};P args[]={key,&desired};
        if(!value(colorGet,material,args,current,ok)||!current.valid())return false;
        if(!entry.owner){
            entry.owner=gchandle_new(material,true);entry.property=property;entry.before=current;
        }else if(!current.same(entry.applied))entry.before=current;
        desired.a=entry.before.a;
        if(!current.same(desired))invoke(colorSet,material,args,ok);
        if(ok)entry.applied=desired;
        return ok;
    }
    void restoreSun(Extra::State& extra,bool restore){
        using namespace Runtime;
        if(!sunOwner)return;
        P transform=gchandle_get_target(sunOwner);
        bool ok=true;V3 current{};
        if(restore&&extra.alive(transform,ok)&&value(eulerGet,transform,nullptr,current,ok)&&FlameDistanceSquared(current,sunApplied)<.01){
            P args[]={&sunBefore};invoke(eulerSet,transform,args,ok);
        }
        gchandle_free(sunOwner);sunOwner=0;
    }
    void restoreLight(Extra::State& extra,bool sameScene){
        using namespace Runtime;
        if(ambientSaved&&sameScene){
            bool ok=true;Color current{};int mode=-1;
            if(value(ambientGet,nullptr,nullptr,current,ok)&&current.same(ambientApplied)){
                P args[]={&ambientBefore};invoke(ambientSet,nullptr,args,ok);
            }
            if(value(ambientModeGet,nullptr,nullptr,mode,ok)&&mode==3){P args[]={&modeBefore};invoke(ambientModeSet,nullptr,args,ok);}
        }
        ambientSaved=false;
        restoreSun(extra,sameScene);
    }
    bool light(Extra::State& extra,float hour){
        using namespace Runtime;
        bool ok=true,changed=false;
        float daylight=std::clamp(sinf((hour-6.f)*(float)M_PI/12.f),0.f,1.f);
        float twilight=std::clamp(1.f-fabsf(daylight-.18f)*4.f,0.f,1.f);
        Color desired{.10f+.60f*daylight+.25f*twilight,.14f+.64f*daylight,.25f+.57f*daylight,1.f};
        if(ambientGet&&ambientSet&&ambientModeGet&&ambientModeSet){
            int mode=-1;Color current{};
            if(value(ambientModeGet,nullptr,nullptr,mode,ok)&&value(ambientGet,nullptr,nullptr,current,ok)&&current.valid()){
                if(!ambientSaved||!current.same(ambientApplied))ambientBefore=current;
                if(!ambientSaved||mode!=3)modeBefore=mode;
                int flat=3;
                if(mode!=flat){P args[]={&flat};invoke(ambientModeSet,nullptr,args,ok);}
                if(!current.same(desired)){P args[]={&desired};invoke(ambientSet,nullptr,args,ok);}
                ambientSaved=changed=ok;ambientApplied=desired;
            }
        }
        if(sunGet&&transformGet&&eulerGet&&eulerSet){
            bool sunOK=true;
            P sun=invoke(sunGet,nullptr,nullptr,sunOK);
            P transform=extra.alive(sun,sunOK)?invoke(transformGet,sun,nullptr,sunOK):nullptr;
            if(sunOwner&&gchandle_get_target(sunOwner)!=transform)restoreSun(extra,true);
            V3 current{};
            if(extra.alive(transform,sunOK)&&value(eulerGet,transform,nullptr,current,sunOK)&&finiteV3(current)){
                if(!sunOwner){sunOwner=gchandle_new(transform,true);sunBefore=current;}
                else if(FlameDistanceSquared(current,sunApplied)>.01)sunBefore=current;
                V3 wanted{fmodf((hour-6.f)*15.f+360.f,360.f),sunBefore.y,sunBefore.z};
                if(FlameDistanceSquared(current,wanted)>.0001){P args[]={&wanted};invoke(eulerSet,transform,args,sunOK);}
                if(sunOK){value(eulerGet,transform,nullptr,sunApplied,sunOK);changed=true;}
            }
        }
        return changed;
    }
    void restorePlayer(Extra::State& extra){
        restoreTint(extra,body,true);restoreTint(extra,lod,true);
    }
    void clear(Extra::State& extra,bool sameScene){
        restoreTint(extra,sky,sameScene);
        restoreTint(extra,body,sameScene);restoreTint(extra,lod,sameScene);
        restoreLight(extra,sameScene);nextTick=0;status=@"";
    }
    void tick(Extra::State& extra,double now,bool changeSky,float skyHue,bool changeLight,float hour,
              bool changePlayer,float playerHue,P bodyRenderer,P lodRenderer){
        using namespace Runtime;
        if(now<nextTick)return;
        nextTick=now+.15;
        if(!initialized)init(extra);
        NSMutableArray* states=[NSMutableArray array];
        if(changeSky){
            bool ok=true;P material=renderSky?invoke(renderSky,nullptr,nullptr,ok):nullptr;
            [states addObject:tint(extra,sky,material,hue(skyHue,.63f),true)?@"небо":@"небо: шейдер без цвета"];
        }else restoreTint(extra,sky,true);
        if(changeLight)[states addObject:light(extra,hour)?@"свет":@"свет: API недоступен"];
        else restoreLight(extra,true);
        if(changePlayer){
            bool ok=true,lodOK=true;
            P a=materialGet&&extra.alive(bodyRenderer,ok)?invoke(materialGet,bodyRenderer,nullptr,ok):nullptr;
            P b=materialGet&&lodRenderer!=bodyRenderer&&extra.alive(lodRenderer,lodOK)?invoke(materialGet,lodRenderer,nullptr,lodOK):nullptr;
            float angle=playerHue>=360.f?fmod(now*105.,360.):playerHue;
            bool aTint=tint(extra,body,a,hue(angle),false);
            bool bTint=tint(extra,lod,b!=a?b:nullptr,hue(angle),false);
            [states addObject:aTint||bTint?@"окраска":@"окраска: материал недоступен"];
        }else restorePlayer(extra);
        status=[states componentsJoinedByString:@" · "];
    }
};
}
