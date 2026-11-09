#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dlfcn.h>
#include <cstring>
#include <vector>
#include <cmath>
#include <utility>

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
    if(exception) { ok=false; NSLog(@"[Flame v4] Managed exception in %s",method_get_name(m)); }
    if(result && !exception && roots) roots->hold(result);
    return exception?nullptr:result;
}
}

struct V3 { float x,y,z; };
static bool finiteV3(V3 v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
struct Target { uint32_t player; std::vector<uint32_t> meshes; };
struct Api {
    Runtime::P player=nullptr;
    Runtime::P active=nullptr,loaded=nullptr,handle=nullptr,find=nullptr,main=nullptr;
    Runtime::P tr=nullptr,pos=nullptr,project=nullptr,get=nullptr,alive=nullptr,width=nullptr,height=nullptr;
    Runtime::P animator=nullptr,skeleton=nullptr,body=nullptr,bodyLod=nullptr;
    Runtime::P bounds=nullptr,bmin=nullptr,bmax=nullptr,enabled=nullptr;
    bool init() {
        using namespace Runtime;
        player=cls("","PlayerController");
        P camera=cls("UnityEngine","Camera"),component=cls("UnityEngine","Component");
        P object=cls("UnityEngine","Object"),scene=cls("UnityEngine.SceneManagement","Scene");
        active=method(cls("UnityEngine.SceneManagement","SceneManager"),"GetActiveScene",0);
        loaded=method(scene,"get_isLoaded",0);handle=method(scene,"get_handle",0);
        find=method(object,"FindObjectsOfType",1,"System.Type");main=method(camera,"get_main",0);
        tr=method(component,"get_transform",0);pos=method(cls("UnityEngine","Transform"),"get_position",0);
        project=method(camera,"WorldToScreenPoint",1,"UnityEngine.Vector3");
        get=method(cls("System","Array"),"GetValue",1,"System.Int32");
        alive=method(object,"op_Implicit",1,"UnityEngine.Object");
        width=method(cls("UnityEngine","Screen"),"get_width",0);height=method(cls("UnityEngine","Screen"),"get_height",0);
        animator=method(player,"get_PlayerAnimatorController",0);
        skeleton=method(cls("ScarletWork.Photon.TPS","PlayerAnimatorController"),"get_CurrentSkeleton",0);
        body=method(cls("ScarletWork.Game","PlayerSkeleton"),"get_Body",0);
        bodyLod=method(cls("ScarletWork.Game","PlayerSkeleton"),"get_BodyLod",0);
        bounds=method(cls("UnityEngine","Renderer"),"get_bounds",0);
        enabled=method(cls("UnityEngine","Renderer"),"get_enabled",0);
        bmin=method(cls("UnityEngine","Bounds"),"get_min",0);bmax=method(cls("UnityEngine","Bounds"),"get_max",0);
        uint32_t align=0;P v=cls("UnityEngine","Vector3");
        return player&&v&&class_value_size(v,&align)==sizeof(V3)&&active&&loaded&&handle&&find&&main&&tr&&pos&&project&&get&&alive&&width&&height&&animator&&skeleton&&body&&bounds&&bmin&&bmax;
    }
};
@interface FlameMarkers : UIView <UIGestureRecognizerDelegate>
@end
@implementation FlameMarkers {
    CADisplayLink* _displayLink;
    std::vector<CGPoint> _points;
    std::vector<CGRect> _boxes;
    std::vector<Target> _targets;
    Api _api;
    NSString* _status;
    BOOL _ready,_apiReady,_enabled;
    UIView* _panel;
    UITapGestureRecognizer* _menuGesture;
    UISwitch* _markerToggle;
    int _sceneHandle;
    NSTimeInterval _settledAfter,_nextDiscovery;
    size_t _bodyCount,_noAnimator,_noSkeleton,_noBody;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.backgroundColor=UIColor.clearColor; self.userInteractionEnabled=YES;
        self.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        _status=@"Flame v4: ESP off"; _sceneHandle=-1;
        _panel=[[UIView alloc] initWithFrame:CGRectMake(55,55,245,160)];
        _panel.backgroundColor=[UIColor colorWithWhite:0.08 alpha:0.95]; _panel.layer.cornerRadius=12;_panel.hidden=YES;
        UILabel* title=[[UILabel alloc] initWithFrame:CGRectMake(15,10,215,25)];
        title.text=@"Flame · ESP v4";title.textColor=UIColor.whiteColor;[_panel addSubview:title];
        UILabel* label=[[UILabel alloc] initWithFrame:CGRectMake(15,53,145,30)];
        label.text=@"ESP boxes";label.textColor=UIColor.whiteColor;[_panel addSubview:label];
        _markerToggle=[[UISwitch alloc] initWithFrame:CGRectMake(175,53,50,30)];
        [_markerToggle addTarget:self action:@selector(changeMarkers:) forControlEvents:UIControlEventValueChanged];
        [_panel addSubview:_markerToggle];
        UISegmentedControl* rate=[[UISegmentedControl alloc] initWithItems:@[@"30 FPS",@"60 FPS"]];
        rate.frame=CGRectMake(15,108,215,32);rate.selectedSegmentIndex=1;
        rate.backgroundColor=UIColor.lightGrayColor;
        [rate addTarget:self action:@selector(changeRate:) forControlEvents:UIControlEventValueChanged];[_panel addSubview:rate];
        [self addSubview:_panel];
        _menuGesture=[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggleMenu)];
        _menuGesture.numberOfTapsRequired=2;_menuGesture.numberOfTouchesRequired=3;
        _menuGesture.cancelsTouchesInView=NO;_menuGesture.delegate=self;
        _displayLink=[CADisplayLink displayLinkWithTarget:self selector:@selector(sample)];
        _displayLink.preferredFramesPerSecond=60;
        [_displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
        _displayLink.paused=YES;
    }return self;
}
- (void)clearTargets {
    for(const Target& t:_targets) {
        Runtime::gchandle_free(t.player);
        for(uint32_t h:t.meshes) Runtime::gchandle_free(h);
    }_targets.clear();_nextDiscovery=0;
}
- (void)didMoveToWindow {
    [super didMoveToWindow];[_menuGesture.view removeGestureRecognizer:_menuGesture];
    if(self.window) [self.window addGestureRecognizer:_menuGesture];
    else { _enabled=NO;_markerToggle.on=NO;_displayLink.paused=YES;[self clearTargets]; }
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)a shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)b { return YES; }
- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
    UIView* hit=[super hitTest:point withEvent:event];return !_panel.hidden&&[hit isDescendantOfView:_panel]?hit:nil;
}
- (void)toggleMenu { _panel.hidden=!_panel.hidden; }
- (void)changeRate:(UISegmentedControl*)control { _displayLink.preferredFramesPerSecond=control.selectedSegmentIndex==0?30:60; }
- (void)changeMarkers:(UISwitch*)toggle {
    [self clearTargets];_points.clear();_boxes.clear();_enabled=toggle.on;_sceneHandle=-1;
    _settledAfter=NSProcessInfo.processInfo.systemUptime+5;_displayLink.paused=!_enabled;
    _status=_enabled?@"Flame v4: waiting for scene":@"Flame v4: ESP off";[self setNeedsDisplay];
}
- (void)pauseWithStatus:(NSString*)status {
    _enabled=NO;_markerToggle.on=NO;_displayLink.paused=YES;
    [self clearTargets];_points.clear();_boxes.clear();_status=status;[self setNeedsDisplay];
}
- (BOOL)isAlive:(Runtime::P)object ok:(bool&)ok {
    if(!object||!ok)return NO;
    Runtime::P args[]={object};Runtime::P box=Runtime::invoke(_api.alive,nullptr,args,ok);
    return box&&*static_cast<bool*>(Runtime::object_unbox(box));
}
- (void)discover:(bool&)ok {
    using namespace Runtime;
    [self clearTargets];
    _bodyCount=0;_noAnimator=0;_noSkeleton=0;_noBody=0;
    P type=type_get_object(class_get_type(_api.player));P args[]={type};P array=invoke(_api.find,nullptr,args,ok);
    if(!ok||!array)return;
    uintptr_t count=array_length(array);if(count>128){ok=false;return;}
    for(int i=0;i<(int)count&&ok;i++) {
        P index[]={&i};P player=invoke(_api.get,array,index,ok);
        if(![self isAlive:player ok:ok])continue;
        Target target{gchandle_new(player,true),{}};
        // These exact property getters exist in this IPA's type/method metadata.
        P animator=invoke(_api.animator,player,nullptr,ok);
        if([self isAlive:animator ok:ok]) {
            P skeleton=invoke(_api.skeleton,animator,nullptr,ok);
            if([self isAlive:skeleton ok:ok]) {
                P mesh=invoke(_api.body,skeleton,nullptr,ok);
                if(![self isAlive:mesh ok:ok]&&ok&&_api.bodyLod)mesh=invoke(_api.bodyLod,skeleton,nullptr,ok);
                if([self isAlive:mesh ok:ok]) {target.meshes.push_back(gchandle_new(mesh,true));_bodyCount++;}
                else _noBody++;
            }else _noSkeleton++;
        }else _noAnimator++;
        _targets.push_back(std::move(target));
    }
}
- (void)sample {
    using namespace Runtime;
    _points.clear();_boxes.clear();
    if(!_enabled)return;
    if(UIApplication.sharedApplication.applicationState!=UIApplicationStateActive||!self.window) {
        [self pauseWithStatus:@"Flame v4: paused in background"];return;
    }
    if(!_ready)_ready=load();
    if(!_ready){_status=@"Flame v4: waiting for runtime";[self setNeedsDisplay];return;}
    if(!NSThread.isMainThread||!thread_current()) {[self pauseWithStatus:@"Flame v4: runtime thread unavailable"];return;}
    if(!_apiReady)_apiReady=_api.init();
    if(!_apiReady){[self pauseWithStatus:@"Flame v4: required methods unavailable"];return;}
    RootScope frameRoots;bool ok=true;
    P scene=invoke(_api.active,nullptr,nullptr,ok);
    P sceneValue=scene?object_unbox(scene):nullptr;
    if(!sceneValue){[self pauseWithStatus:@"Flame v4: no scene"];return;}
    P loaded=invoke(_api.loaded,sceneValue,nullptr,ok),handle=invoke(_api.handle,sceneValue,nullptr,ok);
    if(!ok||!loaded||!handle){[self pauseWithStatus:@"Flame v4: scene check failed"];return;}
    bool isLoaded=*static_cast<bool*>(object_unbox(loaded));int current=*static_cast<int*>(object_unbox(handle));
    NSTimeInterval now=NSProcessInfo.processInfo.systemUptime;
    if(!isLoaded||(_sceneHandle!=-1&&current!=_sceneHandle)) {
        [self pauseWithStatus:@"Flame v4: scene changed; enable after loading"];return;
    }
    if(_sceneHandle==-1){_sceneHandle=current;_settledAfter=now+5;}
    if(now<_settledAfter){_status=@"Flame v4: waiting for scene";[self setNeedsDisplay];return;}
    if(now>=_nextDiscovery) {
        [self discover:ok];_nextDiscovery=now+0.5;
        if(!ok){[self pauseWithStatus:@"Flame v4: discovery exception"];return;}
    }
    P camera=invoke(_api.main,nullptr,nullptr,ok);
    if(![self isAlive:camera ok:ok]){_status=@"Flame v4: no camera";[self setNeedsDisplay];return;}
    P wb=invoke(_api.width,nullptr,nullptr,ok),hb=invoke(_api.height,nullptr,nullptr,ok);
    if(!ok||!wb||!hb){[self pauseWithStatus:@"Flame v4: viewport error"];return;}
    int w=*static_cast<int*>(object_unbox(wb)),h=*static_cast<int*>(object_unbox(hb));
    if(w<=0||h<=0)return;
    CGFloat sx=self.bounds.size.width/w,sy=self.bounds.size.height/h;
    size_t disabled=0,emptyBounds=0,clipped=0;
    for(const Target& target:_targets) {
        // Each target's temporary boxes and vectors are released after that target.
        RootScope targetRoots;
        P player=gchandle_get_target(target.player);
        if(![self isAlive:player ok:ok])continue;
        V3 lo{INFINITY,INFINITY,INFINITY},hi{-INFINITY,-INFINITY,-INFINITY};bool haveBounds=false;
        for(uint32_t handle:target.meshes) {
            P mesh=gchandle_get_target(handle);if(![self isAlive:mesh ok:ok])continue;
            if(_api.enabled) {
                P enabled=invoke(_api.enabled,mesh,nullptr,ok);
                if(enabled&&!*static_cast<bool*>(object_unbox(enabled)))disabled++;
            }
            P b=invoke(_api.bounds,mesh,nullptr,ok);if(!b)continue;
            P raw=object_unbox(b);if(!raw){ok=false;break;}
            P mn=invoke(_api.bmin,raw,nullptr,ok),mx=invoke(_api.bmax,raw,nullptr,ok);
            if(!mn||!mx)continue;
            V3 a,c;memcpy(&a,object_unbox(mn),sizeof(a));memcpy(&c,object_unbox(mx),sizeof(c));
            if(!finiteV3(a)||!finiteV3(c)||a.x>=c.x||a.y>=c.y||a.z>=c.z){emptyBounds++;continue;}
            lo={fminf(lo.x,a.x),fminf(lo.y,a.y),fminf(lo.z,a.z)};
            hi={fmaxf(hi.x,c.x),fmaxf(hi.y,c.y),fmaxf(hi.z,c.z)};haveBounds=true;
        }
        if(!ok)break;
        if(haveBounds) {
            CGFloat left=INFINITY,top=INFINITY,right=-INFINITY,bottom=-INFINITY;bool visible=true;
            for(int c=0;c<8;c++) {
                V3 world{c&1?hi.x:lo.x,c&2?hi.y:lo.y,c&4?hi.z:lo.z};P args[]={&world};
                P box=invoke(_api.project,camera,args,ok);if(!box){visible=false;break;}
                V3 v;memcpy(&v,object_unbox(box),sizeof(v));
                // Reject boxes crossing the near plane instead of drawing huge rectangles.
                if(!finiteV3(v)||v.z<=0.05f){visible=false;break;}
                CGFloat x=v.x*sx,y=(h-v.y)*sy;
                left=fmin(left,x);right=fmax(right,x);top=fmin(top,y);bottom=fmax(bottom,y);
            }
            if(visible&&right>left&&bottom>top) {
                CGRect rect=CGRectMake(left,top,right-left,bottom-top);
                if(CGRectIntersectsRect(rect,self.bounds))_boxes.push_back(rect);
            }else clipped++;
        }else {
            // Missing model bounds are shown as a dot, never an invented-size box.
            P transform=invoke(_api.tr,player,nullptr,ok);if(!transform)continue;
            P pos=invoke(_api.pos,transform,nullptr,ok);if(!pos)continue;
            V3 world;memcpy(&world,object_unbox(pos),sizeof(world));if(!finiteV3(world))continue;
            P args[]={&world};P box=invoke(_api.project,camera,args,ok);if(!box)continue;
            V3 v;memcpy(&v,object_unbox(box),sizeof(v));
            if(finiteV3(v)&&v.z>0&&v.x>=0&&v.x<=w&&v.y>=0&&v.y<=h)_points.push_back(CGPointMake(v.x*sx,(h-v.y)*sy));
        }
        if(!ok)break;
    }
    if(!ok){[self pauseWithStatus:@"Flame v4: managed exception; paused"];return;}
    _status=[NSString stringWithFormat:@"v4: %zu boxes / %zu dots · bodies %zu/%zu\nmissing animator/skeleton/body: %zu/%zu/%zu\nhidden %zu · empty bounds %zu · clipped %zu",_boxes.size(),_points.size(),_bodyCount,_targets.size(),_noAnimator,_noSkeleton,_noBody,disabled,emptyBounds,clipped];
    [self setNeedsDisplay];
}
- (void)drawRect:(CGRect)rect {
    CGContextRef context=UIGraphicsGetCurrentContext();
    CGContextSaveGState(context);CGContextClipToRect(context,self.bounds);
    for(CGRect box:_boxes) {
        [UIColor.blackColor setStroke];UIBezierPath* p=[UIBezierPath bezierPathWithRect:box];p.lineWidth=3;[p stroke];
        [UIColor.cyanColor setStroke];p.lineWidth=1.3;[p stroke];
    }
    [UIColor.cyanColor setStroke];
    for(CGPoint point:_points){UIBezierPath* p=[UIBezierPath bezierPathWithOvalInRect:CGRectMake(point.x-3,point.y-3,6,6)];p.lineWidth=1.5;[p stroke];}
    CGContextRestoreGState(context);
    [_status drawInRect:CGRectMake(55,20,440,55) withAttributes:@{NSForegroundColorAttributeName:UIColor.cyanColor,NSFontAttributeName:[UIFont systemFontOfSize:11]}];
}
@end

__attribute__((constructor)) static void startFlame() {
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSTimer scheduledTimerWithTimeInterval:1 repeats:YES block:^(NSTimer* timer) {
            for(UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
                if(scene.activationState!=UISceneActivationStateForegroundActive||![scene isKindOfClass:UIWindowScene.class])continue;
                for(UIWindow* window in ((UIWindowScene*)scene).windows)if(window.isKeyWindow) {
                    [window addSubview:[[FlameMarkers alloc] initWithFrame:window.bounds]];[timer invalidate];return;
                }
            }
        }];
    });
}

