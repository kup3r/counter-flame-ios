#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dlfcn.h>
#include <cstring>
#include <vector>
#include <cmath>
#include <utility>
#include <unordered_map>

// In-process prototype. No fixed offsets, hooks, or writes to game state.
namespace Runtime {
using P = void*;
P (*domain_get)();
const P* (*domain_get_assemblies)(P,size_t*);
P (*assembly_get_image)(P);
P (*class_from_name)(P,const char*,const char*);
P (*class_get_methods)(P,P*);
const char* (*method_get_name)(P);
uint32_t (*method_get_param_count)(P);
P (*method_get_param)(P,uint32_t);
char* (*type_get_name)(P);
void (*free_)(P);
P (*class_get_type)(P);
P (*type_get_object)(P);
P (*runtime_invoke)(P,P,P*,P*);
P (*object_unbox)(P);
uintptr_t (*array_length)(P);
int32_t (*class_value_size)(P,uint32_t*);
uint32_t (*gchandle_new)(P,bool);
void (*gchandle_free)(uint32_t);
P (*gchandle_get_target)(uint32_t);
P (*thread_current)();
int32_t (*string_length)(P);
const uint16_t* (*string_chars)(P);
struct Roots {
    std::vector<uint32_t> handles;
    void hold(P p) { if(p) handles.push_back(gchandle_new(p,true)); }
    ~Roots() { for(uint32_t h:handles) gchandle_free(h); }
};
static thread_local Roots* roots=nullptr;
struct RootScope {
    Roots owned; Roots* previous;
    RootScope():previous(roots) { roots=&owned; }
    ~RootScope() { roots=previous; }
};
bool load() {
    void* h=dlopen("@rpath/UnityFramework.framework/UnityFramework",RTLD_LAZY|RTLD_NOLOAD);
    if(!h) return false;
    bool ok=true;
#define LOAD(v,n) v=reinterpret_cast<decltype(v)>(dlsym(h,"il2cpp_" n)); ok=ok && v
    LOAD(domain_get,"domain_get"); LOAD(domain_get_assemblies,"domain_get_assemblies");
    LOAD(assembly_get_image,"assembly_get_image"); LOAD(class_from_name,"class_from_name");
    LOAD(class_get_methods,"class_get_methods"); LOAD(method_get_name,"method_get_name");
    LOAD(method_get_param_count,"method_get_param_count"); LOAD(method_get_param,"method_get_param");
    LOAD(type_get_name,"type_get_name"); LOAD(free_,"free");
    LOAD(class_get_type,"class_get_type"); LOAD(type_get_object,"type_get_object");
    LOAD(runtime_invoke,"runtime_invoke"); LOAD(object_unbox,"object_unbox");
    LOAD(array_length,"array_length"); LOAD(class_value_size,"class_value_size");
    LOAD(gchandle_new,"gchandle_new"); LOAD(gchandle_free,"gchandle_free");
    LOAD(thread_current,"thread_current"); LOAD(gchandle_get_target,"gchandle_get_target");
    LOAD(string_length,"string_length"); LOAD(string_chars,"string_chars");
#undef LOAD
    dlclose(h); return ok;
}
P cls(const char* ns,const char* name) {
    P domain=domain_get(); if(!domain) return nullptr;
    size_t count=0; const P* assemblies=domain_get_assemblies(domain,&count);
    for(size_t i=0;i<count;i++) if(P c=class_from_name(assembly_get_image(assemblies[i]),ns,name)) return c;
    return nullptr;
}
P method(P c,const char* name,uint32_t count,const char* first=nullptr) {
    if(!c) return nullptr;
    P iter=nullptr;
    while(P m=class_get_methods(c,&iter)) {
        if(strcmp(method_get_name(m),name)||method_get_param_count(m)!=count) continue;
        if(first) {
            char* t=type_get_name(method_get_param(m,0));
            bool match=t && !strcmp(t,first); if(t) free_(t);
            if(!match) continue;
        }
        return m;
    }
    return nullptr;
}
P invoke(P m,P self,P* args,bool& ok) {
    if(!ok) return nullptr;
    if(!m){ok=false;return nullptr;}
    P exception=nullptr; P result=runtime_invoke(m,self,args,&exception);
    if(exception) { ok=false; NSLog(@"[Flame v5] Managed exception in %s",method_get_name(m)); }
    if(result && !exception && roots) roots->hold(result);
    return exception?nullptr:result;
}
}

