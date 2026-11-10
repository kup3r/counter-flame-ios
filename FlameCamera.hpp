#pragma once

namespace FlameCamera {
using Runtime::P;

struct State {
    P cameraClass=nullptr,gameObjectClass=nullptr,ctor=nullptr,add=nullptr,copy=nullptr,destroy=nullptr;
    P transform=nullptr,positionSet=nullptr,eulerSet=nullptr,depthGet=nullptr,depthSet=nullptr,clearFlags=nullptr;
    P linecast=nullptr,forward=nullptr,eulerGet=nullptr;
    uint32_t object=0,view=0,source=0;
    bool initialized=false,fault=false;

    void init(Extra::State& extra,FlameScene::State& checked){
        using namespace Runtime;
        initialized=true;
        cameraClass=cls("UnityEngine","Camera");gameObjectClass=cls("UnityEngine","GameObject");
        P tr=cls("UnityEngine","Transform");
        ctor=checked.checked(extra,gameObjectClass,".ctor","System.Void",{"System.String"});
        add=checked.checked(extra,gameObjectClass,"AddComponent","UnityEngine.Component",{"System.Type"});
        copy=checked.checked(extra,cameraClass,"CopyFrom","System.Void",{"UnityEngine.Camera"});
        destroy=checked.checked(extra,cls("UnityEngine","Object"),"Destroy","System.Void",{"UnityEngine.Object"});
        transform=checked.checked(extra,cls("UnityEngine","Component"),"get_transform","UnityEngine.Transform");
        positionSet=checked.checked(extra,tr,"set_position","System.Void",{"UnityEngine.Vector3"});
        eulerSet=checked.checked(extra,tr,"set_eulerAngles","System.Void",{"UnityEngine.Vector3"});
        eulerGet=checked.checked(extra,tr,"get_eulerAngles","UnityEngine.Vector3");
        forward=checked.checked(extra,tr,"get_forward","UnityEngine.Vector3");
        depthGet=checked.checked(extra,cameraClass,"get_depth","System.Single");
        depthSet=checked.checked(extra,cameraClass,"set_depth","System.Void",{"System.Single"});
        clearFlags=checked.checked(extra,cameraClass,"set_clearFlags","System.Void",{"UnityEngine.CameraClearFlags"});
        P physics=cls("UnityEngine","Physics");
        if(physics)for(P iter=nullptr,m=nullptr;(m=class_get_methods(physics,&iter));){
            if(!strcmp(method_get_name(m),"Linecast")&&extra.returns(m,"System.Boolean")&&
                extra.parametersMatch(m,{"UnityEngine.Vector3","UnityEngine.Vector3","System.Int32",
                    "UnityEngine.QueryTriggerInteraction"})){linecast=m;break;}
        }
    }
    void clear(Extra::State& extra){
        using namespace Runtime;
        bool ok=true;
        if(object){
            P go=gchandle_get_target(object);
            if(destroy&&extra.alive(go,ok)){P args[]={go};invoke(destroy,nullptr,args,ok);}
            gchandle_free(object);object=0;
        }
        if(view){gchandle_free(view);view=0;}
        if(source){gchandle_free(source);source=0;}
    }
    P sample(Extra::State& extra,FlameScene::State& values,P camera,P cameraTransform,V3 cameraPos,float distance){
        using namespace Runtime;
        if(!initialized)init(extra,values);
        if(fault||!cameraTransform||!extra.object_new||!ctor||!add||!copy||!destroy||!transform||
            !positionSet||!eulerSet||!eulerGet||!forward||!depthGet||!depthSet||!clearFlags||!linecast)return nullptr;
        if(source&&gchandle_get_target(source)!=camera)clear(extra);
        bool ok=true;
        bool newView=!view;
        if(newView){
            P go=extra.object_new(gameObjectClass);
            if(!go)return nullptr;
            object=gchandle_new(go,true);
            P name=extra.string_new("FlameLocalCamera");
            if(roots)roots->hold(name);
            P initArgs[]={name};invoke(ctor,go,initArgs,ok);
            P type=type_get_object(class_get_type(cameraClass));
            if(roots)roots->hold(type);
            P args[]={type};P created=invoke(add,go,args,ok);
            if(!created||!ok){clear(extra);fault=true;return nullptr;}
            view=gchandle_new(created,true);source=gchandle_new(camera,true);
        }
        P target=gchandle_get_target(view);
        if(!extra.alive(target,ok)){clear(extra);return nullptr;}
        if(newView){
            P args[]={camera};invoke(copy,target,args,ok);
            float depth=0;
            if(!values.value(depthGet,camera,nullptr,depth,ok)||!std::isfinite(depth)){clear(extra);return nullptr;}
            depth+=10.f;P depthArg[]={&depth};invoke(depthSet,target,depthArg,ok);
            int flags=1;P clearArg[]={&flags};invoke(clearFlags,target,clearArg,ok);
        }
        if(extra.returns(extra.fovGet,"System.Single")&&extra.returns(extra.fovSet,"System.Void")){
            float fov=0,cloneFov=0;
            if(values.value(extra.fovGet,camera,nullptr,fov,ok)&&values.value(extra.fovGet,target,nullptr,cloneFov,ok)&&
                std::isfinite(fov)&&fabsf(fov-cloneFov)>.01f){
                P args[]={&fov};invoke(extra.fovSet,target,args,ok);
            }
        }
        P targetTransform=invoke(transform,target,nullptr,ok);
        V3 angles{},direction{};
        if(!targetTransform||!values.value(eulerGet,cameraTransform,nullptr,angles,ok)||
            !values.value(forward,cameraTransform,nullptr,direction,ok)||!finiteV3(angles)||!finiteV3(direction)){
            clear(extra);return nullptr;
        }
        V3 wanted{cameraPos.x-direction.x*distance,cameraPos.y-direction.y*distance+.35f,cameraPos.z-direction.z*distance};
        int mask=-5,query=1;P check[]={&cameraPos,&wanted,&mask,&query};
        bool hit=false;
        if(!values.value(linecast,nullptr,check,hit,ok)||hit)wanted=cameraPos;
        P posArg[]={&wanted};invoke(positionSet,targetTransform,posArg,ok);
        P eulerArg[]={&angles};invoke(eulerSet,targetTransform,eulerArg,ok);
        if(!ok){clear(extra);fault=true;return nullptr;}
        return target;
    }
};
}
