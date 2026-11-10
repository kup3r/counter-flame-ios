#pragma once
#include "FlameOptions.hpp"
#include "FlameCatalog.hpp"
#include "FlameSkinArtwork.hpp"
#include "FlameStickerCatalog.hpp"
static UIColor* FlameAccent(NSInteger index){switch(index){case 1:return [UIColor colorWithRed:0.95 green:0.63 blue:0.15 alpha:1];case 2:return [UIColor colorWithRed:0.22 green:0.85 blue:0.48 alpha:1];case 3:return [UIColor colorWithRed:1 green:0.31 blue:0.28 alpha:1];default:return [UIColor colorWithRed:0.1 green:0.82 blue:0.83 alpha:1];}}
@protocol FlameMenuDelegate<NSObject>
- (void)flameSetOption:(NSInteger)tag value:(BOOL)value;
- (void)flameSetNumber:(NSString*)key value:(double)value;
- (void)flameSkinAction:(NSString*)action weapon:(NSDictionary*)weapon skin:(NSDictionary*)skin;
- (void)flameConfigAction:(NSString*)action;
@end
@interface FlameMiniPreview:UIView
@property(nonatomic)BOOL boxes,skeleton,corners,filled,health,head;
@property(nonatomic)NSInteger accent;
@end
@implementation FlameMiniPreview
- (void)drawRect:(CGRect)rect{
    UIColor* ink=FlameAccent(self.accent);CGRect area=CGRectInset(self.bounds,24,15);CGFloat w=area.size.width,h=area.size.height;
    CGPoint p[]={CGPointMake(.51,.14),CGPointMake(.51,.27),CGPointMake(.5,.46),CGPointMake(.49,.60),CGPointMake(.33,.32),CGPointMake(.23,.47),CGPointMake(.7,.29),CGPointMake(.79,.4),CGPointMake(.37,.78),CGPointMake(.33,.96),CGPointMake(.64,.77),CGPointMake(.72,.96)};
    for(auto& v:p)v=CGPointMake(area.origin.x+v.x*w,area.origin.y+v.y*h);
    CGRect box=CGRectMake(area.origin.x+.16*w,area.origin.y+.06*h,.69*w,.94*h);
    if(self.filled){[[ink colorWithAlphaComponent:.09]setFill];UIRectFill(box);}
    [ink setStroke];UIBezierPath* path=[UIBezierPath bezierPath];path.lineWidth=1.3;
    if(self.boxes){if(self.corners){CGFloat x=box.origin.x,y=box.origin.y,r=CGRectGetMaxX(box),b=CGRectGetMaxY(box),s=box.size.width*.23;CGPoint edges[][3]={{{x,y+s},{x,y},{x+s,y}},{{r-s,y},{r,y},{r,y+s}},{{x,b-s},{x,b},{x+s,b}},{{r-s,b},{r,b},{r,b-s}}};for(auto& e:edges){[path moveToPoint:e[0]];[path addLineToPoint:e[1]];[path addLineToPoint:e[2]];}}else [path appendPath:[UIBezierPath bezierPathWithRect:box]];}
    if(self.skeleton){int pairs[][2]={{0,1},{1,2},{2,3},{1,4},{4,5},{1,6},{6,7},{3,8},{8,9},{3,10},{10,11}};for(auto& e:pairs){[path moveToPoint:p[e[0]]];[path addLineToPoint:p[e[1]]];}}
    if(self.head)[path appendPath:[UIBezierPath bezierPathWithOvalInRect:CGRectMake(p[0].x-5,p[0].y-5,10,10)]];
    [path stroke];if(self.health)[@"HP 100" drawAtPoint:CGPointMake(box.origin.x,box.origin.y-12) withAttributes:@{NSFontAttributeName:[UIFont boldSystemFontOfSize:8],NSForegroundColorAttributeName:ink}];
}
@end
@interface FlameSkinCard:UICollectionViewCell
@property(nonatomic,strong) UIImageView* artwork;
@property(nonatomic,strong) UILabel* caption;
@end
@implementation FlameSkinCard
- (instancetype)initWithFrame:(CGRect)frame{
    if((self=[super initWithFrame:frame])){
        self.backgroundColor=[UIColor colorWithRed:.17 green:.20 blue:.20 alpha:1];
        self.layer.cornerRadius=6;
        self.layer.borderWidth=1;
        _artwork=[UIImageView new];
        _artwork.contentMode=UIViewContentModeScaleAspectFit;
        _artwork.tintColor=[UIColor colorWithWhite:.65 alpha:1];
        [self.contentView addSubview:_artwork];
        _caption=[UILabel new];
        _caption.font=[UIFont systemFontOfSize:10 weight:UIFontWeightSemibold];
        _caption.textColor=UIColor.whiteColor;
        _caption.textAlignment=NSTextAlignmentCenter;
        _caption.numberOfLines=2;
        [self.contentView addSubview:_caption];
    }
    return self;
}
- (void)layoutSubviews{
    [super layoutSubviews];
    CGRect b=self.contentView.bounds;
    _artwork.frame=CGRectMake(6,4,b.size.width-12,b.size.height-29);
    _caption.frame=CGRectMake(3,b.size.height-25,b.size.width-6,23);
}
@end
@interface FlameMenu:UIView<UITableViewDataSource,UITableViewDelegate,UICollectionViewDataSource,UICollectionViewDelegate,UITextFieldDelegate,UIGestureRecognizerDelegate>
@property(nonatomic,weak)id<FlameMenuDelegate> delegate;
@property(nonatomic,strong,readonly)UISwitch* master;
@property(nonatomic,strong,readonly)UILabel* statusLabel;
- (instancetype)initWithFrame:(CGRect)frame config:(NSDictionary*)config;
- (void)sync:(NSDictionary*)config;
- (void)setRuntimeOption:(NSInteger)tag value:(BOOL)value;
- (void)updateWeapon:(NSInteger)weapon name:(NSString*)name skin:(NSString*)skin;
- (void)showTab:(NSInteger)index;
@end
@implementation FlameMenu{
    NSMutableDictionary* _config;NSMutableDictionary<NSNumber*,UISwitch*>* _switches;NSMutableDictionary<NSNumber*,NSString*>* _sliderKeys;
    NSMutableDictionary<NSString*,UISlider*>* _sliders;NSMutableDictionary<NSString*,UILabel*>* _values;
    UIView* _sidebar;UIView* _header;UIView* _content;NSArray<UIView*>* _pages;NSMutableArray<UIButton*>* _tabs;
    UILabel* _pageTitle;UILabel* _weaponLabel;UILabel* _statusLabel;FlameMiniPreview* _preview;
    UITableView* _weaponTable;UICollectionView* _skinGrid;UILabel* _skinEmpty;UITextField* _search;UISegmentedControl* _categories;
    NSArray* _allWeapons;NSArray* _weapons;NSArray<NSDictionary*>* _skinCards;NSDictionary* _weaponFilter;NSDictionary* _selectedWeapon;NSDictionary* _selectedSkin;
    UIButton* _applySkin;UIButton* _applyModel;UISegmentedControl* _stickerSlot;NSInteger _currentWeapon,_activeTab;
    UISegmentedControl* _aimPoints;UISegmentedControl* _colorSegments;UISegmentedControl* _fpsSegments;
    CGPoint _dragOrigin;
}
- (UIColor*)textColor{return [UIColor colorWithRed:.27 green:.28 blue:.26 alpha:1];}
- (UILabel*)label:(NSString*)text frame:(CGRect)frame size:(CGFloat)size bold:(BOOL)bold parent:(UIView*)parent{
    UILabel* label=[[UILabel alloc]initWithFrame:frame];label.text=text;label.font=bold?[UIFont boldSystemFontOfSize:size]:[UIFont systemFontOfSize:size];label.textColor=[self textColor];[parent addSubview:label];return label;
}
- (UIButton*)button:(NSString*)text frame:(CGRect)frame target:(SEL)selector tag:(NSInteger)tag parent:(UIView*)parent{
    UIButton* b=[UIButton buttonWithType:UIButtonTypeSystem];b.frame=frame;b.tag=tag;[b setTitle:text forState:UIControlStateNormal];b.titleLabel.font=[UIFont boldSystemFontOfSize:12];
    b.backgroundColor=[UIColor colorWithRed:.31 green:.34 blue:.34 alpha:1];[b setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];b.layer.cornerRadius=4;[b addTarget:self action:selector forControlEvents:UIControlEventTouchUpInside];[parent addSubview:b];return b;
}
- (UIScrollView*)scrollPage{
    UIScrollView* p=[[UIScrollView alloc]initWithFrame:_content.bounds];p.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;return p;
}
- (void)section:(NSString*)text x:(CGFloat)x y:(CGFloat)y width:(CGFloat)width parent:(UIView*)parent{
    [self label:text frame:CGRectMake(x,y,width,20) size:11 bold:YES parent:parent];UIView* line=[[UIView alloc]initWithFrame:CGRectMake(x,y+23,width,1)];line.backgroundColor=[UIColor colorWithRed:.68 green:.75 blue:.72 alpha:1];[parent addSubview:line];
}
- (UISwitch*)option:(NSInteger)tag title:(NSString*)title x:(CGFloat)x y:(CGFloat)y width:(CGFloat)width parent:(UIView*)parent{
    UIButton* row=[UIButton buttonWithType:UIButtonTypeCustom];row.frame=CGRectMake(x,y,width,30);row.tag=1000+tag;[row addTarget:self action:@selector(toggleRow:) forControlEvents:UIControlEventTouchUpInside];[parent addSubview:row];
    [self label:title frame:CGRectMake(x,y,width-64,28) size:12 bold:NO parent:parent];UISwitch* toggle=[[UISwitch alloc]initWithFrame:CGRectZero];[toggle sizeToFit];CGFloat effectiveWidth=toggle.bounds.size.width*.72;toggle.transform=CGAffineTransformMakeScale(.72,.72);toggle.center=CGPointMake(x+width-effectiveWidth/2-3,y+15);toggle.tag=tag;
    toggle.on=[_config[@"options"][FlameOptionKey(tag)] boolValue];toggle.onTintColor=FlameAccent(0);[toggle addTarget:self action:@selector(changeOption:) forControlEvents:UIControlEventValueChanged];[parent addSubview:toggle];_switches[@(tag)]=toggle;return toggle;
}
- (void)slider:(NSString*)key title:(NSString*)title min:(float)min max:(float)max x:(CGFloat)x y:(CGFloat)y width:(CGFloat)width parent:(UIView*)parent{
    [self label:title frame:CGRectMake(x,y,width-62,20) size:12 bold:NO parent:parent];UILabel* value=[self label:@"" frame:CGRectMake(x+width-70,y,70,20) size:11 bold:YES parent:parent];value.textAlignment=NSTextAlignmentRight;_values[key]=value;
    UISlider* slider=[[UISlider alloc]initWithFrame:CGRectMake(x,y+20,width,27)];slider.minimumValue=min;slider.maximumValue=max;slider.value=[_config[key] floatValue];slider.minimumTrackTintColor=FlameAccent(1);slider.maximumTrackTintColor=[UIColor colorWithWhite:.37 alpha:1];slider.tag=500+_sliderKeys.count;_sliderKeys[@(slider.tag)]=key;_sliders[key]=slider;
    [slider addTarget:self action:@selector(changeSlider:) forControlEvents:UIControlEventValueChanged];[parent addSubview:slider];[self refreshValue:key];
}
- (void)refreshValue:(NSString*)key{
    double n=[_config[key] doubleValue];_values[key].text=([key isEqualToString:@"maxDistance"]||[key isEqualToString:@"aimDistance"])&&n==0?@"∞":[key isEqualToString:@"speedFactor"]?[NSString stringWithFormat:@"× %.2f",n]:[NSString stringWithFormat:@"%.1f",n];
}
- (instancetype)initWithFrame:(CGRect)frame config:(NSDictionary*)config{
    if((self=[super initWithFrame:frame])){
        _config=[config mutableCopy];_config[@"options"]=[config[@"options"] mutableCopy];_currentWeapon=-1;
        _switches=[NSMutableDictionary dictionary];_sliderKeys=[NSMutableDictionary dictionary];_sliders=[NSMutableDictionary dictionary];_values=[NSMutableDictionary dictionary];_tabs=[NSMutableArray array];
        self.backgroundColor=[UIColor colorWithRed:.96 green:.93 blue:.85 alpha:.97];self.layer.cornerRadius=7;self.layer.borderWidth=1;self.layer.borderColor=[UIColor colorWithWhite:.35 alpha:.45].CGColor;self.clipsToBounds=YES;
        CGFloat side=108;_sidebar=[[UIView alloc]initWithFrame:CGRectMake(0,0,side,frame.size.height)];_sidebar.autoresizingMask=UIViewAutoresizingFlexibleHeight;_sidebar.backgroundColor=[UIColor colorWithRed:.58 green:.6 blue:.57 alpha:1];[self addSubview:_sidebar];
        UILabel* brand=[self label:@"◉ FLAME" frame:CGRectMake(10,11,98,29) size:19 bold:YES parent:_sidebar];brand.textColor=[UIColor colorWithRed:1 green:.85 blue:.27 alpha:1];
        [self label:@"iOS · v10" frame:CGRectMake(15,42,88,18) size:11 bold:YES parent:_sidebar];
        NSArray* names=@[@"◉ Visual",@"⌖ Aim",@"▦ Misc",@"◇ Skins",@"▣ Config"];
        for(NSInteger i=0;i<names.count;i++){UIButton* b=[self button:names[i] frame:CGRectMake(8,77+i*42,92,35) target:@selector(selectTab:) tag:100+i parent:_sidebar];[_tabs addObject:b];}
        _master=[self option:Master title:@"ON" x:10 y:frame.size.height-45 width:89 parent:_sidebar];_master.autoresizingMask=UIViewAutoresizingFlexibleTopMargin;
        _header=[[UIView alloc]initWithFrame:CGRectMake(side,0,frame.size.width-side,41)];_header.autoresizingMask=UIViewAutoresizingFlexibleWidth;[self addSubview:_header];
        _pageTitle=[self label:@"Visual" frame:CGRectMake(15,9,_header.bounds.size.width-65,25) size:17 bold:YES parent:_header];
        UIButton* close=[self button:@"×" frame:CGRectMake(_header.bounds.size.width-35,6,28,28) target:@selector(closeMenu) tag:0 parent:_header];close.autoresizingMask=UIViewAutoresizingFlexibleLeftMargin;
        UIPanGestureRecognizer* pan=[[UIPanGestureRecognizer alloc]initWithTarget:self action:@selector(drag:)];[_header addGestureRecognizer:pan];
        _content=[[UIView alloc]initWithFrame:CGRectMake(side+14,45,frame.size.width-side-28,frame.size.height-98)];_content.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;[self addSubview:_content];
        _statusLabel=[self label:@"Готов к настройке. Включи модуль после загрузки матча." frame:CGRectMake(side+14,frame.size.height-48,frame.size.width-side-28,42) size:10 bold:NO parent:self];_statusLabel.numberOfLines=3;_statusLabel.autoresizingMask=UIViewAutoresizingFlexibleTopMargin|UIViewAutoresizingFlexibleWidth;
        UIView* visual=[self makeVisual],*aim=[self makeAim],*misc=[self makeMisc],*skins=[self makeSkins],*settings=[self makeConfig];_pages=@[visual,aim,misc,skins,settings];for(UIView* p in _pages){[_content addSubview:p];p.hidden=YES;}
        [self showTab:0];[self refreshPreview];
    }return self;
}
- (UIView*)makeVisual{
    UIScrollView* p=[self scrollPage];CGFloat w=p.bounds.size.width,col=(w-20)/2;p.contentSize=CGSizeMake(w,730);
    [self section:@"ESP" x:0 y:0 width:col parent:p];NSArray* rows=@[@[@(Enemies),@"Только противники"],@[@(Boxes),@"Рамки"],@[@(CornerBox),@"Угловые рамки"],@[@(FilledBox),@"Заливка"],@[@(Skeleton),@"Скелет"],@[@(HeadCircle),@"Круг головы"],@[@(Health),@"Здоровье"],@[@(Names),@"Имена"],@[@(Distance),@"Дистанция"],@[@(Tracers),@"Линии к игрокам"]];
    for(NSInteger i=0;i<rows.count;i++)[self option:[rows[i][0] integerValue] title:rows[i][1] x:0 y:30+i*30 width:col parent:p];
    [self option:Glow title:@"Подсветка рамок" x:0 y:335 width:col parent:p];
    [self option:Vignette title:@"Затемнение краёв" x:0 y:366 width:col parent:p];
    [self slider:@"lineWidth" title:@"Толщина линий" min:.6 max:3 x:0 y:410 width:col parent:p];
    [self option:Trail title:@"След за персонажем" x:0 y:466 width:col parent:p];
    [self slider:@"trailTime" title:@"Длина следа (с)" min:.5 max:5 x:0 y:500 width:col parent:p];
    [self section:@"ESP Preview" x:col+20 y:0 width:col parent:p];_preview=[[FlameMiniPreview alloc]initWithFrame:CGRectMake(col+20,30,col,151)];_preview.backgroundColor=[UIColor colorWithWhite:1 alpha:.3];_preview.layer.borderWidth=.5;_preview.layer.borderColor=[UIColor colorWithWhite:.7 alpha:1].CGColor;[p addSubview:_preview];
    [self option:Crosshair title:@"Прицел" x:col+20 y:190 width:col parent:p];[self option:FPSCounter title:@"Счётчик FPS" x:col+20 y:220 width:col parent:p];[self option:Debug title:@"Диагностика" x:col+20 y:250 width:col parent:p];
    [self option:CenterDot title:@"Точка в центре" x:col+20 y:282 width:col parent:p];
    [self slider:@"maxDistance" title:@"Макс. дистанция (u)" min:0 max:250 x:col+20 y:322 width:col parent:p];
    [self label:@"Цвет противников" frame:CGRectMake(col+20,382,col,20) size:12 bold:NO parent:p];UISegmentedControl* colors=[[UISegmentedControl alloc]initWithItems:@[@"Cyan",@"Gold",@"Green",@"Red"]];_colorSegments=colors;colors.frame=CGRectMake(col+20,410,col,28);colors.selectedSegmentIndex=[_config[@"accent"] integerValue];[colors addTarget:self action:@selector(changeColor:) forControlEvents:UIControlEventValueChanged];[p addSubview:colors];
    [self section:@"Сцена" x:col+20 y:456 width:col parent:p];
    [self option:SkyTint title:@"Цвет неба" x:col+20 y:485 width:col parent:p];
    [self slider:@"skyHue" title:@"Оттенок неба (°)" min:0 max:359 x:col+20 y:518 width:col parent:p];
    [self option:SceneLight title:@"Время суток · свет" x:col+20 y:572 width:col parent:p];
    [self slider:@"sceneHour" title:@"Время суток (ч)" min:0 max:24 x:col+20 y:605 width:col parent:p];
    [self section:@"Персонаж" x:0 y:552 width:col parent:p];
    [self option:PlayerTint title:@"Окрас персонажа" x:0 y:580 width:col parent:p];
    [self slider:@"playerHue" title:@"Оттенок · 360 = RGB" min:0 max:360 x:0 y:617 width:col parent:p];
    [self button:@"Следующий агент команды" frame:CGRectMake(0,675,col,32) target:@selector(nextAgent) tag:0 parent:p];
    return p;
}
- (void)nextAgent{[self.delegate flameConfigAction:@"nextAgent"];}
- (UIView*)makeAim{
    UIScrollView* p=[self scrollPage];
    CGFloat w=p.bounds.size.width,col=(w-22)/2,right=col+22;
    p.contentSize=CGSizeMake(w,360);
    [self section:@"AIM / Weapon" x:0 y:0 width:col parent:p];
    [self option:Aim title:@"AIM при огне" x:0 y:32 width:col parent:p];
    [self option:AimRing title:@"Показывать радиус" x:0 y:65 width:col parent:p];
    [self option:Recoil title:@"No Recoil · тест" x:0 y:98 width:col parent:p];
    [self option:NoSpread title:@"No Spread · клиент" x:0 y:131 width:col parent:p];
    [self slider:@"aimRadius" title:@"Радиус наведения (pt)" min:5 max:180 x:right y:0 width:col parent:p];
    [self slider:@"aimSmoothing" title:@"Скорость реакции" min:1 max:25 x:right y:52 width:col parent:p];
    [self slider:@"aimDelay" title:@"Задержка от огня (мс)" min:0 max:500 x:right y:104 width:col parent:p];
    [self slider:@"aimMaxTurn" title:@"Предел поворота (°/с)" min:15 max:360 x:right y:156 width:col parent:p];
    [self slider:@"aimDeadzone" title:@"Мёртвая зона (pt)" min:0 max:30 x:right y:208 width:col parent:p];
    [self slider:@"aimDistance" title:@"Макс. дистанция (u)" min:0 max:250 x:right y:260 width:col parent:p];
    _aimPoints=[[UISegmentedControl alloc]initWithItems:@[@"Голова",@"Грудь",@"Таз",@"Ближ."]];
    _aimPoints.frame=CGRectMake(0,165,col,29);
    _aimPoints.selectedSegmentIndex=[_config[@"aimPoint"] integerValue];
    [_aimPoints addTarget:self action:@selector(changeAimPoint:) forControlEvents:UIControlEventValueChanged];[p addSubview:_aimPoints];
    [self option:Trigger title:@"TriggerBot · тест" x:0 y:199 width:col parent:p];
    [self slider:@"triggerDelay" title:@"Задержка TriggerBot (мс)" min:0 max:350 x:0 y:231 width:col parent:p];
    UILabel* note=[self label:@"Наведение при удержании огня. Проверка видимости отсутствует.\n\nПоказания No Spread отображаются внизу." frame:CGRectMake(0,280,col,75) size:11 bold:NO parent:p];
    note.numberOfLines=0;return p;
}
- (UIView*)makeMisc{
    UIScrollView* p=[self scrollPage];CGFloat w=p.bounds.size.width,col=(w-22)/2;p.contentSize=CGSizeMake(w,365);
    [self section:@"Movement" x:0 y:0 width:col parent:p];[self option:Speed title:@"SpeedHack · клиент" x:0 y:32 width:col parent:p];[self slider:@"speedFactor" title:@"Множитель скорости" min:1 max:3 x:0 y:75 width:col parent:p];
    [self option:BunnyHop title:@"Bunnyhop при удержании" x:0 y:130 width:col parent:p];
    [self section:@"Ammo / Camera" x:col+22 y:0 width:col parent:p];[self option:Ammo title:@"Unlimited Ammo · клиент" x:col+22 y:32 width:col parent:p];[self option:Fov title:@"Фиксированный FOV" x:col+22 y:65 width:col parent:p];[self slider:@"fov" title:@"Угол обзора" min:60 max:120 x:col+22 y:105 width:col parent:p];
    [self option:ThirdPerson title:@"Вид от третьего лица · тест" x:col+22 y:159 width:col parent:p];
    [self slider:@"cameraDistance" title:@"Дистанция камеры" min:1 max:5 x:col+22 y:194 width:col parent:p];
    [self option:Spin title:@"Вращение персонажа" x:0 y:163 width:col parent:p];
    [self slider:@"spinSpeed" title:@"Скорость вращения (°/с)" min:45 max:720 x:0 y:198 width:col parent:p];
    UILabel* note=[self label:@"Скорость и боезапас меняются у живого локального игрока. Сервер может исправлять эти значения. При выключении модуль восстанавливает сохранённые настройки скорости, разброса, отдачи и FOV." frame:CGRectMake(0,252,w,100) size:12 bold:NO parent:p];note.numberOfLines=0;return p;
}
- (UIView*)makeSkins{
    UIView* p=[[UIView alloc]initWithFrame:_content.bounds];CGFloat w=p.bounds.size.width,h=p.bounds.size.height;
    _allWeapons=FlameCatalog()[@"weapons"];_weapons=_allWeapons;
    _search=[[UITextField alloc]initWithFrame:CGRectMake(0,0,w*.55,29)];_search.placeholder=@"Поиск оружия / скина";_search.borderStyle=UITextBorderStyleRoundedRect;_search.font=[UIFont systemFontOfSize:12];_search.clearButtonMode=UITextFieldViewModeWhileEditing;_search.delegate=self;[_search addTarget:self action:@selector(searchChanged) forControlEvents:UIControlEventEditingChanged];[p addSubview:_search];
    _weaponLabel=[self label:@"В руках: неизвестно" frame:CGRectMake(w*.57,0,w*.43,29) size:10 bold:YES parent:p];_weaponLabel.numberOfLines=2;
    _categories=[[UISegmentedControl alloc]initWithItems:@[@"Все",@"Ножи",@"Пист.",@"Винт.",@"Снайп.",@"Другое",@"Наклейки"]];_categories.frame=CGRectMake(0,35,w,27);_categories.selectedSegmentIndex=0;[_categories addTarget:self action:@selector(searchChanged) forControlEvents:UIControlEventValueChanged];[p addSubview:_categories];
    CGFloat tableH=fmax(70,h-109),left=w*.27,gridWidth=w-left-7;
    _weaponTable=[[UITableView alloc]initWithFrame:CGRectMake(0,68,left,tableH) style:UITableViewStylePlain];
    _weaponTable.dataSource=self;_weaponTable.delegate=self;_weaponTable.rowHeight=30;
    _weaponTable.backgroundColor=[UIColor colorWithWhite:1 alpha:.27];
    [p addSubview:_weaponTable];
    UICollectionViewFlowLayout* layout=[UICollectionViewFlowLayout new];
    layout.minimumInteritemSpacing=6;layout.minimumLineSpacing=6;
    layout.sectionInset=UIEdgeInsetsMake(1,1,1,1);
    NSInteger columns=gridWidth>=340?4:3;
    layout.itemSize=CGSizeMake(floor((gridWidth-2-6*(columns-1))/columns),79);
    _skinGrid=[[UICollectionView alloc]initWithFrame:CGRectMake(left+7,68,gridWidth,tableH) collectionViewLayout:layout];
    _skinGrid.dataSource=self;_skinGrid.delegate=self;
    _skinGrid.backgroundColor=UIColor.clearColor;
    [_skinGrid registerClass:FlameSkinCard.class forCellWithReuseIdentifier:@"skin"];
    [p addSubview:_skinGrid];
    _skinEmpty=[self label:@"Нет скинов по этому фильтру" frame:_skinGrid.frame size:11 bold:NO parent:p];
    _skinEmpty.textAlignment=NSTextAlignmentCenter;_skinEmpty.userInteractionEnabled=NO;
    NSArray* titles=@[@"Применить скин",@"Модель ножа",@"Сброс скина",@"Сброс ножа"];
    for(NSInteger i=0;i<4;i++){
        CGFloat bw=(w-18)/4;
        UIButton* b=[self button:titles[i] frame:CGRectMake(i*(bw+6),h-34,bw,32) target:@selector(skinAction:) tag:300+i parent:p];
        if(i==0)_applySkin=b;
        if(i==1)_applyModel=b;
    }
    _stickerSlot=[[UISegmentedControl alloc]initWithItems:@[@"1",@"2",@"3",@"4",@"5"]];
    _stickerSlot.frame=CGRectMake((w+6)/2,h-34,(w-6)/2,32);
    _stickerSlot.selectedSegmentIndex=0;_stickerSlot.hidden=YES;
    _stickerSlot.accessibilityLabel=@"Слот наклейки";[p addSubview:_stickerSlot];
    [self filterSkins];return p;
}
- (UIView*)makeConfig{
    UIScrollView* p=[self scrollPage];CGFloat w=p.bounds.size.width;p.contentSize=CGSizeMake(w,300);
    NSArray* names=@[@"Сохранить настройки",@"Загрузить настройки",@"Сбросить настройки",@"Выключить и восстановить"];
    for(NSInteger i=0;i<4;i++)[self button:names[i] frame:CGRectMake(0,i*43,w*.6,35) target:@selector(configAction:) tag:400+i parent:p];
    UILabel* note=[self label:@"Открыть/закрыть: два касания тремя пальцами.\nМеню можно перетаскивать за заголовок.\nСохранение не включает модуль при запуске." frame:CGRectMake(0,180,w,65) size:12 bold:NO parent:p];note.numberOfLines=0;
    UISegmentedControl* fps=[[UISegmentedControl alloc]initWithItems:@[@"30 FPS",@"60 FPS"]];_fpsSegments=fps;fps.frame=CGRectMake(0,255,w*.6,30);fps.selectedSegmentIndex=[_config[@"fps"] intValue]==30?0:1;[fps addTarget:self action:@selector(changeFPS:) forControlEvents:UIControlEventValueChanged];[p addSubview:fps];return p;
}
- (void)showTab:(NSInteger)index{
    _activeTab=index;NSArray* titles=@[@"Visual / ESP",@"Aim / Weapon",@"Misc",@"SkinChanger",@"Config"];
    _pageTitle.text=titles[index];for(NSInteger i=0;i<_pages.count;i++){_pages[i].hidden=i!=index;_tabs[i].backgroundColor=i==index?[UIColor colorWithRed:.22 green:.29 blue:.29 alpha:1]:[UIColor colorWithRed:.39 green:.42 blue:.41 alpha:1];}
    [self endEditing:YES];
}
- (void)selectTab:(UIButton*)b{[self showTab:b.tag-100];}
- (void)closeMenu{[self endEditing:YES];self.hidden=YES;}
- (void)drag:(UIPanGestureRecognizer*)pan{
    if(pan.state==UIGestureRecognizerStateBegan)_dragOrigin=self.center;CGPoint delta=[pan translationInView:self.superview];CGPoint next=CGPointMake(_dragOrigin.x+delta.x,_dragOrigin.y+delta.y);
    next.x=fmax(self.bounds.size.width/2,fmin(self.superview.bounds.size.width-self.bounds.size.width/2,next.x));next.y=fmax(self.bounds.size.height/2,fmin(self.superview.bounds.size.height-self.bounds.size.height/2,next.y));self.center=next;
}
- (void)changeOption:(UISwitch*)s{_config[@"options"][FlameOptionKey(s.tag)]=@(s.on);[self.delegate flameSetOption:s.tag value:s.on];[self refreshPreview];}
- (void)toggleRow:(UIButton*)row{UISwitch* s=_switches[@(row.tag-1000)];[s setOn:!s.on animated:YES];[self changeOption:s];}
- (void)changeSlider:(UISlider*)s{NSString* key=_sliderKeys[@(s.tag)];_config[key]=@(s.value);[self refreshValue:key];[self.delegate flameSetNumber:key value:s.value];}
- (void)changeAimPoint:(UISegmentedControl*)s{
    _config[@"aimPoint"]=@(s.selectedSegmentIndex);
    [self.delegate flameSetNumber:@"aimPoint" value:s.selectedSegmentIndex];
}
- (void)changeColor:(UISegmentedControl*)s{_config[@"accent"]=@(s.selectedSegmentIndex);[self.delegate flameSetNumber:@"accent" value:s.selectedSegmentIndex];[self refreshPreview];}
- (void)changeFPS:(UISegmentedControl*)s{double value=s.selectedSegmentIndex==0?30:60;_config[@"fps"]=@(value);[self.delegate flameSetNumber:@"fps" value:value];}
- (void)refreshPreview{
    _preview.boxes=[_config[@"options"][FlameOptionKey(Boxes)] boolValue];_preview.skeleton=[_config[@"options"][FlameOptionKey(Skeleton)] boolValue];_preview.corners=[_config[@"options"][FlameOptionKey(CornerBox)] boolValue];_preview.filled=[_config[@"options"][FlameOptionKey(FilledBox)] boolValue];_preview.health=[_config[@"options"][FlameOptionKey(Health)] boolValue];_preview.head=[_config[@"options"][FlameOptionKey(HeadCircle)] boolValue];_preview.accent=[_config[@"accent"] integerValue];[_preview setNeedsDisplay];
}
- (void)setRuntimeOption:(NSInteger)tag value:(BOOL)value{_switches[@(tag)].on=value;_config[@"options"][FlameOptionKey(tag)]=@(value);[self refreshPreview];}
- (void)sync:(NSDictionary*)config{
    _config=[config mutableCopy];_config[@"options"]=[config[@"options"] mutableCopy];for(NSNumber* tag in _switches)[self setRuntimeOption:tag.integerValue value:[config[@"options"][FlameOptionKey(tag.integerValue)] boolValue]];
    _aimPoints.selectedSegmentIndex=[config[@"aimPoint"] integerValue];
    for(NSString* key in _sliders){_sliders[key].value=[config[key] floatValue];[self refreshValue:key];}_colorSegments.selectedSegmentIndex=[config[@"accent"] integerValue];_fpsSegments.selectedSegmentIndex=[config[@"fps"] intValue]==30?0:1;[self refreshPreview];
}
- (BOOL)weapon:(NSDictionary*)weapon matchesQuery:(NSString*)query {
    return !query.length||[[weapon[@"label"] lowercaseString] containsString:query]||[[weapon[@"name"] lowercaseString] containsString:query];
}
- (BOOL)skin:(NSDictionary*)skin matchesQuery:(NSString*)query {
    return [[skin[@"label"] lowercaseString] containsString:query]||[[skin[@"key"] lowercaseString] containsString:query];
}
- (void)searchChanged{
    NSString* query=[_search.text lowercaseString]?:@"";
    NSInteger category=_categories.selectedSegmentIndex;
    if(category==6){_weaponFilter=nil;_selectedSkin=nil;_selectedWeapon=nil;_weapons=@[];[_weaponTable reloadData];[self filterSkins];return;}
    _weapons=[_allWeapons filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDictionary* w,NSDictionary*){
        NSString* c=w[@"category"];
        BOOL match=category==0||(category==1&&[c isEqual:@"knives"])||(category==2&&[c isEqual:@"pistols"])||(category==3&&[c isEqual:@"rifles"])||(category==4&&[c isEqual:@"snipers"])||(category==5&&![c isEqual:@"knives"]&&![c isEqual:@"pistols"]&&![c isEqual:@"rifles"]&&![c isEqual:@"snipers"]);
        if(!match)return NO;
        if([self weapon:w matchesQuery:query])return YES;
        for(NSDictionary* skin in w[@"skins"])if([self skin:skin matchesQuery:query])return YES;
        return NO;
    }]];
    if(![_weapons containsObject:_weaponFilter])_weaponFilter=nil;
    [_weaponTable reloadData];[self filterSkins];
}
- (void)filterSkins{
    NSString* query=[_search.text lowercaseString]?:@"";
    BOOL stickers=_categories.selectedSegmentIndex==6;
    _stickerSlot.hidden=!stickers;
    for(NSInteger i=1;i<4;i++)[[_stickerSlot.superview viewWithTag:300+i]setHidden:stickers];
    [_applySkin setTitle:stickers?@"Наклеить":@"Применить скин" forState:UIControlStateNormal];
    NSArray* weapons=_weaponFilter?@[_weaponFilter]:_weapons;
    NSMutableArray* cards=[NSMutableArray array];
    BOOL selectionVisible=NO;
    for(NSDictionary* weapon in weapons){
        BOOL weaponMatch=[self weapon:weapon matchesQuery:query];
        for(NSDictionary* skin in weapon[@"skins"]){
            if(!weaponMatch&&![self skin:skin matchesQuery:query])continue;
            [cards addObject:@{@"weapon":weapon,@"skin":skin}];
            if(skin==_selectedSkin)selectionVisible=YES;
        }
    }
    if(stickers)for(NSDictionary* item in FlameStickerCatalog()){
        if(!query.length||[[item[@"label"] lowercaseString] containsString:query]||[[item[@"id"] stringValue] containsString:query]){
            [cards addObject:@{@"skin":item}];
            if(item==_selectedSkin)selectionVisible=YES;
        }
    }
    _skinCards=cards;
    if(!selectionVisible){_selectedSkin=nil;_selectedWeapon=_weaponFilter;}
    _skinEmpty.hidden=cards.count>0;
    [_skinGrid reloadData];[_skinGrid setContentOffset:CGPointZero animated:NO];
    [self refreshActions];
}
- (void)refreshActions{
    BOOL currentKnife=NO;
    for(NSDictionary* w in _allWeapons)if([w[@"id"] integerValue]==_currentWeapon){
        currentKnife=[w[@"category"] isEqual:@"knives"];break;
    }
    BOOL selectedKnife=[_selectedWeapon[@"category"] isEqual:@"knives"];
    BOOL sameWeapon=_selectedWeapon&&_currentWeapon==[_selectedWeapon[@"id"] integerValue];
    _applySkin.enabled=_selectedSkin&&(_categories.selectedSegmentIndex==6?(_currentWeapon>=0&&!currentKnife):(sameWeapon||(currentKnife&&selectedKnife)));
    _applySkin.alpha=_applySkin.enabled?1:.4;
    _applyModel.enabled=currentKnife&&selectedKnife;
    _applyModel.alpha=_applyModel.enabled?1:.4;
}
- (void)updateWeapon:(NSInteger)weapon name:(NSString*)name skin:(NSString*)skin{
    _currentWeapon=weapon;
    _weaponLabel.text=[NSString stringWithFormat:@"В руках: %@ · ID %ld\n%@",name.length?name:@"—",(long)weapon,skin.length?skin:@"стандартный материал"];[self refreshActions];
}
- (NSInteger)tableView:(UITableView*)table numberOfRowsInSection:(NSInteger)section{return _weapons.count+1;}
- (UITableViewCell*)tableView:(UITableView*)table cellForRowAtIndexPath:(NSIndexPath*)index{
    UITableViewCell* cell=[table dequeueReusableCellWithIdentifier:@"weapon"];
    if(!cell)cell=[[UITableViewCell alloc]initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"weapon"];
    NSDictionary* weapon=index.row?_weapons[index.row-1]:nil;
    cell.textLabel.text=weapon?weapon[@"label"]:@"Все скины";
    cell.textLabel.font=[UIFont systemFontOfSize:11 weight:UIFontWeightSemibold];
    cell.textLabel.textColor=[self textColor];cell.backgroundColor=UIColor.clearColor;
    cell.accessoryType=weapon==_weaponFilter?UITableViewCellAccessoryCheckmark:UITableViewCellAccessoryNone;
    return cell;
}
- (void)tableView:(UITableView*)table didSelectRowAtIndexPath:(NSIndexPath*)index{
    _weaponFilter=index.row?_weapons[index.row-1]:nil;
    _selectedWeapon=_weaponFilter;_selectedSkin=nil;
    [_weaponTable reloadData];[self filterSkins];
}
- (NSInteger)collectionView:(UICollectionView*)view numberOfItemsInSection:(NSInteger)section{return _skinCards.count;}
- (__kindof UICollectionViewCell*)collectionView:(UICollectionView*)view cellForItemAtIndexPath:(NSIndexPath*)index{
    FlameSkinCard* card=[view dequeueReusableCellWithReuseIdentifier:@"skin" forIndexPath:index];
    NSDictionary* entry=_skinCards[index.item];
    NSDictionary* skin=entry[@"skin"];
    UIImage* artwork=FlameSkinImage(skin[@"key"]);
    card.artwork.image=artwork?:[UIImage systemImageNamed:_categories.selectedSegmentIndex==6?@"seal":@"photo"];
    card.caption.text=skin[@"label"];
    card.accessibilityLabel=[NSString stringWithFormat:@"%@, %@%@",entry[@"weapon"][@"label"]?:@"Наклейка",skin[@"label"],(artwork||_categories.selectedSegmentIndex==6)?@"":@", изображение недоступно"];
    card.isAccessibilityElement=YES;
    card.layer.borderColor=(skin==_selectedSkin?FlameAccent(1):[UIColor colorWithWhite:.38 alpha:1]).CGColor;
    return card;
}
- (void)collectionView:(UICollectionView*)view didSelectItemAtIndexPath:(NSIndexPath*)index{
    NSDictionary* entry=_skinCards[index.item];
    _selectedWeapon=entry[@"weapon"];_selectedSkin=entry[@"skin"];
    [view reloadItemsAtIndexPaths:view.indexPathsForVisibleItems];[self refreshActions];
}
- (void)skinAction:(UIButton*)b{
    NSArray* actions=@[@"skin",@"model",@"restoreSkin",@"restoreModel"];
    [self endEditing:YES];
    if(_categories.selectedSegmentIndex==6){
        if(_selectedSkin){
            NSMutableDictionary* sticker=[_selectedSkin mutableCopy];
            sticker[@"slot"]=@(_stickerSlot.selectedSegmentIndex);
            [self.delegate flameSkinAction:@"sticker" weapon:nil skin:sticker];
        }
        return;
    }
    [self.delegate flameSkinAction:actions[b.tag-300] weapon:_selectedWeapon skin:_selectedSkin];
}
- (void)configAction:(UIButton*)b{NSArray* actions=@[@"save",@"load",@"reset",@"stop"];[self.delegate flameConfigAction:actions[b.tag-400]];}
- (BOOL)textFieldShouldReturn:(UITextField*)textField{[textField resignFirstResponder];return YES;}
@end
