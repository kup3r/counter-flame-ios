#pragma once

namespace FlameNades {
using namespace Runtime;

struct State {
    enum Action {None,Save,Next,Stand,Align,Remove};
    bool enabled=false,initialized=false;
    Action pending=None;
    P sceneName=nullptr,forward=nullptr,teleport=nullptr;
    NSString* map=nil;
    NSMutableArray<NSArray<NSNumber*>*>* lineups=nil;
    NSString* status=@"";
    NSUInteger selected=0;
    V3 stand{},aim{};
    float distance=0;
    bool visible=false;

    void clear(){pending=None;map=nil;lineups=nil;selected=0;visible=false;status=@"";}
    void save(){
        if(map.length)[NSUserDefaults.standardUserDefaults setObject:lineups
            forKey:[@"flame.v12.lineups." stringByAppendingString:map]];
    }
    void load(NSString* name){
        map=[name copy];selected=0;
        NSString* key=[@"flame.v12.lineups." stringByAppendingString:map];
        lineups=[NSMutableArray array];
        for(id item in [NSUserDefaults.standardUserDefaults arrayForKey:key]){
            if(lineups.count>=24)break;
            if(![item isKindOfClass:NSArray.class]||[item count]!=6)continue;
            bool valid=true;
            for(id n in item)if(![n isKindOfClass:NSNumber.class]||!std::isfinite([n floatValue]))valid=false;
            if(valid)[lineups addObject:item];
        }
    }
    void tick(Extra::State& extra,FlameScene::State& c,FlameRoundPractice::State& practice,
              P scene,P player,P camera,V3 cameraPos,P position){
        visible=false;
        if(!enabled||!scene||!player||!camera)return;
        if(!initialized){
            initialized=true;
            sceneName=c.checked(extra,cls("UnityEngine.SceneManagement","Scene"),"get_name","System.String");
            forward=c.checked(extra,cls("UnityEngine","Transform"),"get_forward","UnityEngine.Vector3");
            teleport=c.checked(extra,cls("","PlayerMovement"),"Teleport","System.Void",{"UnityEngine.Vector3"});
        }
        bool ok=true;
        P name=invoke(sceneName,scene,nullptr,ok);
        if(!name){status=@"Раскидки: имя карты недоступно";return;}
        int length=string_length(name);
        if(length<=0||length>128)return;
        NSString* current=[[NSString alloc]initWithCharacters:string_chars(name) length:length];
        if(![map isEqualToString:current])load(current);
        if(!c.initialized)c.init(extra);
        P move=extra.movement?invoke(extra.movement,player,nullptr,ok):nullptr;
        if(!extra.alive(move,ok))return;
        P transform=invoke(c.transformGet,move,nullptr,ok);
        V3 origin{};
        if(!transform||!c.value(position,transform,nullptr,origin,ok)||!finiteV3(origin))return;
        Action action=pending;pending=None;
        if(action==Save){
            V3 dir{};
            if(lineups.count>=24){status=@"Раскидки: на карте уже 24 точки";return;}
            if(!c.value(forward,camera,nullptr,dir,ok)||!finiteV3(dir)||!finiteV3(cameraPos))return;
            V3 target={cameraPos.x+20*dir.x,cameraPos.y+20*dir.y,cameraPos.z+20*dir.z};
            [lineups addObject:@[@(origin.x),@(origin.y),@(origin.z),@(target.x),@(target.y),@(target.z)]];
            selected=lineups.count-1;save();
        }
        if(!lineups.count){status=@"Раскидки: сохрани свою первую точку на карте";return;}
        if(action==Next)selected=(selected+1)%lineups.count;
        if(action==Remove){
            [lineups removeObjectAtIndex:selected];save();
            if(!lineups.count){selected=0;status=@"Раскидки: точка удалена";return;}
            selected=std::min(selected,lineups.count-1);
        }
        NSArray<NSNumber*>* point=lineups[selected];
        stand={point[0].floatValue,point[1].floatValue,point[2].floatValue};
        aim={point[3].floatValue,point[4].floatValue,point[5].floatValue};
        distance=sqrtf((stand.x-origin.x)*(stand.x-origin.x)+(stand.y-origin.y)*(stand.y-origin.y)+(stand.z-origin.z)*(stand.z-origin.z));
        visible=true;
        status=[NSString stringWithFormat:@"Раскидка %lu/%lu · %.2f м",(unsigned long)selected+1,(unsigned long)lineups.count,distance];
        if(action!=Stand&&action!=Align)return;
        if(!practice.initialized)practice.init(extra,c);
        if(!practice.gate(c)){status=@"Раскидки: автопозиция и наводка доступны в одиночной тренировке";return;}
        if(action==Align){
            if(distance>.5f){status=@"Раскидки: сначала встань на отметку";return;}
            extra.aim(player,camera,cameraPos,aim,1.f,180.f,ok);
        }else if(teleport){
            P args[]={&stand};invoke(teleport,move,args,ok);
        }else ok=false;
        if(!ok)status=@"Раскидки: игра отклонила действие";
    }
};
}
