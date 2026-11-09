#import <UIKit/UIKit.h>
#import <dlfcn.h>
#include <cstring>
#include <vector>
#include <cmath>

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
    LOAD(thread_current,"thread_current");
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
    if(exception) { ok=false; NSLog(@"[Flame v2] Managed exception in %s",method_get_name(m)); }
    if(result && !exception && roots) roots->hold(result);
    return exception?nullptr:result;
}
}

struct V3 { float x,y,z; };
@interface FlameMarkers : UIView <UIGestureRecognizerDelegate>
@end
@implementation FlameMarkers {
    NSTimer* _timer;
    std::vector<CGPoint> _points;
    NSString* _status;
    BOOL _ready;
    BOOL _enabled;
    UIView* _panel;
    UITapGestureRecognizer* _menuGesture;
    UISwitch* _markerToggle;
    int _sceneHandle;
    NSTimeInterval _settledAfter;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])) {
        self.backgroundColor=UIColor.clearColor; self.userInteractionEnabled=YES;
        self.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        _status=@"Flame: waiting for IL2CPP";
        _enabled=NO; _sceneHandle=-1;
        _panel=[[UIView alloc] initWithFrame:CGRectMake(55,55,245,110)];
        _panel.backgroundColor=[UIColor colorWithWhite:0.08 alpha:0.95];
        _panel.layer.cornerRadius=12; _panel.hidden=YES;
        UILabel* title=[[UILabel alloc] initWithFrame:CGRectMake(15,10,215,25)];
        title.text=@"Flame · diagnostic v2"; title.textColor=UIColor.whiteColor;
        [_panel addSubview:title];
        UILabel* label=[[UILabel alloc] initWithFrame:CGRectMake(15,53,145,30)];
        label.text=@"Player markers"; label.textColor=UIColor.whiteColor;
        [_panel addSubview:label];
        UISwitch* toggle=[[UISwitch alloc] initWithFrame:CGRectMake(175,53,50,30)];
        _markerToggle=toggle;
        toggle.on=NO; [toggle addTarget:self action:@selector(changeMarkers:) forControlEvents:UIControlEventValueChanged];
        [_panel addSubview:toggle]; [self addSubview:_panel];
        _menuGesture=[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(toggleMenu)];
        _menuGesture.numberOfTapsRequired=2; _menuGesture.numberOfTouchesRequired=3;
        _menuGesture.cancelsTouchesInView=NO; _menuGesture.delegate=self;
        _timer=[NSTimer scheduledTimerWithTimeInterval:0.5 target:self selector:@selector(sample) userInfo:nil repeats:YES];
    } return self;
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    [_menuGesture.view removeGestureRecognizer:_menuGesture];
    if(self.window) [self.window addGestureRecognizer:_menuGesture];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)a shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)b { return YES; }
- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
    UIView* hit=[super hitTest:point withEvent:event];
    return !_panel.hidden && [hit isDescendantOfView:_panel] ? hit : nil;
}
- (void)toggleMenu { _panel.hidden=!_panel.hidden; }
- (void)changeMarkers:(UISwitch*)toggle {
    _enabled=toggle.on; _sceneHandle=-1;
    _settledAfter=NSProcessInfo.processInfo.systemUptime+5;
    NSLog(@"[Flame v2] Markers %@",_enabled?@"enabled":@"disabled");
}
- (void)sample {
    using namespace Runtime;
    _points.clear();
    if(!_enabled){_status=@"Flame: markers off";[self setNeedsDisplay];return;}
    if(!_ready) _ready=load();
    if(!_ready){[self setNeedsDisplay];return;}
    if(!NSThread.isMainThread || !thread_current()) {
        _status=@"Flame v2: runtime thread unavailable";[self setNeedsDisplay];return;
    }
    if(UIApplication.sharedApplication.applicationState!=UIApplicationStateActive || !self.window) return;
    RootScope managedRoots;
    bool ok=true;
    P sceneClass=cls("UnityEngine.SceneManagement","Scene");
    P active=method(cls("UnityEngine.SceneManagement","SceneManager"),"GetActiveScene",0);
    P loaded=method(sceneClass,"get_isLoaded",0), handle=method(sceneClass,"get_handle",0);
    if(!active || !loaded || !handle) {
        _status=@"Flame v2: scene methods unavailable";[self setNeedsDisplay];return;
    }
    P scene=invoke(active,nullptr,nullptr,ok);
    if(!ok || !scene) {_status=@"Flame v2: no active scene";[self setNeedsDisplay];return;}
    P sceneValue=object_unbox(scene);
    if(!sceneValue) return;
    P loadedBox=invoke(loaded,sceneValue,nullptr,ok);
    P handleBox=invoke(handle,sceneValue,nullptr,ok);
    if(!ok || !loadedBox || !handleBox) {_status=@"Flame v2: scene check failed";[self setNeedsDisplay];return;}
    bool isLoaded=*static_cast<bool*>(object_unbox(loadedBox));
    int currentHandle=*static_cast<int*>(object_unbox(handleBox));
    NSTimeInterval now=NSProcessInfo.processInfo.systemUptime;
    if(!isLoaded || currentHandle!=_sceneHandle) {
        _sceneHandle=currentHandle; _settledAfter=now+5;
    }
    if(!isLoaded || now<_settledAfter) {
        _status=@"Flame v2: waiting for scene";[self setNeedsDisplay];return;
    }
    P player=cls("","PlayerController"), cameraClass=cls("UnityEngine","Camera");
    P component=cls("UnityEngine","Component"), transform=cls("UnityEngine","Transform");
    P vector=cls("UnityEngine","Vector3"); uint32_t align=0;
    if(!vector || class_value_size(vector,&align)!=sizeof(V3)) {
        _status=@"Flame: Vector3 layout unavailable"; [self setNeedsDisplay];return;
    }
    P find=method(cls("UnityEngine","Object"),"FindObjectsOfType",1,"System.Type");
    P main=method(cameraClass,"get_main",0), tr=method(component,"get_transform",0);
    P pos=method(transform,"get_position",0), project=method(cameraClass,"WorldToScreenPoint",1,"UnityEngine.Vector3");
    P get=method(cls("System","Array"),"GetValue",1,"System.Int32");
    if(!player||!find||!main||!tr||!pos||!project||!get) {
        _status=@"Flame: required class/method unavailable";[self setNeedsDisplay];return;
    }
    P cam=invoke(main,nullptr,nullptr,ok);
    if(!cam){_status=@"Flame: Camera.main unavailable";[self setNeedsDisplay];return;}
    P type=type_get_object(class_get_type(player)); P findArgs[]={type};
    P array=invoke(find,nullptr,findArgs,ok);
    if(!ok||!array){_status=@"Flame: enumeration failed";[self setNeedsDisplay];return;}
    // Unity screen coordinates are pixels with a bottom-left origin.
    P width=method(cls("UnityEngine","Screen"),"get_width",0);
    P height=method(cls("UnityEngine","Screen"),"get_height",0);
    P wb=invoke(width,nullptr,nullptr,ok), hb=invoke(height,nullptr,nullptr,ok);
    if(!ok||!wb||!hb) return;
    int w=*static_cast<int*>(object_unbox(wb)),h=*static_cast<int*>(object_unbox(hb));
    if(w<=0||h<=0) return;
    uintptr_t count=array_length(array);
    if(count>128){_status=@"Flame: unexpected object count";[self setNeedsDisplay];return;}
    P alive=method(cls("UnityEngine","Object"),"op_Implicit",1,"UnityEngine.Object");
    if(!alive){_status=@"Flame v2: object validity method unavailable";[self setNeedsDisplay];return;}
    for(int i=0;i<(int)count && ok;i++) {
        P args[]={&i}; P object=invoke(get,array,args,ok); if(!object) continue;
        P aliveArgs[]={object}; P aliveBox=invoke(alive,nullptr,aliveArgs,ok);
        if(!aliveBox || !*static_cast<bool*>(object_unbox(aliveBox))) continue;
        P t=invoke(tr,object,nullptr,ok); if(!t) continue;
        P box=invoke(pos,t,nullptr,ok); if(!box) continue;
        V3 world; memcpy(&world,object_unbox(box),sizeof(world)); P projectArgs[]={&world};
        P screen=invoke(project,cam,projectArgs,ok); if(!screen) continue;
        V3 v; memcpy(&v,object_unbox(screen),sizeof(v));
        if(std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) && v.z>0 && v.x>=0 && v.x<=w && v.y>=0 && v.y<=h)
            _points.push_back(CGPointMake(v.x/w*self.bounds.size.width,(1-v.y/h)*self.bounds.size.height));
    }
    if(!ok) { _enabled=NO; _markerToggle.on=NO; _points.clear(); }
    _status=ok?[NSString stringWithFormat:@"Flame v2: %zu markers",_points.size()]:@"Flame v2: exception; paused";
    [self setNeedsDisplay];
}
- (void)drawRect:(CGRect)rect {
    [UIColor.cyanColor setStroke];
    for(CGPoint p:_points) { UIBezierPath* path=[UIBezierPath bezierPathWithOvalInRect:CGRectMake(p.x-5,p.y-5,10,10)];path.lineWidth=2;[path stroke]; }
    [_status drawAtPoint:CGPointMake(55,25) withAttributes:@{NSForegroundColorAttributeName:UIColor.cyanColor,NSFontAttributeName:[UIFont systemFontOfSize:13]}];
}
@end

__attribute__((constructor)) static void startFlame() {
    dispatch_async(dispatch_get_main_queue(), ^{
        // The timer waits for a visible foreground window; no new overlay window.
        __block NSTimer* waiter=nil;
        waiter=[NSTimer scheduledTimerWithTimeInterval:1 repeats:YES block:^(NSTimer* timer) {
            for(UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
                if(scene.activationState!=UISceneActivationStateForegroundActive || ![scene isKindOfClass:UIWindowScene.class]) continue;
                for(UIWindow* window in ((UIWindowScene*)scene).windows) if(window.isKeyWindow) {
                    [window addSubview:[[FlameMarkers alloc] initWithFrame:window.bounds]];
                    [timer invalidate]; waiter=nil; return;
                }
            }
        }];
    });
}

