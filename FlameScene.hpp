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
    P skyType=nullptr,cameraSkyGet=nullptr,componentGet=nullptr,clearGet=nullptr,clearSet=nullptr,backgroundGet=nullptr,backgroundSet=nullptr;
    P gradeActive=nullptr,gradeExposure=nullptr,gradeShadow=nullptr,gradeHighlight=nullptr,gradeApply=nullptr;
    void (*staticRead)(P,P)=nullptr;
    P transformGet=nullptr,eulerGet=nullptr,eulerSet=nullptr,isChildOf=nullptr;
    P hasColor=nullptr,colorGet=nullptr,colorSet=nullptr,materialGet=nullptr;
    bool initialized=false;
    uint32_t cameraOwner=0,gradeOwner=0,spinOwner=0;
    int clearBefore=0;
    Color backgroundBefore{},backgroundApplied{},shadowBefore{},shadowApplied{},highlightBefore{},highlightApplied{};
    float exposureBefore=0,exposureApplied=0,spinOffset=0;
    V3 spinBefore{},spinApplied{};
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
        P material=cls("UnityEngine","Material");
        P renderer=cls("UnityEngine","Renderer");
        P component=cls("UnityEngine","Component");
        P camera=cls("UnityEngine","Camera");
        P transform=cls("UnityEngine","Transform");
        P skybox=cls("UnityEngine","Skybox");
        if(skybox)skyType=class_get_type(skybox);
        cameraSkyGet=checked(extra,skybox,"get_material","UnityEngine.Material");
        componentGet=checked(extra,component,"GetComponent","UnityEngine.Component",{"System.Type"});
        P flags=cls("UnityEngine","CameraClearFlags");
        if(flags&&class_value_size(flags,&alignment)==sizeof(int)){
            clearGet=checked(extra,camera,"get_clearFlags","UnityEngine.CameraClearFlags");
            clearSet=checked(extra,camera,"set_clearFlags","System.Void",{"UnityEngine.CameraClearFlags"});
        }
        backgroundGet=checked(extra,camera,"get_backgroundColor","UnityEngine.Color");
        backgroundSet=checked(extra,camera,"set_backgroundColor","System.Void",{"UnityEngine.Color"});

        P grade=cls("","MapColorGrading");
        gradeActive=grade?extra.field(grade,"_active"):nullptr;
        if(gradeActive&&!(extra.field_flags(gradeActive)&0x10))gradeActive=nullptr;
        gradeExposure=extra.typedField(grade,"exposure","System.Single");
        gradeShadow=extra.typedField(grade,"shadowTint","UnityEngine.Color");
        gradeHighlight=extra.typedField(grade,"highlightTint","UnityEngine.Color");
        gradeApply=checked(extra,grade,"Apply","System.Void");
        void* h=dlopen("@rpath/UnityFramework.framework/UnityFramework",RTLD_LAZY|RTLD_NOLOAD);
        if(h){staticRead=reinterpret_cast<decltype(staticRead)>(dlsym(h,"il2cpp_field_static_get_value"));dlclose(h);}
        transformGet=checked(extra,component,"get_transform","UnityEngine.Transform");
        eulerGet=checked(extra,transform,"get_localEulerAngles","UnityEngine.Vector3");
        eulerSet=checked(extra,transform,"set_localEulerAngles","System.Void",{"UnityEngine.Vector3"});
        isChildOf=checked(extra,transform,"IsChildOf","System.Boolean",{"UnityEngine.Transform"});
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
    void restoreBackground(Extra::State& extra,bool restore){
        using namespace Runtime;
        if(!cameraOwner)return;
        P camera=gchandle_get_target(cameraOwner);
        bool ok=true;Color current{};int flags=0;
        if(restore&&extra.alive(camera,ok)){
            if(value(backgroundGet,camera,nullptr,current,ok)&&current.same(backgroundApplied)){
                P args[]={&backgroundBefore};invoke(backgroundSet,camera,args,ok);
            }
            if(value(clearGet,camera,nullptr,flags,ok)&&flags==2){
                P args[]={&clearBefore};invoke(clearSet,camera,args,ok);
            }
        }
        gchandle_free(cameraOwner);cameraOwner=0;
    }
    NSString* changeSky(Extra::State& extra,P camera,Color desired){
        using namespace Runtime;
        if(!camera)return @"Небо: камера недоступна";
        bool ok=true;
        if(skyType&&cameraSkyGet&&componentGet){
            P type=type_get_object(skyType);if(type&&roots)roots->hold(type);
            P args[]={type};P component=type?invoke(componentGet,camera,args,ok):nullptr;
            P material=extra.alive(component,ok)?invoke(cameraSkyGet,component,nullptr,ok):nullptr;
            if(tint(extra,sky,material,desired,true)){
                restoreBackground(extra,true);return @"Небо: оттенок материала";
            }
        }
        if(!clearSet||!backgroundSet)return @"Небо: камера без поддержки цвета";
        ok=true;Color current{};int flags=0;
        if(cameraOwner&&gchandle_get_target(cameraOwner)!=camera)restoreBackground(extra,true);
        if(!value(backgroundGet,camera,nullptr,current,ok)||!value(clearGet,camera,nullptr,flags,ok)||!current.valid())
            return @"Небо: цвет камеры недоступен";
        if(!cameraOwner){cameraOwner=gchandle_new(camera,true);backgroundBefore=current;clearBefore=flags;}
        else {
            if(!current.same(backgroundApplied))backgroundBefore=current;
            if(flags!=2)clearBefore=flags;
        }
        if(flags!=2){int solid=2;P args[]={&solid};invoke(clearSet,camera,args,ok);}
        if(!current.same(desired)){P args[]={&desired};invoke(backgroundSet,camera,args,ok);}
        if(ok)backgroundApplied=desired;
        return ok?@"Небо: фон камеры":@"Небо: игра отклонила цвет";
    }
    void restoreLight(Extra::State& extra,bool sameScene){
        using namespace Runtime;
        if(!gradeOwner)return;
        P grade=gchandle_get_target(gradeOwner);
        bool ok=true;P active=nullptr;
        if(sameScene&&gradeActive&&staticRead)staticRead(gradeActive,&active);
        if(active==grade&&extra.alive(grade,ok)){
            float exposure=0;Color shadow{},highlight{};
            extra.read(grade,gradeExposure,&exposure);extra.read(grade,gradeShadow,&shadow);extra.read(grade,gradeHighlight,&highlight);
            if(fabsf(exposure-exposureApplied)<.001f)extra.write(grade,gradeExposure,&exposureBefore);
            if(shadow.same(shadowApplied))extra.write(grade,gradeShadow,&shadowBefore);
            if(highlight.same(highlightApplied))extra.write(grade,gradeHighlight,&highlightBefore);
            invoke(gradeApply,grade,nullptr,ok);
        }
        gchandle_free(gradeOwner);gradeOwner=0;
    }
    bool light(Extra::State& extra,float hour,float tintHue){
        using namespace Runtime;
        if(!staticRead||!gradeActive||!gradeExposure||!gradeShadow||!gradeHighlight||!gradeApply)return false;
        P grade=nullptr;staticRead(gradeActive,&grade);
        if(gradeOwner&&gchandle_get_target(gradeOwner)!=grade)restoreLight(extra,false);
        bool ok=true;
        if(!extra.alive(grade,ok))return false;
        float exposure=0;Color shadow{},highlight{};
        extra.read(grade,gradeExposure,&exposure);extra.read(grade,gradeShadow,&shadow);extra.read(grade,gradeHighlight,&highlight);
        if(!std::isfinite(exposure)||!shadow.valid()||!highlight.valid())return false;
        if(!gradeOwner){
            gradeOwner=gchandle_new(grade,true);
            exposureBefore=exposure;shadowBefore=shadow;highlightBefore=highlight;
        }else{
            if(fabsf(exposure-exposureApplied)>.001f)exposureBefore=exposure;
            if(!shadow.same(shadowApplied))shadowBefore=shadow;
            if(!highlight.same(highlightApplied))highlightBefore=highlight;
        }
        float daylight=std::clamp(sinf((hour-6.f)*(float)M_PI/12.f),0.f,1.f);
        float twilight=std::clamp(1.f-std::min(fabsf(hour-6.f),fabsf(hour-18.f))/2.5f,0.f,1.f);
        exposureApplied=std::clamp(exposureBefore*(.48f+.67f*daylight+.18f*twilight),.2f,3.f);
        shadowApplied={.44f+.56f*daylight,.63f+.37f*daylight,1.f,shadowBefore.a};
        highlightApplied={.72f+.28f*daylight+.28f*twilight,.76f+.24f*daylight-.20f*twilight,1.f-.52f*twilight,highlightBefore.a};
        if(tintHue<360.f){
            Color custom=hue(tintHue,.66f);
            shadowApplied={shadowApplied.r*custom.r,shadowApplied.g*custom.g,shadowApplied.b*custom.b,shadowBefore.a};
            highlightApplied={highlightApplied.r*(.5f+.5f*custom.r),highlightApplied.g*(.5f+.5f*custom.g),
                highlightApplied.b*(.5f+.5f*custom.b),highlightBefore.a};
        }
        if(fabsf(exposure-exposureApplied)>.001f||!shadow.same(shadowApplied)||!highlight.same(highlightApplied)){
            extra.write(grade,gradeExposure,&exposureApplied);
            extra.write(grade,gradeShadow,&shadowApplied);
            extra.write(grade,gradeHighlight,&highlightApplied);
            invoke(gradeApply,grade,nullptr,ok);
        }
        return ok;
    }
    static bool sameAngles(V3 a,V3 b){
        return fabsf(Extra::State::angle(a.x-b.x))<.05f&&fabsf(Extra::State::angle(a.y-b.y))<.05f&&fabsf(Extra::State::angle(a.z-b.z))<.05f;
    }
    void restoreSpin(Extra::State& extra){
        using namespace Runtime;
        if(!spinOwner)return;
        P transform=gchandle_get_target(spinOwner);
        bool ok=true;V3 current{};
        if(extra.alive(transform,ok)&&value(eulerGet,transform,nullptr,current,ok)&&sameAngles(current,spinApplied)){
            P args[]={&spinBefore};invoke(eulerSet,transform,args,ok);
        }
        gchandle_free(spinOwner);spinOwner=0;spinOffset=0;
    }
    bool spin(Extra::State& extra,P skeleton,P camera,float degrees){
        using namespace Runtime;
        if(!initialized)init(extra);
        if(!skeleton||!camera||!transformGet||!isChildOf||!eulerGet||!eulerSet)return false;
        bool ok=true,containsCamera=false;
        P transform=invoke(transformGet,skeleton,nullptr,ok);
        P cameraTransform=invoke(transformGet,camera,nullptr,ok);
        if(!transform||!cameraTransform||cameraTransform==transform)return false;
        P parent[]={transform};
        if(!value(isChildOf,cameraTransform,parent,containsCamera,ok)||containsCamera)return false;
        if(spinOwner&&gchandle_get_target(spinOwner)!=transform)restoreSpin(extra);
        V3 current{};
        if(!value(eulerGet,transform,nullptr,current,ok)||!finiteV3(current))return false;
        if(!spinOwner){spinOwner=gchandle_new(transform,true);spinBefore=current;}
        else if(!sameAngles(current,spinApplied))spinBefore=current;
        spinOffset=Extra::State::angle(spinOffset+degrees);
        V3 applied=spinBefore;applied.y=Extra::State::angle(applied.y+spinOffset);
        P args[]={&applied};invoke(eulerSet,transform,args,ok);
        if(ok){spinApplied=applied;value(eulerGet,transform,nullptr,spinApplied,ok);}
        return ok;
    }
    void restorePlayer(Extra::State& extra){
        restoreTint(extra,body,true);restoreTint(extra,lod,true);
    }
    void clear(Extra::State& extra,bool sameScene){
        restoreTint(extra,sky,sameScene);restoreBackground(extra,sameScene);restoreSpin(extra);
        restoreTint(extra,body,sameScene);restoreTint(extra,lod,sameScene);
        restoreLight(extra,sameScene);nextTick=0;status=@"";
    }
    void tick(Extra::State& extra,double now,bool changeSky,float skyHue,bool changeLight,float hour,float tintHue,
              bool changePlayer,float playerHue,P bodyRenderer,P lodRenderer,P camera){
        using namespace Runtime;
        if(now<nextTick)return;
        nextTick=now+.15;
        if(!initialized)init(extra);
        NSMutableArray* states=[NSMutableArray array];
        if(changeSky)[states addObject:this->changeSky(extra,camera,hue(skyHue,.63f))];
        else {restoreTint(extra,sky,true);restoreBackground(extra,true);}
        if(changeLight)[states addObject:light(extra,hour,tintHue)?@"свет":@"свет: API недоступен"];
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