struct V3 {float x,y,z;};
static bool finiteV3(V3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct Bone {uint32_t handle;int parent=-1;};
struct Target {uint32_t player=0,network=0;int actor=-1;std::vector<Bone> bones;};
struct Segment {CGPoint a,b;bool enemy;};
struct Item {CGRect box;NSString* text;bool enemy;};
struct Api {
    Runtime::P player=nullptr,spectatorClass=nullptr;
    Runtime::P active=nullptr,loaded=nullptr,handle=nullptr,find=nullptr,main=nullptr;
    Runtime::P tr=nullptr,pos=nullptr,project=nullptr,get=nullptr,alive=nullptr,width=nullptr,height=nullptr;
    Runtime::P animator=nullptr,skeleton=nullptr,body=nullptr,bodyLod=nullptr,bones=nullptr,parent=nullptr,name=nullptr;
    Runtime::P network=nullptr,local=nullptr,team=nullptr,actor=nullptr,nick=nullptr,health=nullptr,dead=nullptr;
    Runtime::P spectating=nullptr,watched=nullptr;
    bool init(){
        using namespace Runtime;
        player=cls("","PlayerController");spectatorClass=cls("","SpectatorManager");
        P camera=cls("UnityEngine","Camera"),component=cls("UnityEngine","Component"),object=cls("UnityEngine","Object");
        P scene=cls("UnityEngine.SceneManagement","Scene"),transform=cls("UnityEngine","Transform");
        active=method(cls("UnityEngine.SceneManagement","SceneManager"),"GetActiveScene",0);
        loaded=method(scene,"get_isLoaded",0);handle=method(scene,"get_handle",0);
        find=method(object,"FindObjectsOfType",1,"System.Type");main=method(camera,"get_main",0);
        tr=method(component,"get_transform",0);pos=method(transform,"get_position",0);parent=method(transform,"get_parent",0);
        project=method(camera,"WorldToScreenPoint",1,"UnityEngine.Vector3");get=method(cls("System","Array"),"GetValue",1,"System.Int32");
        alive=method(object,"op_Implicit",1,"UnityEngine.Object");name=method(object,"get_name",0);
        width=method(cls("UnityEngine","Screen"),"get_width",0);height=method(cls("UnityEngine","Screen"),"get_height",0);
        animator=method(player,"get_PlayerAnimatorController",0);
        skeleton=method(cls("ScarletWork.Photon.TPS","PlayerAnimatorController"),"get_CurrentSkeleton",0);
        P sk=cls("ScarletWork.Game","PlayerSkeleton");body=method(sk,"get_Body",0);bodyLod=method(sk,"get_BodyLod",0);
        bones=method(cls("UnityEngine","SkinnedMeshRenderer"),"get_bones",0);
        network=method(player,"get_Player",0);health=method(player,"get_Health",0);dead=method(player,"get_IsDeath",0);
        P photon=cls("Photon.Realtime","Player");team=method(photon,"get_Team",0);actor=method(photon,"get_ActorNumber",0);nick=method(photon,"get_NickName",0);
        local=method(cls("Photon.Pun","PhotonNetwork"),"get_LocalPlayer",0);
        spectating=method(spectatorClass,"get_IsSpectating",0);watched=method(spectatorClass,"get_CurrentPlayer",0);
        uint32_t align=0;P v=cls("UnityEngine","Vector3");
        return player&&v&&class_value_size(v,&align)==sizeof(V3)&&active&&loaded&&handle&&find&&main&&tr&&pos&&project&&get&&alive&&width&&height&&animator&&skeleton&&body&&bones&&parent&&network&&local&&team&&actor&&health&&dead;
    }
};
enum Option {Master=1,Enemies,Boxes,Skeleton,Health,Names,Distance,Tracers,Debug};
@interface FlameMarkers:UIView<UIGestureRecognizerDelegate>
@end
@implementation FlameMarkers {
    CADisplayLink* _displayLink;UIView* _panel;UITapGestureRecognizer* _gesture;UISwitch* _master;
    Api _api;BOOL _ready,_apiReady,_enabled,_onlyEnemies,_showBoxes,_showSkeleton,_showHealth,_showNames,_showDistance,_showTracers,_debug;
    std::vector<Target> _targets;std::vector<Segment> _segments,_tracers;std::vector<Item> _items;std::vector<CGPoint> _dots;
    NSString* _status;int _sceneHandle;NSTimeInterval _settledAfter,_nextDiscovery;uint32_t _spectator;
    size_t _rigs,_boneCount,_missingRig;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])){
        self.backgroundColor=UIColor.clearColor;self.userInteractionEnabled=YES;
        self.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        _sceneHandle=-1;_status=@"Flame v5: ESP off";
        _onlyEnemies=YES;_showBoxes=YES;_showSkeleton=YES;_showHealth=YES;_showDistance=YES;
        CGFloat height=fmax(150,fmin(355,frame.size.height-90));
        _panel=[[UIView alloc]initWithFrame:CGRectMake(45,65,285,height)];
        _panel.backgroundColor=[UIColor colorWithWhite:0.06 alpha:0.95];_panel.layer.cornerRadius=14;_panel.hidden=YES;
        UILabel* title=[[UILabel alloc]initWithFrame:CGRectMake(16,8,250,30)];title.text=@"Flame ESP · v5";title.textColor=UIColor.whiteColor;[_panel addSubview:title];
        UIScrollView* scroll=[[UIScrollView alloc]initWithFrame:CGRectMake(0,42,285,height-45)];
        scroll.autoresizingMask=UIViewAutoresizingFlexibleHeight;scroll.contentSize=CGSizeMake(285,390);[_panel addSubview:scroll];
        NSArray* labels=@[@"ESP",@"Только противники",@"Рамки",@"Скелет",@"Здоровье",@"Имена",@"Дистанция",@"Линии",@"Диагностика"];
        for(NSInteger row=0;row<labels.count;row++){
            UILabel* label=[[UILabel alloc]initWithFrame:CGRectMake(16,row*37,205,32)];label.text=labels[row];label.textColor=UIColor.whiteColor;label.font=[UIFont systemFontOfSize:15];[scroll addSubview:label];
            UISwitch* toggle=[[UISwitch alloc]initWithFrame:CGRectMake(222,row*37,50,31)];toggle.tag=row+1;
            toggle.on=row==1||row==2||row==3||row==4||row==6;
            [toggle addTarget:self action:@selector(optionChanged:) forControlEvents:UIControlEventValueChanged];[scroll addSubview:toggle];if(row==0)_master=toggle;
        }
        UISegmentedControl* rate=[[UISegmentedControl alloc]initWithItems:@[@"30 FPS",@"60 FPS"]];rate.frame=CGRectMake(16,343,252,32);rate.selectedSegmentIndex=1;rate.backgroundColor=UIColor.lightGrayColor;
        [rate addTarget:self action:@selector(changeRate:) forControlEvents:UIControlEventValueChanged];[scroll addSubview:rate];[self addSubview:_panel];
        _gesture=[[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(toggleMenu)];_gesture.numberOfTapsRequired=2;_gesture.numberOfTouchesRequired=3;_gesture.cancelsTouchesInView=NO;_gesture.delegate=self;
        _displayLink=[CADisplayLink displayLinkWithTarget:self selector:@selector(sample)];_displayLink.preferredFramesPerSecond=60;
        [_displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];_displayLink.paused=YES;
    }return self;
}
- (void)clearTargets {
    for(const Target& t:_targets){if(t.player)Runtime::gchandle_free(t.player);if(t.network)Runtime::gchandle_free(t.network);for(Bone b:t.bones)Runtime::gchandle_free(b.handle);}_targets.clear();
    if(_spectator){Runtime::gchandle_free(_spectator);_spectator=0;}_nextDiscovery=0;
}
- (void)clearDrawing {_items.clear();_segments.clear();_tracers.clear();_dots.clear();}
- (void)didMoveToWindow {
    [super didMoveToWindow];[_gesture.view removeGestureRecognizer:_gesture];if(self.window)[self.window addGestureRecognizer:_gesture];else [self pause:@"Flame v5: paused"];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)a shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)b{return YES;}
