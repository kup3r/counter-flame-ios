#pragma once

namespace FlameMovement {
using namespace Runtime;

struct State {
    bool autoStrafe=false,edgeJump=false,unlimitedDuck=false,initialized=false;
    P movementGet=nullptr,coreField=nullptr,tuningField=nullptr,ground=nullptr,ladder=nullptr,jump=nullptr,linecast=nullptr;
    P velocityField=nullptr,originField=nullptr,frictionField=nullptr,groundMask=nullptr,holdField=nullptr,defusingField=nullptr,freezeField=nullptr,waterField=nullptr;
    P maxSpeed=nullptr,airCap=nullptr,accelField=nullptr,airAccelerate=nullptr,forwardGet=nullptr;
    P duckModifier=nullptr,duckPenalty=nullptr,duckInterval=nullptr;
    double previous=0,nextJump=0;
    bool readBool(Extra::State& extra,P move,P field){
        bool value=false;
        if(field)extra.read(move,field,&value);
        return value;
    }
    void init(Extra::State& extra,FlameScene::State& c){
        initialized=true;
        P pc=cls("","PlayerController"),pm=cls("","PlayerMovement"),core=cls("","SourceGameMovement");
        P tuning=cls("","SourceMovementTuning");
        movementGet=c.checked(extra,pc,"get_MovementController","PlayerMovement");
        coreField=extra.typedField(pm,"_core","SourceGameMovement");
        tuningField=extra.typedField(pm,"_movement","SourceMovementTuning");
        velocityField=extra.typedField(core,"Velocity","UnityEngine.Vector3");
        originField=extra.typedField(core,"Origin","UnityEngine.Vector3");
        frictionField=extra.typedField(core,"SurfaceFriction","System.Single");
        groundMask=extra.typedField(pm,"_groundProbeMask","System.Int32");
        holdField=extra.typedField(pm,"_held","System.Boolean");
        defusingField=extra.typedField(pm,"_defusing","System.Boolean");
        freezeField=extra.typedField(pm,"_isFreezing","System.Boolean");
        waterField=extra.typedField(pm,"_waterLevel","System.Int32");
        duckModifier=extra.typedField(tuning,"DuckModifier","System.Single");
        duckPenalty=extra.typedField(tuning,"DuckSpamPenalty","System.Single");
        duckInterval=extra.typedField(tuning,"TimeBetweenDucks","System.Single");
        ground=c.checked(extra,pm,"get_IsGround","System.Boolean");
        ladder=c.checked(extra,pm,"get_IsOnLadder","System.Boolean");
        jump=c.checked(extra,pm,"JumpEvent","System.Void");
        maxSpeed=c.checked(extra,pm,"get_MaxPlayerSpeed","System.Single");
        airCap=c.checked(extra,tuning,"get_AirWishSpeedCapMeters","System.Single");
        accelField=extra.typedField(tuning,"AirAccelerate","System.Single");
        airAccelerate=c.checked(extra,cls("","SourceMove"),"AirAccelerate","System.Void",
            {"UnityEngine.Vector3&","UnityEngine.Vector3","System.Single","System.Single","System.Single","System.Single","System.Single"});
        forwardGet=c.checked(extra,cls("UnityEngine","Transform"),"get_forward","UnityEngine.Vector3");
        uint32_t alignment=0;
        P query=cls("UnityEngine","QueryTriggerInteraction");
        if(query&&class_value_size(query,&alignment)==sizeof(int))
            linecast=c.checked(extra,cls("UnityEngine","Physics"),"Linecast","System.Boolean",
                {"UnityEngine.Vector3","UnityEngine.Vector3","System.Int32","UnityEngine.QueryTriggerInteraction"});
    }
    bool groundBelow(FlameScene::State& c,V3 point,int mask,bool& hit,bool& ok){
        V3 from=point,to=point;
        from.y+=.06f;to.y-=.24f;
        int ignoreTriggers=1;
        P args[]={&from,&to,&mask,&ignoreTriggers};
        return c.value(linecast,nullptr,args,hit,ok);
    }
    void tryJump(Extra::State& extra,FlameScene::State& c,P move,P core,V3 speed,float horizontal,double now,bool& ok){
        if(!jump||!originField||!groundMask||now<nextJump||horizontal<1.f)return;
        int mask=0;V3 feet{};
        extra.read(move,groundMask,&mask);extra.read(core,originField,&feet);
        if(!mask||!finiteV3(feet))return;
        float lead=std::clamp(horizontal*.04f,.14f,.28f)/horizontal;
        V3 ahead={feet.x+speed.x*lead,feet.y,feet.z+speed.z*lead};
        bool here=false,there=true;
        if(groundBelow(c,feet,mask,here,ok)&&here&&groundBelow(c,ahead,mask,there,ok)&&!there){
            invoke(jump,move,nullptr,ok);
            nextJump=now+.22;
        }
    }
    void strafe(Extra::State& extra,FlameScene::State& c,P move,P core,P tuning,P camera,V3 speed,float horizontal,float dt,bool& ok){
        if(!camera||!tuning||!airAccelerate||!frictionField||!accelField||!maxSpeed||!airCap||horizontal>35.f)return;
        float wish=0,cap=0,accel=0,friction=0;
        if(!c.value(maxSpeed,move,nullptr,wish,ok)||!c.value(airCap,tuning,nullptr,cap,ok))return;
        extra.read(core,frictionField,&friction);extra.read(tuning,accelField,&accel);
        if(!(wish>0&&wish<35&&cap>0&&cap<=8&&accel>0&&accel<=100&&friction>0&&friction<=2))return;
        V3 look{};
        if(!c.value(forwardGet,camera,nullptr,look,ok)||!finiteV3(look))return;
        float lookLength=hypotf(look.x,look.z);
        if(lookLength<.05f)return;
        V3 dir={look.x/lookLength,0,look.z/lookLength};
        if(horizontal>.1f){
            float vx=speed.x/horizontal,vz=speed.z/horizontal;
            float along=std::clamp((std::min(cap,wish)-accel*wish*friction*dt)/horizontal,0.f,1.f);
            float across=sqrtf(1.f-along*along);
            if(-vz*dir.x+vx*dir.z<0)across=-across;
            dir={vx*along-vz*across,0,vz*along+vx*across};
        }
        P args[]={&speed,&dir,&wish,&cap,&accel,&friction,&dt};
        invoke(airAccelerate,nullptr,args,ok);
        if(ok&&finiteV3(speed)&&hypotf(speed.x,speed.z)<=35.f)extra.write(core,velocityField,&speed);
    }
    void clear(Extra::State& extra){
        if(extra.ready)extra.patches.restore(4);
        previous=nextJump=0;
    }
    void tick(Extra::State& extra,FlameScene::State& c,P player,P camera,double now,bool inputActive){
        float dt=previous?(float)std::clamp(now-previous,0.,1./30.):0;
        previous=now;
        if(!player||(!autoStrafe&&!edgeJump&&!unlimitedDuck))return;
        if(!initialized)init(extra,c);
        if(!movementGet)return;
        bool ok=true;
        P move=invoke(movementGet,player,nullptr,ok);
        if(!extra.alive(move,ok))return;
        P tuning=extra.objectField(move,tuningField);
        if(unlimitedDuck&&tuning&&duckModifier&&duckPenalty&&duckInterval){
            float full=1.f,zero=0.f;
            extra.patches.set(tuning,duckModifier,&full,sizeof(full),4);
            extra.patches.set(tuning,duckPenalty,&zero,sizeof(zero),4);
            extra.patches.set(tuning,duckInterval,&zero,sizeof(zero),4);
        }
        if(!inputActive||dt<=0||!velocityField||!holdField||!defusingField||!freezeField||!waterField||
            readBool(extra,move,holdField)||readBool(extra,move,defusingField)||readBool(extra,move,freezeField))return;
        int water=0;extra.read(move,waterField,&water);if(water>=2)return;
        P core=extra.objectField(move,coreField);
        if(!core)return;
        bool grounded=false,onLadder=false;
        if(!c.value(ground,move,nullptr,grounded,ok)||!c.value(ladder,move,nullptr,onLadder,ok)||onLadder)return;
        V3 speed{};extra.read(core,velocityField,&speed);
        if(!finiteV3(speed))return;
        float horizontal=hypotf(speed.x,speed.z);
        if(grounded&&edgeJump)tryJump(extra,c,move,core,speed,horizontal,now,ok);
        if(!grounded&&autoStrafe)strafe(extra,c,move,core,tuning,camera,speed,horizontal,dt,ok);
    }
};
}
