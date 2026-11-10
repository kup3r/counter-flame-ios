#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <dlfcn.h>
#include <cstring>
#include <vector>
#include <deque>
#include <cmath>
#include <utility>
#include <unordered_map>

// In-process IL2CPP module. Runtime names and typed fields; no fixed offsets.
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
static NSString* lastException=nil;
void (*format_exception)(P,char*,int)=nullptr;
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
    format_exception=reinterpret_cast<decltype(format_exception)>(dlsym(h,"il2cpp_format_exception"));
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
    if(exception){
        ok=false;
        char details[192]{};
        if(format_exception)format_exception(exception,details,sizeof(details)-1);
        NSString* reason=details[0]?[NSString stringWithUTF8String:details]:nil;
        lastException=[NSString stringWithFormat:@"%s: %@",method_get_name(m),reason?:@"исключение Unity"];
        NSLog(@"[Flame v12] %@",lastException);
    }
    if(result && !exception && roots) roots->hold(result);
    return exception?nullptr:result;
}
}

struct V3 {float x,y,z;};
static float FlameDistanceSquared(V3 a,V3 b){
    float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
    return x*x+y*y+z*z;
}
static bool finiteV3(V3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
#include "FlameExtras.hpp"
#include "FlameScene.hpp"
#include "FlameGloves.hpp"
#include "FlameRoundPractice.hpp"
#include "FlameEffects.hpp"
#include "FlameMovement.hpp"
#include "FlameNades.hpp"
#include "FlameLag.hpp"
#include "FlameInput.hpp"
#include "FlameCamera.hpp"
struct Bone {uint32_t handle;int parent=-1;};
struct Target {
    uint32_t player=0,network=0,head=0;
    int actor=-1,chest=-1,pelvis=-1;
    std::vector<Bone> bones;
};
struct TrailPoint {V3 position;double time;};
struct Segment {CGPoint a,b;bool enemy;};
struct Item {CGRect box;NSString* text;bool enemy;};
struct HeadMarker {CGPoint point;CGFloat radius;bool enemy;};
struct Api {
    Runtime::P player=nullptr,spectatorClass=nullptr;
    Runtime::P active=nullptr,loaded=nullptr,handle=nullptr,find=nullptr,main=nullptr;
    Runtime::P tr=nullptr,pos=nullptr,project=nullptr,get=nullptr,alive=nullptr,width=nullptr,height=nullptr;
    Runtime::P animator=nullptr,skeleton=nullptr,body=nullptr,bodyLod=nullptr,bones=nullptr,parent=nullptr,name=nullptr;
    Runtime::P network=nullptr,local=nullptr,team=nullptr,actor=nullptr,nick=nullptr,health=nullptr,dead=nullptr;
    Runtime::P spectating=nullptr,watched=nullptr;
    Runtime::P head=nullptr;
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
        head=method(sk,"GetHeadBone",0);
        bones=method(cls("UnityEngine","SkinnedMeshRenderer"),"get_bones",0);
        network=method(player,"get_Player",0);health=method(player,"get_Health",0);dead=method(player,"get_IsDeath",0);
        P photon=cls("Photon.Realtime","Player");team=method(photon,"get_Team",0);actor=method(photon,"get_ActorNumber",0);nick=method(photon,"get_NickName",0);
        local=method(cls("Photon.Pun","PhotonNetwork"),"get_LocalPlayer",0);
        spectating=method(spectatorClass,"get_IsSpectating",0);watched=method(spectatorClass,"get_CurrentPlayer",0);
        uint32_t align=0;P v=cls("UnityEngine","Vector3");
        return player&&v&&class_value_size(v,&align)==sizeof(V3)&&active&&loaded&&handle&&find&&main&&tr&&pos&&project&&get&&alive&&width&&height&&animator&&skeleton&&body&&bones&&parent&&network&&local&&team&&actor&&health&&dead;
    }
};
#include "FlameMenu.hpp"
@interface FlameMarkers:UIView<UIGestureRecognizerDelegate,FlameMenuDelegate>
@end
@implementation FlameMarkers {
    CADisplayLink* _displayLink;UIView* _panel;UITapGestureRecognizer* _gesture;UISwitch* _master;
    Api _api;BOOL _ready,_apiReady,_enabled,_onlyEnemies,_showBoxes,_showSkeleton,_showHealth,_showNames,_showDistance,_showTracers,_debug;
    std::vector<Target> _targets;std::vector<Segment> _segments,_tracers;std::vector<Item> _items;std::vector<CGPoint> _dots;
    NSString* _status;int _sceneHandle;NSTimeInterval _settledAfter,_nextDiscovery;uint32_t _spectator;
    size_t _rigs,_boneCount,_missingRig;
    Extra::State _extras;BOOL _aim,_noRecoil;uint32_t _controls;float _aimRadius;
    UILabel* _extraLabel;
    uint32_t _activeLocal;BOOL _aimFault;NSTimeInterval _nextPlayerProbe,_retryAfter;
    FlameMenu* _menu;NSDictionary* _skinRequest;
    BOOL _noSpread,_speedHack,_unlimitedAmmo,_fovOverride,_crosshair,_corners,_filled,_headCircle,_showFps;
    float _speedFactor,_fovValue,_maxDistance,_lineWidth,_aimSmoothing;
    float _aimDelay,_aimMaxTurn,_aimDeadzone,_aimDistance;
    BOOL _aimRing,_glow,_vignette,_centerDot,_trail;
    NSInteger _aimPoint;
    FlameScene::State _sceneVisuals;
    FlameGloves::State _gloves;
    FlameRoundPractice::State _practice;
    FlameEffects::State _effects;
    FlameMovement::State _movementHelpers;
    FlameNades::State _nades;
    FlameLag::State _lag;
    std::vector<std::pair<CGPoint,NSString*>> _nadeMarks;
    int _spinMode;
    float _spinPitch;
    FlameInput::State _input;
    FlameCamera::State _thirdView;
    BOOL _thirdPerson,_spin;
    float _cameraDistance,_spinSpeed;
    BOOL _nextAgent;
    BOOL _trigger,_bunnyHop,_wallShot;float _triggerDelay;double _bunnyProbe;
    BOOL _skyTint,_sceneLight,_playerTint;
    float _skyHue,_sceneHour,_sceneHue,_playerHue;
    float _trailTime;
    std::deque<TrailPoint> _trailPoints;
    std::vector<std::pair<Segment,float>> _trailSegments;
    double _fireStarted;
    CGGradientRef _vignetteGradient;NSInteger _accent;
    std::vector<HeadMarker> _headMarkers;double _observeAfter,_fpsStart;NSInteger _frames;float _observedFPS;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if((self=[super initWithFrame:frame])){
        self.backgroundColor=UIColor.clearColor;self.userInteractionEnabled=YES;
        self.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        _sceneHandle=-1;_status=@"Flame v12: ESP off";_aimRadius=100;_aimSmoothing=10;_aimMaxTurn=180;_aimRing=YES;_trailTime=2;_cameraDistance=3;_spinSpeed=180;_triggerDelay=80;_skyHue=210;_sceneHour=14;_sceneHue=360;_playerHue=180;
        _speedFactor=1.5;_fovValue=90;_lineWidth=1.2;_corners=YES;_showFps=YES;
        _onlyEnemies=YES;_showBoxes=YES;_showSkeleton=YES;_showHealth=YES;_showDistance=YES;
        CGFloat width=fmin(740,frame.size.width-42),height=fmin(355,frame.size.height-30);
        _menu=[[FlameMenu alloc]initWithFrame:CGRectMake((frame.size.width-width)/2,(frame.size.height-height)/2,width,height) config:FlameDefaultConfig()];
        _menu.delegate=self;_menu.hidden=YES;_panel=_menu;_master=_menu.master;_extraLabel=_menu.statusLabel;[self addSubview:_menu];
        _gesture=[[UITapGestureRecognizer alloc]initWithTarget:self action:@selector(toggleMenu)];_gesture.numberOfTapsRequired=2;_gesture.numberOfTouchesRequired=3;_gesture.cancelsTouchesInView=NO;_gesture.delegate=self;
        _displayLink=[CADisplayLink displayLinkWithTarget:self selector:@selector(sample)];_displayLink.preferredFramesPerSecond=60;
        [_displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];_displayLink.paused=YES;
    }return self;
}
- (NSDictionary*)preferences {
    NSDictionary* options=@{FlameOptionKey(Master):@NO,FlameOptionKey(Enemies):@(_onlyEnemies),FlameOptionKey(Boxes):@(_showBoxes),FlameOptionKey(Skeleton):@(_showSkeleton),FlameOptionKey(Health):@(_showHealth),FlameOptionKey(Names):@(_showNames),FlameOptionKey(Distance):@(_showDistance),FlameOptionKey(Tracers):@(_showTracers),FlameOptionKey(Debug):@(_debug),FlameOptionKey(Aim):@(_aim),FlameOptionKey(Recoil):@(_noRecoil),FlameOptionKey(NoSpread):@(_noSpread),FlameOptionKey(Speed):@(_speedHack),FlameOptionKey(Ammo):@(_unlimitedAmmo),FlameOptionKey(Fov):@(_fovOverride),FlameOptionKey(Crosshair):@(_crosshair),FlameOptionKey(CornerBox):@(_corners),FlameOptionKey(FilledBox):@(_filled),FlameOptionKey(HeadCircle):@(_headCircle),FlameOptionKey(FPSCounter):@(_showFps),
        FlameOptionKey(AimRing):@(_aimRing),FlameOptionKey(Glow):@(_glow),FlameOptionKey(Vignette):@(_vignette),FlameOptionKey(CenterDot):@(_centerDot),FlameOptionKey(Trail):@(_trail),FlameOptionKey(SkyTint):@(_skyTint),FlameOptionKey(SceneLight):@(_sceneLight),FlameOptionKey(PlayerTint):@(_playerTint),FlameOptionKey(Trigger):@(_trigger),FlameOptionKey(BunnyHop):@(_bunnyHop),FlameOptionKey(ThirdPerson):@(_thirdPerson),FlameOptionKey(Spin):@(_spin),FlameOptionKey(PracticeWin):@(_practice.enabled),FlameOptionKey(AntiFlash):@(_effects.antiFlash),FlameOptionKey(AntiSmoke):@(_effects.antiSmoke),FlameOptionKey(AutoStrafe):@(_movementHelpers.autoStrafe),FlameOptionKey(EdgeJump):@(_movementHelpers.edgeJump),FlameOptionKey(UnlimitedDuck):@(_movementHelpers.unlimitedDuck),FlameOptionKey(NadeHelper):@(_nades.enabled),FlameOptionKey(FakeLag):@(_lag.enabled),FlameOptionKey(WallShot):@(_wallShot)};
    return @{@"options":options,@"cameraDistance":@(_cameraDistance),@"spinSpeed":@(_spinSpeed),@"spinMode":@(_spinMode),@"spinPitch":@(_spinPitch),@"lagMs":@(_lag.milliseconds),@"triggerDelay":@(_triggerDelay),@"aimPoint":@(_aimPoint),@"trailTime":@(_trailTime),@"skyHue":@(_skyHue),@"sceneHour":@(_sceneHour),@"sceneHue":@(_sceneHue),@"playerHue":@(_playerHue),@"aimRadius":@(_aimRadius),@"aimSmoothing":@(_aimSmoothing),@"aimDelay":@(_aimDelay),@"aimMaxTurn":@(_aimMaxTurn),@"aimDeadzone":@(_aimDeadzone),@"aimDistance":@(_aimDistance),@"speedFactor":@(_speedFactor),@"fov":@(_fovValue),@"maxDistance":@(_maxDistance),@"lineWidth":@(_lineWidth),@"accent":@(_accent),@"fps":@(_displayLink.preferredFramesPerSecond)};
}
- (void)flameSetOption:(NSInteger)tag value:(BOOL)value {
    UISwitch* change=[[UISwitch alloc]init];change.tag=tag;change.on=value;[self optionChanged:change];
}
- (void)flameSetNumber:(NSString*)key value:(double)value {
    if(!std::isfinite(value))return;
    if([key isEqual:@"aimRadius"])_aimRadius=std::clamp(value,5.0,180.0);
    else if([key isEqual:@"aimSmoothing"])_aimSmoothing=std::clamp(value,1.0,25.0);
    else if([key isEqual:@"cameraDistance"])_cameraDistance=std::clamp(value,1.0,5.0);
    else if([key isEqual:@"spinSpeed"])_spinSpeed=std::clamp(value,45.0,720.0);
    else if([key isEqual:@"spinMode"])_spinMode=(int)std::clamp(round(value),0.,2.);
    else if([key isEqual:@"spinPitch"])_spinPitch=std::clamp(value,-45.,45.);
    else if([key isEqual:@"lagMs"])_lag.milliseconds=(int)std::clamp(round(value),30.,250.);
    else if([key isEqual:@"triggerDelay"])_triggerDelay=std::clamp(value,0.0,350.0);
    else if([key isEqual:@"aimPoint"])_aimPoint=(NSInteger)std::clamp(value,0.0,3.0);
    else if([key isEqual:@"skyHue"])_skyHue=std::clamp(value,0.0,359.0);
    else if([key isEqual:@"sceneHue"])_sceneHue=std::clamp(value,0.0,360.0);
    else if([key isEqual:@"sceneHour"])_sceneHour=std::clamp(value,0.0,24.0);
    else if([key isEqual:@"playerHue"])_playerHue=std::clamp(value,0.0,360.0);
    else if([key isEqual:@"trailTime"])_trailTime=std::clamp(value,.5,5.0);
    else if([key isEqual:@"aimDelay"])_aimDelay=std::clamp(value,0.0,500.0);
    else if([key isEqual:@"aimMaxTurn"])_aimMaxTurn=std::clamp(value,15.0,360.0);
    else if([key isEqual:@"aimDeadzone"])_aimDeadzone=std::clamp(value,0.0,30.0);
    else if([key isEqual:@"aimDistance"])_aimDistance=std::clamp(value,0.0,250.0);
    else if([key isEqual:@"speedFactor"])_speedFactor=std::clamp(value,1.0,3.0);
    else if([key isEqual:@"fov"])_fovValue=std::clamp(value,60.0,120.0);
    else if([key isEqual:@"maxDistance"])_maxDistance=std::clamp(value,0.0,250.0);
    else if([key isEqual:@"lineWidth"])_lineWidth=std::clamp(value,.6,3.0);
    else if([key isEqual:@"accent"])_accent=(NSInteger)std::clamp(value,0.0,3.0);
    else if([key isEqual:@"fps"])_displayLink.preferredFramesPerSecond=value<45?30:60;
    [self setNeedsDisplay];
}
- (void)flameSkinAction:(NSString*)action weapon:(NSDictionary*)weapon skin:(NSDictionary*)skin {
    if(!_enabled){_extraLabel.text=@"Включи модуль после загрузки матча";return;}
    if([action isEqual:@"gloves"]||[action isEqual:@"restoreGloves"]){
        _gloves.clear(_extras,true);
        _gloves.desired=[action isEqual:@"gloves"]?skin:nil;
        _gloves.status=_gloves.desired?@"Перчатки: ожидаю оружие в руках":@"Перчатки: исходный вид запрошен";
        _extraLabel.text=_gloves.status;
        return;
    }
    if(!weapon&&![action isEqual:@"restoreSkin"]&&![action isEqual:@"restoreModel"]&&![action isEqual:@"sticker"])return;
    _skinRequest=@{@"action":action,@"weapon":weapon?:@{},@"skin":skin?:@{}};_extras.status=@"Проверка текущего оружия и выбранной конфигурации…";_extraLabel.text=_extras.status;
}
- (void)applyPreferences:(NSDictionary*)input {
    [self flameSetOption:Master value:NO];
    NSDictionary* defaults=FlameDefaultConfig();NSMutableDictionary* config=[defaults mutableCopy];NSMutableDictionary* opts=[defaults[@"options"] mutableCopy];
    NSDictionary* supplied=[input[@"options"] isKindOfClass:NSDictionary.class]?input[@"options"]:@{};
    for(NSString* key in defaults[@"options"])if([supplied[key] isKindOfClass:NSNumber.class])opts[key]=@([supplied[key] boolValue]);
    opts[FlameOptionKey(Master)]=@NO;config[@"options"]=opts;
    for(NSString* key in defaults)if(![key isEqual:@"options"]&&[input[key] isKindOfClass:NSNumber.class]&&std::isfinite([input[key] doubleValue]))config[key]=input[key];
    for(NSString* key in defaults)if(![key isEqual:@"options"])[self flameSetNumber:key value:[config[key] doubleValue]];
    for(NSString* key in opts)if(key.integerValue!=Master)[self flameSetOption:key.integerValue value:[opts[key] boolValue]];
    [_menu sync:[self preferences]];_extraLabel.text=@"Настройки загружены. Модуль оставлен выключенным.";
}
- (void)flameConfigAction:(NSString*)action {
    if([action isEqual:@"save"]){[NSUserDefaults.standardUserDefaults setObject:[self preferences] forKey:@"flame.v7.preferences"];_extraLabel.text=@"Настройки сохранены в приложении";}
    else if([action isEqual:@"load"]){id saved=[NSUserDefaults.standardUserDefaults objectForKey:@"flame.v7.preferences"];if([saved isKindOfClass:NSDictionary.class])[self applyPreferences:saved];else _extraLabel.text=@"Сохранённых настроек ещё нет";}
    else if([action isEqual:@"reset"]){_extras.desiredKnife=nil;_gloves.desired=nil;[_extras.desiredSkins removeAllObjects];[self applyPreferences:FlameDefaultConfig()];_extraLabel.text=@"Стандартные настройки восстановлены";}
    else if([action isEqual:@"practicePlant"]||[action isEqual:@"practiceDefuse"]){
        if(!_enabled){_extraLabel.text=@"Включи модуль после загрузки матча";return;}
        _practice.queue([action isEqual:@"practicePlant"]?FlameRoundPractice::Action::Plant:FlameRoundPractice::Action::Defuse);
    }
    else if([action hasPrefix:@"nade"]){
        if(!_enabled||!_nades.enabled){_extraLabel.text=@"Включи модуль и Grenade Helper после загрузки матча";return;}
        if(!_activeLocal){_extraLabel.text=@"Для раскидок нужен живой персонаж";return;}
        if([action isEqual:@"nadeSave"])_nades.pending=FlameNades::State::Save;
        else if([action isEqual:@"nadeNext"])_nades.pending=FlameNades::State::Next;
        else if([action isEqual:@"nadeStand"])_nades.pending=FlameNades::State::Stand;
        else if([action isEqual:@"nadeAlign"])_nades.pending=FlameNades::State::Align;
        else if([action isEqual:@"nadeRemove"])_nades.pending=FlameNades::State::Remove;
    }
    else if([action isEqual:@"nextAgent"]){
        if(_enabled){_nextAgent=YES;_extraLabel.text=@"Поиск загруженного агента своей команды…";}
        else _extraLabel.text=@"Включи модуль после загрузки матча";
    }
    else if([action isEqual:@"stop"]){[self flameSetOption:Master value:NO];[_menu setRuntimeOption:Master value:NO];_extraLabel.text=@"Модуль выключен; сохранённые параметры восстановлены";}
}