- (UIView*)hitTest:(CGPoint)p withEvent:(UIEvent*)e {UIView* hit=[super hitTest:p withEvent:e];return !_panel.hidden&&[hit isDescendantOfView:_panel]?hit:nil;}
- (void)toggleMenu {_panel.hidden=!_panel.hidden;}
- (void)changeRate:(UISegmentedControl*)c {_displayLink.preferredFramesPerSecond=c.selectedSegmentIndex==0?30:60;}
- (void)optionChanged:(UISwitch*)s {
    switch(s.tag){
        case Master:[self clearTargets];[self clearDrawing];_enabled=s.on;_sceneHandle=-1;_settledAfter=NSProcessInfo.processInfo.systemUptime+5;_displayLink.paused=!_enabled;_status=_enabled?@"Flame v5: waiting for scene":@"Flame v5: ESP off";break;
        case Enemies:_onlyEnemies=s.on;break;case Boxes:_showBoxes=s.on;break;case Skeleton:_showSkeleton=s.on;break;
        case Health:_showHealth=s.on;break;case Names:_showNames=s.on;break;case Distance:_showDistance=s.on;break;case Tracers:_showTracers=s.on;break;case Debug:_debug=s.on;break;
    }[self setNeedsDisplay];
}
- (void)pause:(NSString*)status {_enabled=NO;_master.on=NO;_displayLink.paused=YES;[self clearTargets];[self clearDrawing];_status=status;[self setNeedsDisplay];}
- (BOOL)isAlive:(Runtime::P)object ok:(bool&)ok {
    if(!object||!ok)return NO;Runtime::P args[]={object};Runtime::P b=Runtime::invoke(_api.alive,nullptr,args,ok);return b&&*static_cast<bool*>(Runtime::object_unbox(b));
}
- (NSString*)string:(Runtime::P)object {
    if(!object)return @"";int len=Runtime::string_length(object);if(len<0||len>1024)return @"";
    return [[NSString alloc]initWithCharacters:(const unichar*)Runtime::string_chars(object) length:len]?:@"";
}
- (int)integer:(Runtime::P)box {return box?*static_cast<int*>(Runtime::object_unbox(box)):-1;}
- (BOOL)position:(Runtime::P)transform result:(V3&)value ok:(bool&)ok {
    Runtime::P b=Runtime::invoke(_api.pos,transform,nullptr,ok);if(!b)return NO;memcpy(&value,Runtime::object_unbox(b),sizeof(value));return finiteV3(value);
}
- (BOOL)project:(V3)world camera:(Runtime::P)camera scale:(CGPoint)scale height:(int)height result:(CGPoint&)point ok:(bool&)ok {
    Runtime::P args[]={&world};Runtime::P b=Runtime::invoke(_api.project,camera,args,ok);if(!b)return NO;
    V3 v;memcpy(&v,Runtime::object_unbox(b),sizeof(v));if(!finiteV3(v)||v.z<=0.05f)return NO;
    point=CGPointMake(v.x*scale.x,(height-v.y)*scale.y);return std::isfinite(point.x)&&std::isfinite(point.y);
}
- (void)discover:(bool&)ok {
    using namespace Runtime;[self clearTargets];_rigs=0;_boneCount=0;_missingRig=0;
    if(_api.spectatorClass&&_api.spectating&&_api.watched){
        P type=type_get_object(class_get_type(_api.spectatorClass));P args[]={type};P list=invoke(_api.find,nullptr,args,ok);
        if(list&&array_length(list)>0&&array_length(list)<=8){int zero=0;P idx[]={&zero};P object=invoke(_api.get,list,idx,ok);if([self isAlive:object ok:ok])_spectator=gchandle_new(object,true);}
    }
    P type=type_get_object(class_get_type(_api.player));P args[]={type};P array=invoke(_api.find,nullptr,args,ok);if(!ok||!array)return;
    uintptr_t count=array_length(array);if(count>128){ok=false;return;}
    for(int i=0;i<(int)count&&ok;i++){
        RootScope targetRoots;int idx=i;P arg[]={&idx};P player=invoke(_api.get,array,arg,ok);if(![self isAlive:player ok:ok])continue;
        Target target;target.player=gchandle_new(player,true);
        P network=invoke(_api.network,player,nullptr,ok);if(network){target.network=gchandle_new(network,true);target.actor=[self integer:invoke(_api.actor,network,nullptr,ok)];}
        P animator=invoke(_api.animator,player,nullptr,ok),sk=nullptr,mesh=nullptr;
        if([self isAlive:animator ok:ok])sk=invoke(_api.skeleton,animator,nullptr,ok);
        if([self isAlive:sk ok:ok]){mesh=invoke(_api.body,sk,nullptr,ok);if(![self isAlive:mesh ok:ok]&&ok&&_api.bodyLod)mesh=invoke(_api.bodyLod,sk,nullptr,ok);}
        if([self isAlive:mesh ok:ok]){
            P list=invoke(_api.bones,mesh,nullptr,ok);
            if(list&&array_length(list)<=256){
                std::vector<P> objects;std::unordered_map<P,int> indices;
                for(int j=0;j<(int)array_length(list)&&ok;j++){
                    P index[]={&j};P bone=invoke(_api.get,list,index,ok);if(![self isAlive:bone ok:ok]||indices.count(bone))continue;
                    // Optional naming filter removes fine finger/toe/helper chains; no bone indices are guessed.
                    NSString* name=_api.name?[[self string:invoke(_api.name,bone,nullptr,ok)] lowercaseString]:@"";
                    BOOL fine=NO;for(NSString* token in @[@"finger",@"thumb",@"index",@"middle",@"pinky",@"little",@"toe",@"twist",@"helper",@"weapon",@"ik_"])if([name containsString:token]){fine=YES;break;}
                    if(fine)continue;
                    indices[bone]=(int)objects.size();objects.push_back(bone);target.bones.push_back({gchandle_new(bone,true),-1});
                }
                for(size_t j=0;j<objects.size()&&ok;j++){
                    P p=objects[j];
                    for(int depth=0;depth<8&&ok;depth++){
                        p=invoke(_api.parent,p,nullptr,ok);if(![self isAlive:p ok:ok])break;
                        auto it=indices.find(p);if(it!=indices.end()){if(it->second!=(int)j)target.bones[j].parent=it->second;break;}
                    }
                }
            }
        }
        if(target.bones.size()>=4){_rigs++;_boneCount+=target.bones.size();}else _missingRig++;
        _targets.push_back(std::move(target));
    }
}
- (void)sample {
    using namespace Runtime;[self clearDrawing];if(!_enabled)return;
    if(UIApplication.sharedApplication.applicationState!=UIApplicationStateActive||!self.window){[self pause:@"Flame v5: paused in background"];return;}
    if(!_ready)_ready=load();if(!_ready){_status=@"Flame v5: waiting for runtime";[self setNeedsDisplay];return;}
    if(!NSThread.isMainThread||!thread_current()){[self pause:@"Flame v5: runtime thread unavailable"];return;}
    if(!_apiReady)_apiReady=_api.init();if(!_apiReady){[self pause:@"Flame v5: required methods unavailable"];return;}
    RootScope frameRoots;bool ok=true;
    P scene=invoke(_api.active,nullptr,nullptr,ok),raw=scene?object_unbox(scene):nullptr;
    if(!raw){[self pause:@"Flame v5: no scene"];return;}
    P loaded=invoke(_api.loaded,raw,nullptr,ok),handle=invoke(_api.handle,raw,nullptr,ok);
    if(!ok||!loaded||!handle){[self pause:@"Flame v5: scene check failed"];return;}
    int current=[self integer:handle];NSTimeInterval now=NSProcessInfo.processInfo.systemUptime;
    if(!*static_cast<bool*>(object_unbox(loaded))||(_sceneHandle!=-1&&current!=_sceneHandle)){[self pause:@"Flame v5: scene changed; enable after loading"];return;}
    if(_sceneHandle==-1){_sceneHandle=current;_settledAfter=now+5;}
    if(now<_settledAfter){_status=@"Flame v5: waiting for scene";[self setNeedsDisplay];return;}
    if(now>=_nextDiscovery){[self discover:ok];_nextDiscovery=now+0.5;if(!ok){[self pause:@"Flame v5: discovery exception"];return;}}
    if(!_showBoxes&&!_showSkeleton&&!_showHealth&&!_showNames&&!_showDistance&&!_showTracers){_status=@"Flame v5: choose an ESP feature in menu";[self setNeedsDisplay];return;}
    P camera=invoke(_api.main,nullptr,nullptr,ok);
    if(![self isAlive:camera ok:ok]){if(!ok)[self pause:@"Flame v5: camera exception"];else {_status=@"Flame v5: no camera";[self setNeedsDisplay];}return;}
    int w=[self integer:invoke(_api.width,nullptr,nullptr,ok)],h=[self integer:invoke(_api.height,nullptr,nullptr,ok)];
    if(!ok||w<=0||h<=0){[self pause:@"Flame v5: viewport error"];return;}
    CGPoint scale=CGPointMake(self.bounds.size.width/w,self.bounds.size.height/h);
    P local=invoke(_api.local,nullptr,nullptr,ok),anchor=local;
    int localActor=local?[self integer:invoke(_api.actor,local,nullptr,ok)]:-1;
    BOOL watching=NO;
    if(_spectator){P manager=gchandle_get_target(_spectator);if([self isAlive:manager ok:ok]){
        P state=invoke(_api.spectating,manager,nullptr,ok);
        if(state&&*static_cast<bool*>(object_unbox(state))){P watched=invoke(_api.watched,manager,nullptr,ok);if(watched){anchor=watched;watching=YES;}}
    }}
    int anchorActor=anchor?[self integer:invoke(_api.actor,anchor,nullptr,ok)]:-1;
    NSString* anchorTeam=anchor?[[self string:invoke(_api.team,anchor,nullptr,ok)] lowercaseString]:@"";
    V3 cameraPos{};BOOL haveCameraPos=NO;
    if(_showDistance){P ct=invoke(_api.tr,camera,nullptr,ok);if(ct)haveCameraPos=[self position:ct result:cameraPos ok:ok];}
    size_t filtered=0,unknown=0,visible=0;
    for(const Target& target:_targets){
        RootScope targetRoots;P player=gchandle_get_target(target.player);if(![self isAlive:player ok:ok])continue;
        P isDead=invoke(_api.dead,player,nullptr,ok);if(isDead&&*static_cast<bool*>(object_unbox(isDead)))continue;
        if(target.actor>=0&&(target.actor==localActor||target.actor==anchorActor))continue;
        P network=target.network?gchandle_get_target(target.network):nullptr;
        NSString* team=network?[[self string:invoke(_api.team,network,nullptr,ok)] lowercaseString]:@"";
        BOOL known=team.length>0&&anchorTeam.length>0,enemy=known&&![team isEqualToString:anchorTeam];
        if(_onlyEnemies&&!known){unknown++;continue;}if(_onlyEnemies&&!enemy){filtered++;continue;}
        std::vector<CGPoint> points(target.bones.size());std::vector<bool> valid(target.bones.size(),false);
        CGFloat left=INFINITY,top=INFINITY,right=-INFINITY,bottom=-INFINITY;size_t projected=0;BOOL nearPlane=NO;
        if(_showBoxes||_showSkeleton||_showHealth||_showNames||_showDistance||_showTracers){
            for(size_t j=0;j<target.bones.size()&&ok;j++){
                RootScope boneRoots;P transform=gchandle_get_target(target.bones[j].handle);if(![self isAlive:transform ok:ok])continue;
                V3 pos;if(![self position:transform result:pos ok:ok])continue;CGPoint p;
                if(![self project:pos camera:camera scale:scale height:h result:p ok:ok]){nearPlane=YES;continue;}
                points[j]=p;valid[j]=true;projected++;left=fmin(left,p.x);right=fmax(right,p.x);top=fmin(top,p.y);bottom=fmax(bottom,p.y);
            }
        }
        if(!ok)break;
        if(nearPlane)continue;
        if(projected<4||right<=left||bottom<=top){
            P transform=invoke(_api.tr,player,nullptr,ok);V3 pos;CGPoint p;
            if(transform&&[self position:transform result:pos ok:ok]&&[self project:pos camera:camera scale:scale height:h result:p ok:ok]&&CGRectContainsPoint(self.bounds,p))_dots.push_back(p);
            continue;
        }
        // Padding only covers skin around actual projected joints, not animation-wide renderer bounds.
        CGFloat padX=fmax(2,(right-left)*0.04),padY=fmax(3,(bottom-top)*0.07);
        CGRect box=CGRectMake(left-padX,top-padY,right-left+2*padX,bottom-top+2*padY);
        if(!CGRectIntersectsRect(box,self.bounds))continue;visible++;
        if(_showSkeleton)for(size_t j=0;j<target.bones.size();j++){
            int parent=target.bones[j].parent;if(parent>=0&&parent<(int)points.size()&&valid[j]&&valid[parent])_segments.push_back({points[j],points[parent],(bool)enemy});
        }
        NSMutableArray* parts=[NSMutableArray array];
        if(_showNames&&network&&_api.nick){NSString* nick=[self string:invoke(_api.nick,network,nullptr,ok)];if(nick.length)[parts addObject:nick];}
        if(_showHealth){int hp=[self integer:invoke(_api.health,player,nullptr,ok)];if(hp>=0)[parts addObject:[NSString stringWithFormat:@"HP %d",hp]];}
        if(_showDistance&&haveCameraPos){P tr=invoke(_api.tr,player,nullptr,ok);V3 pos;if(tr&&[self position:tr result:pos ok:ok]){
            double dx=pos.x-cameraPos.x,dy=pos.y-cameraPos.y,dz=pos.z-cameraPos.z;double distance=sqrt(dx*dx+dy*dy+dz*dz);
            if(std::isfinite(distance))[parts addObject:[NSString stringWithFormat:@"%.0f u",distance]];
        }}
        _items.push_back({box,[parts componentsJoinedByString:@" · "],(bool)enemy});
        if(_showTracers)_tracers.push_back({CGPointMake(self.bounds.size.width/2,self.bounds.size.height-8),CGPointMake(CGRectGetMidX(box),CGRectGetMaxY(box)),(bool)enemy});
        if(!ok)break;
    }
    if(!ok){[self pause:@"Flame v5: managed exception; paused"];return;}
    _status=[NSString stringWithFormat:@"v5: %zu players · %@ team %@\nrigs %zu/%zu · bones %zu · missing %zu\nallies filtered %zu · unknown teams %zu",visible,watching?@"watched":@"local",anchorTeam.length?anchorTeam:@"unknown",_rigs,_targets.size(),_boneCount,_missingRig,filtered,unknown];
    [self setNeedsDisplay];
}
- (UIColor*)color:(BOOL)enemy{return enemy?UIColor.cyanColor:[UIColor colorWithRed:0.35 green:1 blue:0.45 alpha:1];}
- (void)stroke:(const std::vector<Segment>&)segments width:(CGFloat)width {
    for(const Segment& s:segments){UIBezierPath* p=[UIBezierPath bezierPath];[p moveToPoint:s.a];[p addLineToPoint:s.b];[UIColor.blackColor setStroke];p.lineWidth=width+1.5;[p stroke];[[self color:s.enemy]setStroke];p.lineWidth=width;[p stroke];}
}
- (void)drawRect:(CGRect)rect {
    CGContextRef context=UIGraphicsGetCurrentContext();CGContextSaveGState(context);CGContextClipToRect(context,self.bounds);
    [self stroke:_tracers width:0.8];[self stroke:_segments width:1.2];
    for(const Item& item:_items){
        if(_showBoxes){UIBezierPath* p=[UIBezierPath bezierPathWithRect:item.box];[UIColor.blackColor setStroke];p.lineWidth=3;[p stroke];[[self color:item.enemy]setStroke];p.lineWidth=1.2;[p stroke];}
        if(item.text.length){NSDictionary* attrs=@{NSForegroundColorAttributeName:[self color:item.enemy],NSFontAttributeName:[UIFont boldSystemFontOfSize:10],NSStrokeColorAttributeName:UIColor.blackColor,NSStrokeWidthAttributeName:@(-2)};
            CGSize size=[item.text sizeWithAttributes:attrs];[item.text drawAtPoint:CGPointMake(CGRectGetMidX(item.box)-size.width/2,fmax(0,item.box.origin.y-size.height-2)) withAttributes:attrs];}
    }
    [UIColor.cyanColor setStroke];for(CGPoint p:_dots){UIBezierPath* path=[UIBezierPath bezierPathWithOvalInRect:CGRectMake(p.x-2,p.y-2,4,4)];path.lineWidth=1.2;[path stroke];}
    CGContextRestoreGState(context);
    if(_debug||!_enabled||!_apiReady||(_onlyEnemies&&_items.empty()))[_status drawInRect:CGRectMake(48,18,480,55) withAttributes:@{NSForegroundColorAttributeName:UIColor.cyanColor,NSFontAttributeName:[UIFont systemFontOfSize:11]}];
}
@end
__attribute__((constructor))static void startFlame(){dispatch_async(dispatch_get_main_queue(),^{
    [NSTimer scheduledTimerWithTimeInterval:1 repeats:YES block:^(NSTimer* timer){for(UIScene* scene in UIApplication.sharedApplication.connectedScenes){
        if(scene.activationState!=UISceneActivationStateForegroundActive||![scene isKindOfClass:UIWindowScene.class])continue;
        for(UIWindow* window in ((UIWindowScene*)scene).windows)if(window.isKeyWindow){[window addSubview:[[FlameMarkers alloc]initWithFrame:window.bounds]];[timer invalidate];return;}
    }}];
});}