- (void)clearTargets {
    for(const Target& t:_targets){if(t.player)Runtime::gchandle_free(t.player);if(t.network)Runtime::gchandle_free(t.network);if(t.head)Runtime::gchandle_free(t.head);for(Bone b:t.bones)Runtime::gchandle_free(b.handle);}_targets.clear();
    if(_spectator){Runtime::gchandle_free(_spectator);_spectator=0;}_nextDiscovery=0;
    if(_controls){Runtime::gchandle_free(_controls);_controls=0;}
}
- (void)clearDrawing {_items.clear();_segments.clear();_tracers.clear();_dots.clear();_headMarkers.clear();_trailSegments.clear();_nadeMarks.clear();}
- (void)didMoveToWindow {
    [super didMoveToWindow];[_gesture.view removeGestureRecognizer:_gesture];if(self.window)[self.window addGestureRecognizer:_gesture];else [self pause:@"Flame v12: paused"];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)a shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)b{return YES;}
- (UIView*)hitTest:(CGPoint)p withEvent:(UIEvent*)e {UIView* hit=[super hitTest:p withEvent:e];return !_panel.hidden&&[hit isDescendantOfView:_panel]?hit:nil;}
- (void)toggleMenu {_panel.hidden=!_panel.hidden;}
- (void)resetLocalState {
    _movementHelpers.clear(_extras);_nades.pending=FlameNades::State::None;_nadeMarks.clear();
    _extras.restorePlayer();_extras.resetCosmetics();_gloves.clear(_extras,false);_sceneVisuals.restorePlayer(_extras);_sceneVisuals.restoreSpin(_extras);_input.clear();_thirdView.clear(_extras);
    if(_activeLocal){Runtime::gchandle_free(_activeLocal);_activeLocal=0;}
    _fireStarted=0;_observeAfter=0;_aimFault=NO;_trailPoints.clear();
    [_menu updateWeapon:-1 name:@"" skin:@""];
}
- (void)optionChanged:(UISwitch*)s {
    switch(s.tag){
        case Master:_effects.clear(_extras,_sceneVisuals);_movementHelpers.clear(_extras);_nades.clear();_lag.clear(_sceneVisuals);_nadeMarks.clear();_extras.status=s.on?@"Включение после проверки сцены":@"Модуль выключен; параметры восстановлены";_extraLabel.text=_extras.status;_extras.restoreFov();_extras.restorePlayer();_gloves.clear(_extras,true);_sceneVisuals.clear(_extras,true);_input.clear();_thirdView.clear(_extras);_practice.clear();_nextAgent=NO;_trailPoints.clear();_aimFault=NO;_fireStarted=0;_skinRequest=nil;[self clearTargets];[self clearDrawing];_enabled=s.on;_sceneHandle=-1;_retryAfter=0;_settledAfter=NSProcessInfo.processInfo.systemUptime+5;_fpsStart=0;_frames=0;_displayLink.paused=!_enabled;_status=_enabled?@"Flame v12: waiting for scene":@"Flame v12: ESP off";break;
        case Enemies:_onlyEnemies=s.on;break;case Boxes:_showBoxes=s.on;break;case Skeleton:_showSkeleton=s.on;break;
        case Health:_showHealth=s.on;break;case Names:_showNames=s.on;break;case Distance:_showDistance=s.on;break;case Tracers:_showTracers=s.on;break;case Debug:_debug=s.on;break;
        case Aim:_aim=s.on;_aimFault=NO;_fireStarted=0;_nextDiscovery=0;break;
        case Recoil:_noRecoil=s.on;if(!_noRecoil)_extras.restoreRecoil();break;
        case NoSpread:_noSpread=s.on;if(!_noSpread)_extras.restoreSpread();break;
        case Speed:_speedHack=s.on;if(!_speedHack)_extras.restoreSpeed();break;
        case Ammo:_unlimitedAmmo=s.on;break;case Fov:_fovOverride=s.on;if(!_fovOverride)_extras.restoreFov();break;
        case Crosshair:_crosshair=s.on;break;case CornerBox:_corners=s.on;break;case FilledBox:_filled=s.on;break;case HeadCircle:_headCircle=s.on;break;case FPSCounter:_showFps=s.on;break;
        case AimRing:_aimRing=s.on;break;case Glow:_glow=s.on;break;
        case Vignette:
            _vignette=s.on;
            if(_vignette&&!_vignetteGradient){
                CGColorSpaceRef space=CGColorSpaceCreateDeviceRGB();
                CGFloat stops[]={0,0,0,0,0,0,0,.3};
                _vignetteGradient=CGGradientCreateWithColorComponents(space,stops,nullptr,2);
                CGColorSpaceRelease(space);
            }
            break;
        case CenterDot:_centerDot=s.on;break;
        case Trail:_trail=s.on;if(!_trail)_trailPoints.clear();break;
        case ThirdPerson:_thirdPerson=s.on;_thirdView.fault=false;if(!_thirdPerson)_thirdView.clear(_extras);break;
        case AntiFlash:_effects.antiFlash=s.on;if(!s.on)_effects.restoreFlash(_extras,_sceneVisuals);break;
        case AntiSmoke:_effects.antiSmoke=s.on;if(!s.on)_effects.restoreSmoke(_extras,_sceneVisuals);break;
        case AutoStrafe:_movementHelpers.autoStrafe=s.on;break;
        case EdgeJump:_movementHelpers.edgeJump=s.on;break;
        case UnlimitedDuck:_movementHelpers.unlimitedDuck=s.on;if(!s.on)_movementHelpers.clear(_extras);break;
        case NadeHelper:_nades.enabled=s.on;if(!s.on){_nades.clear();_nadeMarks.clear();}break;
        case FakeLag:_lag.enabled=s.on;if(!s.on)_lag.clear(_sceneVisuals);break;
        case PracticeWin:_practice.enabled=s.on;if(!_practice.enabled)_practice.clear();break;
        case Spin:_spin=s.on;if(!_spin)_sceneVisuals.restoreSpin(_extras);break;
        case WallShot:_wallShot=s.on;_input.resetTrigger();break;
        case Trigger:_trigger=s.on;_input.resetTrigger();break;
        case BunnyHop:_bunnyHop=s.on;if(!_bunnyHop)_input.clear();break;
        case SkyTint:_skyTint=s.on;break;case SceneLight:_sceneLight=s.on;break;case PlayerTint:_playerTint=s.on;break;
    }[self setNeedsDisplay];
}
- (void)pause:(NSString*)status {_effects.clear(_extras,_sceneVisuals);_lag.clear(_sceneVisuals);_nades.clear();_practice.clear();_sceneVisuals.clear(_extras,false);_extras.restoreFov();[self resetLocalState];_skinRequest=nil;_enabled=NO;_master.on=NO;[_menu setRuntimeOption:Master value:NO];_displayLink.paused=YES;[self clearTargets];[self clearDrawing];_status=status;[self setNeedsDisplay];}
- (void)waitForPlayers:(NSString*)status now:(NSTimeInterval)now {
    if(_activeLocal)[self resetLocalState];
    _lag.clear(_sceneVisuals);
    [self clearTargets];[self clearDrawing];
    _retryAfter=now+.5;_status=status;
    [self setNeedsDisplay];
}
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
    if(_extras.ready&&_extras.controlsClass){bool ok=true;P type=type_get_object(class_get_type(_extras.controlsClass));P args[]={type};P list=invoke(_api.find,nullptr,args,ok);if(list&&array_length(list)>0&&array_length(list)<=8){int zero=0;P idx[]={&zero};P c=invoke(_api.get,list,idx,ok);if([self isAlive:c ok:ok])_controls=gchandle_new(c,true);}}
    if(_api.spectatorClass&&_api.spectating&&_api.watched){bool ok=true;
        P type=type_get_object(class_get_type(_api.spectatorClass));P args[]={type};P list=invoke(_api.find,nullptr,args,ok);
        if(list&&array_length(list)>0&&array_length(list)<=8){int zero=0;P idx[]={&zero};P object=invoke(_api.get,list,idx,ok);if([self isAlive:object ok:ok])_spectator=gchandle_new(object,true);}
    }
    P type=type_get_object(class_get_type(_api.player));P args[]={type};P array=invoke(_api.find,nullptr,args,ok);if(!ok||!array)return;
    uintptr_t count=array_length(array);if(count>128){ok=false;return;}
    for(int i=0;i<(int)count&&ok;i++){
        RootScope targetRoots;bool ok=true;int idx=i;P arg[]={&idx};P player=invoke(_api.get,array,arg,ok);if(![self isAlive:player ok:ok])continue;
        Target target;target.player=gchandle_new(player,true);
        P network=invoke(_api.network,player,nullptr,ok);if(network){target.network=gchandle_new(network,true);target.actor=[self integer:invoke(_api.actor,network,nullptr,ok)];}
        P animator=invoke(_api.animator,player,nullptr,ok),sk=nullptr,mesh=nullptr;
        if([self isAlive:animator ok:ok])sk=invoke(_api.skeleton,animator,nullptr,ok);
        if([self isAlive:sk ok:ok]){if(_api.head){P head=invoke(_api.head,sk,nullptr,ok);if([self isAlive:head ok:ok])target.head=gchandle_new(head,true);}mesh=invoke(_api.body,sk,nullptr,ok);if(![self isAlive:mesh ok:ok]&&ok&&_api.bodyLod)mesh=invoke(_api.bodyLod,sk,nullptr,ok);}
        if([self isAlive:mesh ok:ok]){
            P list=invoke(_api.bones,mesh,nullptr,ok);
            if(list&&array_length(list)<=256){
                std::vector<P> objects;std::unordered_map<P,int> indices;
                int chestRank=0;
                for(int j=0;j<(int)array_length(list)&&ok;j++){
                    P index[]={&j};P bone=invoke(_api.get,list,index,ok);if(![self isAlive:bone ok:ok]||indices.count(bone))continue;
                    // Optional naming filter removes fine finger/toe/helper chains; no bone indices are guessed.
                    NSString* name=_api.name?[[self string:invoke(_api.name,bone,nullptr,ok)] lowercaseString]:@"";
                    BOOL fine=NO;for(NSString* token in @[@"finger",@"thumb",@"index",@"middle",@"pinky",@"little",@"toe",@"twist",@"helper",@"weapon",@"ik_"])if([name containsString:token]){fine=YES;break;}
                    if(fine)continue;
                    NSString* compact=[[name componentsSeparatedByCharactersInSet:NSCharacterSet.alphanumericCharacterSet.invertedSet] componentsJoinedByString:@""];
                    int rank=[compact hasSuffix:@"upperchest"]?4:[compact hasSuffix:@"chest"]?3:[compact hasSuffix:@"spine2"]?2:[compact hasSuffix:@"spine1"]?1:0;
                    if(rank>chestRank){target.chest=(int)objects.size();chestRank=rank;}
                    if([compact hasSuffix:@"pelvis"]||[compact hasSuffix:@"hips"])target.pelvis=(int)objects.size();
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
    if(UIApplication.sharedApplication.applicationState!=UIApplicationStateActive||!self.window){[self pause:@"Flame v12: paused in background"];return;}
    if(!_ready)_ready=load();if(!_ready){_status=@"Flame v12: waiting for runtime";[self setNeedsDisplay];return;}
    if(!NSThread.isMainThread||!thread_current()){[self pause:@"Flame v12: runtime thread unavailable"];return;}
    if(!_apiReady)_apiReady=_api.init();if(!_apiReady){[self pause:@"Flame v12: required methods unavailable"];return;}
    RootScope frameRoots;bool ok=true;
    P scene=invoke(_api.active,nullptr,nullptr,ok),raw=scene?object_unbox(scene):nullptr;
    if(!raw){[self pause:@"Flame v12: no scene"];return;}
    P loaded=invoke(_api.loaded,raw,nullptr,ok),handle=invoke(_api.handle,raw,nullptr,ok);
    if(!ok||!loaded||!handle){[self pause:@"Flame v12: scene check failed"];return;}
    int current=[self integer:handle];NSTimeInterval now=NSProcessInfo.processInfo.systemUptime;
    if(!_fpsStart){_fpsStart=now;_frames=0;}_frames++;if(now-_fpsStart>=.5){_observedFPS=_frames/(now-_fpsStart);_frames=0;_fpsStart=now;}
    if(!*static_cast<bool*>(object_unbox(loaded))||(_sceneHandle!=-1&&current!=_sceneHandle)){[self pause:@"Flame v12: scene changed; enable after loading"];return;}
    if(_sceneHandle==-1){_sceneHandle=current;_settledAfter=now+5;}
    if(now<_settledAfter){_status=@"Flame v12: waiting for scene";[self setNeedsDisplay];return;}
    if(now<_retryAfter)return;
    if(!_extras.ready){if(!_extras.init())_extras.status=@"Экспериментальные API недоступны";}
    if(now>=_nextDiscovery){[self discover:ok];_nextDiscovery=now+0.5;if(!ok){[self waitForPlayers:@"Ожидание объектов матча" now:now];return;}}
    P camera=invoke(_api.main,nullptr,nullptr,ok);
    if(![self isAlive:camera ok:ok]){[self waitForPlayers:@"Ожидание камеры матча" now:now];return;}
    int w=[self integer:invoke(_api.width,nullptr,nullptr,ok)],h=[self integer:invoke(_api.height,nullptr,nullptr,ok)];
    if(!ok||w<=0||h<=0){[self waitForPlayers:@"Ожидание экрана матча" now:now];return;}
    CGPoint scale=CGPointMake(self.bounds.size.width/w,self.bounds.size.height/h);
    P local=invoke(_api.local,nullptr,nullptr,ok),anchor=local;
    int localActor=local?[self integer:invoke(_api.actor,local,nullptr,ok)]:-1;
    BOOL watching=NO;
    if(_spectator){
        bool spectatorOK=true;
        P manager=gchandle_get_target(_spectator);
        if([self isAlive:manager ok:spectatorOK]){
            P state=invoke(_api.spectating,manager,nullptr,spectatorOK);
            if(state&&*static_cast<bool*>(object_unbox(state))){
                watching=YES;
                if(P watched=invoke(_api.watched,manager,nullptr,spectatorOK))anchor=watched;
            }
        }
    }
    int anchorActor=anchor?[self integer:invoke(_api.actor,anchor,nullptr,ok)]:-1;
    NSString* anchorTeam=anchor?[[self string:invoke(_api.team,anchor,nullptr,ok)] lowercaseString]:@"";
    V3 cameraPos{};BOOL haveCameraPos=NO;P cameraTransform=nullptr;
    if(_showDistance||_aim||_trigger||_wallShot||_thirdPerson||_maxDistance>0||_movementHelpers.autoStrafe||_nades.enabled){cameraTransform=invoke(_api.tr,camera,nullptr,ok);if(cameraTransform)haveCameraPos=[self position:cameraTransform result:cameraPos ok:ok];}
    if(!ok){[self waitForPlayers:@"Ожидание данных матча" now:now];return;}
    P localController=nullptr;
    if(!watching)for(const Target& t:_targets)if(t.actor>=0&&t.actor==localActor){
        bool playerOK=true;
        P candidate=gchandle_get_target(t.player);
        if(![self isAlive:candidate ok:playerOK])continue;
        P dead=invoke(_api.dead,candidate,nullptr,playerOK);
        if(dead&&!*static_cast<bool*>(object_unbox(dead))){localController=candidate;break;}
    }
    if((_activeLocal?gchandle_get_target(_activeLocal):nullptr)!=localController){
        [self resetLocalState];
        if(localController)_activeLocal=gchandle_new(localController,true);
        else {
            _extras.status=@"Ожидание возрождения. Настройки сохранены.";
            if(now>=_nextPlayerProbe){_nextDiscovery=0;_nextPlayerProbe=now+.5;}
        }
    }
    if(_extras.ready){
        _practice.tick(_extras,_sceneVisuals,localController,localActor,anchorTeam,now);
        _effects.tick(_extras,_sceneVisuals,now);
        _lag.tick(_extras,_sceneVisuals,_practice,now);
        _movementHelpers.tick(_extras,_sceneVisuals,localController,cameraTransform,now,_panel.hidden);
        _nades.tick(_extras,_sceneVisuals,_practice,raw,localController,haveCameraPos?cameraTransform:nullptr,cameraPos,_api.pos);
    }
    if(_trail&&localController){
        while(!_trailPoints.empty()&&now-_trailPoints.front().time>_trailTime)_trailPoints.pop_front();
        if(_trailPoints.empty()||now-_trailPoints.back().time>=.05){
            bool trailOK=true;
            P transform=invoke(_api.tr,localController,nullptr,trailOK);V3 point{};
            if(transform&&[self position:transform result:point ok:trailOK]){
                point.y+=.08f;
                if(!_trailPoints.empty()&&FlameDistanceSquared(point,_trailPoints.back().position)>64)_trailPoints.clear();
                if(_trailPoints.empty()||FlameDistanceSquared(point,_trailPoints.back().position)>.0064){
                    _trailPoints.push_back({point,now});
                }
            }
        }
    }
    P viewCamera=camera;
    if(_extras.ready&&_thirdPerson&&localController&&haveCameraPos){
        if(P view=_thirdView.sample(_extras,_sceneVisuals,camera,cameraTransform,cameraPos,_cameraDistance))viewCamera=view;
    }else if(_thirdView.object)_thirdView.clear(_extras);
    if(_nades.visible&&localController){
        bool nadeOK=true;
        CGPoint marker{};
        V3 stand=_nades.stand;stand.y+=.12f;
        if([self project:stand camera:viewCamera scale:scale height:h result:marker ok:nadeOK])
            _nadeMarks.emplace_back(marker,[NSString stringWithFormat:@"Встань здесь · %.1f м",_nades.distance]);
        if([self project:_nades.aim camera:viewCamera scale:scale height:h result:marker ok:nadeOK])
            _nadeMarks.emplace_back(marker,@"Целься сюда");
    }
    if(_extras.ready&&_nextAgent&&localController){
        _nextAgent=NO;
        bool agentOK=true;
        P animator=invoke(_api.animator,localController,nullptr,agentOK);
        P skeleton=[self isAlive:animator ok:agentOK]?invoke(_api.skeleton,animator,nullptr,agentOK):nullptr;
        if([self isAlive:skeleton ok:agentOK])_extras.nextAgent(skeleton,_api.get,agentOK);
    }
    if(_extras.ready&&(_skyTint||_sceneLight||_playerTint||_sceneVisuals.status.length)){
        bool renderOK=true;
        P body=nullptr,lod=nullptr;
        if(_playerTint&&localController&&now>=_sceneVisuals.nextTick){
            P animator=invoke(_api.animator,localController,nullptr,renderOK);
            P skeleton=[self isAlive:animator ok:renderOK]?invoke(_api.skeleton,animator,nullptr,renderOK):nullptr;
            if([self isAlive:skeleton ok:renderOK]){
                body=invoke(_api.body,skeleton,nullptr,renderOK);
                if(_api.bodyLod)lod=invoke(_api.bodyLod,skeleton,nullptr,renderOK);
            }
        }
        _sceneVisuals.tick(_extras,now,_skyTint,_skyHue,_sceneLight,_sceneHour,_sceneHue,_playerTint,_playerHue,body,lod,viewCamera);
    }
    BOOL fireHeld=NO;
    if(_extras.ready&&localController){
        if((_trigger||_wallShot||_bunnyHop)&&!_input.ready)_input.init(_extras,_sceneVisuals);
        if(_bunnyHop&&now>=_bunnyProbe){
            _input.bunny(_sceneVisuals);
            _bunnyProbe=now+.2;
        }
        bool armsOK=true;
        P ac=invoke(_extras.arms,localController,nullptr,armsOK);
        if([self isAlive:ac ok:armsOK]){
            if(_gloves.desired)_gloves.tick(_extras,_sceneVisuals,ac,now);
            if(_noRecoil){
                bool stepOK=true;_extras.noRecoil(ac,stepOK);
                if(!stepOK)_extras.restoreRecoil();
            }
            if(_noSpread){
                bool stepOK=true;_extras.noSpread(ac,now,stepOK);
                if(!stepOK)_extras.restoreSpread();
            }
            if(_speedHack){
                bool stepOK=true;_extras.speed(localController,_speedFactor,stepOK);
                if(!stepOK)_extras.restoreSpeed();
            }
            if(_unlimitedAmmo){
                bool stepOK=true;_extras.unlimitedAmmo(ac,stepOK);
                if(!stepOK)_extras.ammoObserved=-1;
            }
            if(now>=_observeAfter){
                Runtime::lastException=nil;
                bool skinOK=true;
                bool handled=_extras.cosmetics(ac,_skinRequest,_api.get,now,skinOK);
                if(!skinOK){
                    if(![_skinRequest[@"action"] isEqual:@"sticker"])_extras.cancelCosmeticRetry();
                    _skinRequest=nil;
                    _extras.status=Runtime::lastException?:@"Игра отклонила операцию с внешним видом";
                }else if(handled)_skinRequest=nil;
                _observeAfter=now+.25;
                [_menu updateWeapon:_extras.observedWeapon name:_extras.observedName skin:_extras.observedSkin];
            }
            if((_trigger||_wallShot)&&viewCamera==camera&&_panel.hidden&&haveCameraPos&&anchorTeam.length){
                int hit=_input.actor(_extras,_sceneVisuals,ac,cameraTransform,cameraPos,now,_trigger,_wallShot);
                if(hit!=-2){
                    bool enemy=false;
                    if(hit>0&&hit!=localActor){
                        for(const Target& target:_targets)if(target.actor==hit&&target.network){
                            bool hitOK=true;P player=gchandle_get_target(target.player);
                            P network=gchandle_get_target(target.network);
                            if([self isAlive:player ok:hitOK]){
                                P dead=invoke(_api.dead,player,nullptr,hitOK);
                                NSString* team=network?[[self string:invoke(_api.team,network,nullptr,hitOK)] lowercaseString]:@"";
                                enemy=hitOK&&dead&&!*static_cast<bool*>(object_unbox(dead))&&team.length&&![team isEqual:anchorTeam];
                            }
                            break;
                        }
                    }
                    if(enemy)_input.fire(ac,hit,now,_triggerDelay);
                    else _input.resetTrigger();
                }
            }else _input.resetTrigger();
            if(_aim&&viewCamera==camera&&!_aimFault&&_controls&&_panel.hidden){
                bool inputOK=true;P c=gchandle_get_target(_controls);
                if([self isAlive:c ok:inputOK])fireHeld=_extras.fireHeld(c,inputOK);
            }
        }
    }
    if(!fireHeld)_fireStarted=0;
    else if(!_fireStarted)_fireStarted=now;
    BOOL aimActive=fireHeld&&(now-_fireStarted)*1000>=_aimDelay;
    if(_extras.ready){bool fovOK=true;if(_fovOverride)_extras.fieldOfView(camera,_fovValue,fovOK);else _extras.restoreFov();if(!fovOK){_fovOverride=NO;[_menu setRuntimeOption:Fov value:NO];_extras.restoreFov();}}

    if(_extras.ready&&_spin&&localController&&_panel.hidden){
        bool spinOK=true;
        P animator=invoke(_api.animator,localController,nullptr,spinOK);
        P skeleton=[self isAlive:animator ok:spinOK]?invoke(_api.skeleton,animator,nullptr,spinOK):nullptr;
        float dt=fmax(.001,fmin(.05,_displayLink.targetTimestamp-_displayLink.timestamp));
        if([self isAlive:skeleton ok:spinOK])_sceneVisuals.spin(_extras,skeleton,camera,_spinSpeed*dt,_spinMode,_spinPitch,now);
    }else if(_sceneVisuals.spinOwner)_sceneVisuals.restoreSpin(_extras);
    if(_trail){
        bool trailOK=true,previousVisible=false;CGPoint previous{};
        for(const TrailPoint& sample:_trailPoints){
            CGPoint point{};
            bool visible=[self project:sample.position camera:viewCamera scale:scale height:h result:point ok:trailOK];
            if(visible&&previousVisible){
                float opacity=std::clamp(1.f-(float)(now-sample.time)/_trailTime,0.f,1.f);
                _trailSegments.push_back({{previous,point,true},opacity});
            }
            previousVisible=visible;previous=point;
        }
    }
    V3 aimTarget{};BOOL haveAimTarget=NO;float nearest=_aimRadius*_aimRadius;
    size_t filtered=0,unknown=0,visible=0;
    for(const Target& target:_targets){
        RootScope targetRoots;bool ok=true;P player=gchandle_get_target(target.player);if(![self isAlive:player ok:ok])continue;
        P isDead=invoke(_api.dead,player,nullptr,ok);if(isDead&&*static_cast<bool*>(object_unbox(isDead)))continue;
        if(target.actor>=0&&(target.actor==localActor||target.actor==anchorActor))continue;
        P network=target.network?gchandle_get_target(target.network):nullptr;
        NSString* team=network?[[self string:invoke(_api.team,network,nullptr,ok)] lowercaseString]:@"";
        BOOL known=team.length>0&&anchorTeam.length>0,enemy=known&&![team isEqualToString:anchorTeam];
        if(_onlyEnemies&&!known){unknown++;continue;}if(_onlyEnemies&&!enemy){filtered++;continue;}
        if(_maxDistance>0&&haveCameraPos){P transform=invoke(_api.tr,player,nullptr,ok);V3 pos;if(transform&&[self position:transform result:pos ok:ok]){double dx=pos.x-cameraPos.x,dy=pos.y-cameraPos.y,dz=pos.z-cameraPos.z;if(dx*dx+dy*dy+dz*dz>_maxDistance*_maxDistance)continue;}}
        if(aimActive&&enemy&&haveCameraPos){
            uint32_t points[]={target.head,
                target.chest>=0?target.bones[target.chest].handle:0,
                target.pelvis>=0?target.bones[target.pelvis].handle:0};
            for(int i=0;i<3&&ok;i++){
                if((_aimPoint!=3&&_aimPoint!=i)||!points[i])continue;
                P bone=gchandle_get_target(points[i]);V3 world;CGPoint point;
                if(![self isAlive:bone ok:ok]||![self position:bone result:world ok:ok]||
                    ![self project:world camera:camera scale:scale height:h result:point ok:ok])continue;
                float dx=point.x-CGRectGetMidX(self.bounds),dy=point.y-CGRectGetMidY(self.bounds),d=dx*dx+dy*dy;
                if(d<nearest&&(_aimDistance==0||FlameDistanceSquared(world,cameraPos)<=_aimDistance*_aimDistance)){
                    nearest=d;aimTarget=world;haveAimTarget=YES;
                }
            }
        }
        std::vector<CGPoint> points(target.bones.size());std::vector<bool> valid(target.bones.size(),false);
        CGFloat left=INFINITY,top=INFINITY,right=-INFINITY,bottom=-INFINITY;size_t projected=0;BOOL nearPlane=NO;
        if(_showBoxes||_showSkeleton||_showHealth||_showNames||_showDistance||_showTracers||_headCircle){
            for(size_t j=0;j<target.bones.size()&&ok;j++){
                RootScope boneRoots;P transform=gchandle_get_target(target.bones[j].handle);if(![self isAlive:transform ok:ok])continue;
                V3 pos;if(![self position:transform result:pos ok:ok])continue;CGPoint p;
                if(![self project:pos camera:viewCamera scale:scale height:h result:p ok:ok]){nearPlane=YES;continue;}
                points[j]=p;valid[j]=true;projected++;left=fmin(left,p.x);right=fmax(right,p.x);top=fmin(top,p.y);bottom=fmax(bottom,p.y);
            }
        }
        if(!ok)continue;
        if(nearPlane)continue;
        if(projected<4||right<=left||bottom<=top){
            P transform=invoke(_api.tr,player,nullptr,ok);V3 pos;CGPoint p;
            if((_showBoxes||_showSkeleton)&&transform&&[self position:transform result:pos ok:ok]&&[self project:pos camera:viewCamera scale:scale height:h result:p ok:ok]&&CGRectContainsPoint(self.bounds,p))_dots.push_back(p);
            continue;
        }
        // Padding only covers skin around actual projected joints, not animation-wide renderer bounds.
        CGFloat padX=fmax(2,(right-left)*0.04),padY=fmax(3,(bottom-top)*0.07);
        CGRect box=CGRectMake(left-padX,top-padY,right-left+2*padX,bottom-top+2*padY);
        if(!CGRectIntersectsRect(box,self.bounds))continue;visible++;
        if(_headCircle&&target.head){P head=gchandle_get_target(target.head);V3 world;CGPoint point;if([self isAlive:head ok:ok]&&[self position:head result:world ok:ok]&&[self project:world camera:viewCamera scale:scale height:h result:point ok:ok])_headMarkers.push_back({point,fmax(2,fmin(12,box.size.height*.055)),(bool)enemy});}
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
    }
    if(ok&&haveAimTarget&&cameraTransform&&nearest>_aimDeadzone*_aimDeadzone){
        bool aimOK=true;
        float dt=fmax(0.001,fmin(0.05,_displayLink.targetTimestamp-_displayLink.timestamp));
        _extras.aim(localController,cameraTransform,cameraPos,aimTarget,1-expf(-_aimSmoothing*dt),_aimMaxTurn*dt,aimOK);
        if(!aimOK){_aimFault=YES;_extras.status=@"AIM приостановлен для этого персонажа после ошибки игры";}
    }
    NSString* diagnostics=_extras.summary();
    if(_practice.status.length)diagnostics=[diagnostics stringByAppendingFormat:@"  %@",_practice.status];
    if(_gloves.status.length)diagnostics=[diagnostics stringByAppendingFormat:@"  %@",_gloves.status];
    if(_input.status.length)diagnostics=[diagnostics stringByAppendingFormat:@"  %@",_input.status];
    if(_sceneVisuals.status.length)diagnostics=[diagnostics stringByAppendingFormat:@"  %@",_sceneVisuals.status];
    for(NSString* helper in @[_effects.status,_nades.status,_lag.status])
        if(helper.length)diagnostics=[diagnostics stringByAppendingFormat:@"  %@",helper];
    if(_noSpread&&localController&&!_extras.spreadFields)
        diagnostics=[diagnostics stringByAppendingString:@"  No Spread: нет подтверждённых изменений"];
    _extraLabel.text=[NSString stringWithFormat:@"%@\n%@",_extras.status,diagnostics];
    _status=[NSString stringWithFormat:@"v12: %zu players · %@ team %@\nrigs %zu/%zu · bones %zu · missing %zu\nallies filtered %zu · unknown teams %zu",visible,watching?@"watched":@"local",anchorTeam.length?anchorTeam:@"unknown",_rigs,_targets.size(),_boneCount,_missingRig,filtered,unknown];
    [self setNeedsDisplay];
}
- (UIColor*)color:(BOOL)enemy{return enemy?FlameAccent(_accent):[UIColor colorWithRed:0.35 green:1 blue:0.45 alpha:1];}
- (UIBezierPath*)boxPath:(CGRect)rect {
    if(!_corners)return [UIBezierPath bezierPathWithRect:rect];
    UIBezierPath* path=[UIBezierPath bezierPath];CGFloat x=rect.origin.x,y=rect.origin.y,r=CGRectGetMaxX(rect),b=CGRectGetMaxY(rect),len=fmin(rect.size.width,rect.size.height)*.25;
    CGPoint segments[][3]={{{x,y+len},{x,y},{x+len,y}},{{r-len,y},{r,y},{r,y+len}},{{x,b-len},{x,b},{x+len,b}},{{r-len,b},{r,b},{r,b-len}}};
    for(auto& points:segments){[path moveToPoint:points[0]];[path addLineToPoint:points[1]];[path addLineToPoint:points[2]];}return path;
}
- (void)stroke:(const std::vector<Segment>&)segments width:(CGFloat)width {
    for(const Segment& s:segments){UIBezierPath* p=[UIBezierPath bezierPath];[p moveToPoint:s.a];[p addLineToPoint:s.b];[UIColor.blackColor setStroke];p.lineWidth=width+1.5;[p stroke];[[self color:s.enemy]setStroke];p.lineWidth=width;[p stroke];}
}
- (void)drawRect:(CGRect)rect {
    CGContextRef context=UIGraphicsGetCurrentContext();CGContextSaveGState(context);CGContextClipToRect(context,self.bounds);
    if(_enabled&&_aim&&_aimRing){[[UIColor colorWithWhite:1 alpha:0.45] setStroke];UIBezierPath* circle=[UIBezierPath bezierPathWithOvalInRect:CGRectMake(self.bounds.size.width/2-_aimRadius,self.bounds.size.height/2-_aimRadius,2*_aimRadius,2*_aimRadius)];circle.lineWidth=0.8;[circle stroke];}
    if(_enabled&&_vignette&&_vignetteGradient){
        CGPoint center=CGPointMake(CGRectGetMidX(self.bounds),CGRectGetMidY(self.bounds));
        CGFloat radius=hypot(center.x,center.y);
        if(radius>0)CGContextDrawRadialGradient(context,_vignetteGradient,center,radius*.4,center,radius,0);
    }
    for(const auto& mark:_nadeMarks){
        CGPoint p=mark.first;
        [UIColor.cyanColor setStroke];
        UIBezierPath* ring=[UIBezierPath bezierPathWithOvalInRect:CGRectMake(p.x-4,p.y-4,8,8)];
        [ring stroke];
        [mark.second drawAtPoint:CGPointMake(p.x+7,p.y-6) withAttributes:@{
            NSForegroundColorAttributeName:UIColor.cyanColor,NSFontAttributeName:[UIFont boldSystemFontOfSize:11],
            NSStrokeColorAttributeName:UIColor.blackColor,NSStrokeWidthAttributeName:@(-2)}];
    }
    if(_filled)for(const Item& item:_items){[[[self color:item.enemy] colorWithAlphaComponent:.09]setFill];UIRectFill(item.box);}
    for(const auto& entry:_trailSegments){
        UIBezierPath* path=[UIBezierPath bezierPath];
        [path moveToPoint:entry.first.a];[path addLineToPoint:entry.first.b];
        [[FlameAccent(_accent) colorWithAlphaComponent:entry.second]setStroke];
        path.lineWidth=2.2;path.lineCapStyle=kCGLineCapRound;[path stroke];
    }
    [self stroke:_tracers width:fmax(.6,_lineWidth*.65)];[self stroke:_segments width:_lineWidth];
    for(const Item& item:_items){
        if(_showBoxes){UIBezierPath* p=[self boxPath:item.box];
            if(_glow){[[[self color:item.enemy]colorWithAlphaComponent:.2]setStroke];p.lineWidth=_lineWidth+5;[p stroke];}
            [UIColor.blackColor setStroke];p.lineWidth=_lineWidth+1.6;[p stroke];[[self color:item.enemy]setStroke];p.lineWidth=_lineWidth;[p stroke];}
        if(item.text.length){NSDictionary* attrs=@{NSForegroundColorAttributeName:[self color:item.enemy],NSFontAttributeName:[UIFont boldSystemFontOfSize:10],NSStrokeColorAttributeName:UIColor.blackColor,NSStrokeWidthAttributeName:@(-2)};
            CGSize size=[item.text sizeWithAttributes:attrs];[item.text drawAtPoint:CGPointMake(CGRectGetMidX(item.box)-size.width/2,fmax(0,item.box.origin.y-size.height-2)) withAttributes:attrs];}
    }
    [UIColor.cyanColor setStroke];for(CGPoint p:_dots){UIBezierPath* path=[UIBezierPath bezierPathWithOvalInRect:CGRectMake(p.x-2,p.y-2,4,4)];path.lineWidth=1.2;[path stroke];}
    for(const HeadMarker& marker:_headMarkers){[[self color:marker.enemy]setStroke];UIBezierPath* circle=[UIBezierPath bezierPathWithOvalInRect:CGRectMake(marker.point.x-marker.radius,marker.point.y-marker.radius,2*marker.radius,2*marker.radius)];circle.lineWidth=_lineWidth;[circle stroke];}
    if(_enabled&&_crosshair){CGFloat x=self.bounds.size.width/2,y=self.bounds.size.height/2;UIBezierPath* p=[UIBezierPath bezierPath];[p moveToPoint:CGPointMake(x-7,y)];[p addLineToPoint:CGPointMake(x-2,y)];[p moveToPoint:CGPointMake(x+2,y)];[p addLineToPoint:CGPointMake(x+7,y)];[p moveToPoint:CGPointMake(x,y-7)];[p addLineToPoint:CGPointMake(x,y-2)];[p moveToPoint:CGPointMake(x,y+2)];[p addLineToPoint:CGPointMake(x,y+7)];[UIColor.blackColor setStroke];p.lineWidth=3;[p stroke];[FlameAccent(_accent)setStroke];p.lineWidth=1.2;[p stroke];}
    if(_enabled&&_centerDot){
        CGFloat x=CGRectGetMidX(self.bounds),y=CGRectGetMidY(self.bounds);
        [[UIColor.blackColor colorWithAlphaComponent:.8]setFill];
        [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(x-2.5,y-2.5,5,5)]fill];
        [FlameAccent(_accent)setFill];
        [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(x-1.5,y-1.5,3,3)]fill];
    }
    CGContextRestoreGState(context);
    if(_enabled&&_showFps)[[NSString stringWithFormat:@"ESP %.0f FPS",_observedFPS]drawAtPoint:CGPointMake(self.bounds.size.width-116,19) withAttributes:@{NSForegroundColorAttributeName:FlameAccent(_accent),NSFontAttributeName:[UIFont boldSystemFontOfSize:11]}];
    if(_debug||!_enabled||!_apiReady||(_onlyEnemies&&_items.empty()))[_status drawInRect:CGRectMake(48,18,480,55) withAttributes:@{NSForegroundColorAttributeName:UIColor.cyanColor,NSFontAttributeName:[UIFont systemFontOfSize:11]}];
}
- (void)dealloc {if(_vignetteGradient)CGGradientRelease(_vignetteGradient);}
@end
__attribute__((constructor))static void startFlame(){dispatch_async(dispatch_get_main_queue(),^{
    [NSTimer scheduledTimerWithTimeInterval:1 repeats:YES block:^(NSTimer* timer){for(UIScene* scene in UIApplication.sharedApplication.connectedScenes){
        if(scene.activationState!=UISceneActivationStateForegroundActive||![scene isKindOfClass:UIWindowScene.class])continue;
        for(UIWindow* window in ((UIWindowScene*)scene).windows)if(window.isKeyWindow){[window addSubview:[[FlameMarkers alloc]initWithFrame:window.bounds]];[timer invalidate];return;}
    }}];
});}
