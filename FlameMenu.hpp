#pragma once
#include "FlameOptions.hpp"
#include "FlameCatalog.hpp"
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
@interface FlameMenu:UIView<UITableViewDataSource,UITableViewDelegate,UITextFieldDelegate,UIGestureRecognizerDelegate>
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
    UITableView* _weaponTable;UITableView* _skinTable;UITextField* _search;UISegmentedControl* _categories;
    NSArray* _allWeapons;NSArray* _weapons;NSArray* _skins;NSDictionary* _selectedWeapon;NSDictionary* _selectedSkin;
    UIButton* _applySkin;UIButton* _applyModel;NSInteger _currentWeapon,_activeTab;
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
    [self label:title frame:CGRectMake(x,y,width-44,28) size:12 bold:NO parent:parent];UISwitch* toggle=[[UISwitch alloc]initWithFrame:CGRectMake(x+width-49,y,51,31)];toggle.transform=CGAffineTransformMakeScale(.72,.72);toggle.tag=tag;
    toggle.on=[_config[@"options"][FlameOptionKey(tag)] boolValue];toggle.onTintColor=FlameAccent(0);[toggle addTarget:self action:@selector(changeOption:) forControlEvents:UIControlEventValueChanged];[parent addSubview:toggle];_switches[@(tag)]=toggle;return toggle;
}
- (void)slider:(NSString*)key title:(NSString*)title min:(float)min max:(float)max x:(CGFloat)x y:(CGFloat)y width:(CGFloat)width parent:(UIView*)parent{
    [self label:title frame:CGRectMake(x,y,width-62,20) size:12 bold:NO parent:parent];UILabel* value=[self label:@"" frame:CGRectMake(x+width-70,y,70,20) size:11 bold:YES parent:parent];value.textAlignment=NSTextAlignmentRight;_values[key]=value;
    UISlider* slider=[[UISlider alloc]initWithFrame:CGRectMake(x,y+20,width,27)];slider.minimumValue=min;slider.maximumValue=max;slider.value=[_config[key] floatValue];slider.minimumTrackTintColor=FlameAccent(1);slider.maximumTrackTintColor=[UIColor colorWithWhite:.37 alpha:1];slider.tag=500+_sliderKeys.count;_sliderKeys[@(slider.tag)]=key;_sliders[key]=slider;
    [slider addTarget:self action:@selector(changeSlider:) forControlEvents:UIControlEventValueChanged];[parent addSubview:slider];[self refreshValue:key];
}
- (void)refreshValue:(NSString*)key{
    double n=[_config[key] doubleValue];_values[key].text=[key isEqualToString:@"maxDistance"]&&n<1?@"∞":[key isEqualToString:@"speedFactor"]?[NSString stringWithFormat:@"× %.2f",n]:[NSString stringWithFormat:@"%.1f",n];
}
- (instancetype)initWithFrame:(CGRect)frame config:(NSDictionary*)config{
    if((self=[super initWithFrame:frame])){
        _config=[config mutableCopy];_config[@"options"]=[config[@"options"] mutableCopy];_currentWeapon=-1;
        _switches=[NSMutableDictionary dictionary];_sliderKeys=[NSMutableDictionary dictionary];_sliders=[NSMutableDictionary dictionary];_values=[NSMutableDictionary dictionary];_tabs=[NSMutableArray array];
        self.backgroundColor=[UIColor colorWithRed:.96 green:.93 blue:.85 alpha:.97];self.layer.cornerRadius=7;self.layer.borderWidth=1;self.layer.borderColor=[UIColor colorWithWhite:.35 alpha:.45].CGColor;self.clipsToBounds=YES;
        CGFloat side=108;_sidebar=[[UIView alloc]initWithFrame:CGRectMake(0,0,side,frame.size.height)];_sidebar.autoresizingMask=UIViewAutoresizingFlexibleHeight;_sidebar.backgroundColor=[UIColor colorWithRed:.58 green:.6 blue:.57 alpha:1];[self addSubview:_sidebar];
        UILabel* brand=[self label:@"◉ FLAME" frame:CGRectMake(10,11,98,29) size:19 bold:YES parent:_sidebar];brand.textColor=[UIColor colorWithRed:1 green:.85 blue:.27 alpha:1];
        [self label:@"iOS · v7" frame:CGRectMake(15,42,88,18) size:11 bold:YES parent:_sidebar];
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
    UIScrollView* p=[self scrollPage];CGFloat w=p.bounds.size.width,col=(w-20)/2;p.contentSize=CGSizeMake(w,430);
    [self section:@"ESP" x:0 y:0 width:col parent:p];NSArray* rows=@[@[@(Enemies),@"Только противники"],@[@(Boxes),@"Рамки"],@[@(CornerBox),@"Угловые рамки"],@[@(FilledBox),@"Заливка"],@[@(Skeleton),@"Скелет"],@[@(HeadCircle),@"Круг головы"],@[@(Health),@"Здоровье"],@[@(Names),@"Имена"],@[@(Distance),@"Дистанция"],@[@(Tracers),@"Линии к игрокам"]];
    for(NSInteger i=0;i<rows.count;i++)[self option:[rows[i][0] integerValue] title:rows[i][1] x:0 y:30+i*30 width:col parent:p];
    [self slider:@"lineWidth" title:@"Толщина линий" min:.6 max:3 x:0 y:340 width:col parent:p];
    [self section:@"ESP Preview" x:col+20 y:0 width:col parent:p];_preview=[[FlameMiniPreview alloc]initWithFrame:CGRectMake(col+20,30,col,151)];_preview.backgroundColor=[UIColor colorWithWhite:1 alpha:.3];_preview.layer.borderWidth=.5;_preview.layer.borderColor=[UIColor colorWithWhite:.7 alpha:1].CGColor;[p addSubview:_preview];
    [self option:Crosshair title:@"Прицел" x:col+20 y:190 width:col parent:p];[self option:FPSCounter title:@"Счётчик FPS" x:col+20 y:220 width:col parent:p];[self option:Debug title:@"Диагностика" x:col+20 y:250 width:col parent:p];
    [self slider:@"maxDistance" title:@"Макс. дистанция (u)" min:0 max:250 x:col+20 y:285 width:col parent:p];
    [self label:@"Цвет противников" frame:CGRectMake(col+20,345,col,20) size:12 bold:NO parent:p];UISegmentedControl* colors=[[UISegmentedControl alloc]initWithItems:@[@"Cyan",@"Gold",@"Green",@"Red"]];colors.frame=CGRectMake(col+20,372,col,28);colors.selectedSegmentIndex=[_config[@"accent"] integerValue];[colors addTarget:self action:@selector(changeColor:) forControlEvents:UIControlEventValueChanged];[p addSubview:colors];return p;
}
- (UIView*)makeAim{
    UIScrollView* p=[self scrollPage];CGFloat w=p.bounds.size.width,col=(w-22)/2;p.contentSize=CGSizeMake(w,300);
    [self section:@"AIM / Weapon" x:0 y:0 width:col parent:p];[self option:Aim title:@"AIM при огне · тест" x:0 y:32 width:col parent:p];[self option:Recoil title:@"No Recoil · тест" x:0 y:65 width:col parent:p];[self option:NoSpread title:@"No Spread · клиент" x:0 y:98 width:col parent:p];
    [self slider:@"aimRadius" title:@"Радиус наведения" min:30 max:180 x:col+22 y:0 width:col parent:p];[self slider:@"aimSmoothing" title:@"Скорость наведения" min:3 max:25 x:col+22 y:65 width:col parent:p];
    UILabel* note=[self label:@"AIM поворачивает камеру при удержании огня. Проверки видимости нет. Silent Aim не подключён.\n\nNo Spread меняет найденные локальные параметры; результат сервера отдельно проверяется в матче." frame:CGRectMake(0,155,w,130) size:12 bold:NO parent:p];note.numberOfLines=0;return p;
}
- (UIView*)makeMisc{
    UIScrollView* p=[self scrollPage];CGFloat w=p.bounds.size.width,col=(w-22)/2;p.contentSize=CGSizeMake(w,300);
    [self section:@"Movement" x:0 y:0 width:col parent:p];[self option:Speed title:@"SpeedHack · клиент" x:0 y:32 width:col parent:p];[self slider:@"speedFactor" title:@"Множитель скорости" min:1 max:3 x:0 y:75 width:col parent:p];
    [self section:@"Ammo / Camera" x:col+22 y:0 width:col parent:p];[self option:Ammo title:@"Unlimited Ammo · клиент" x:col+22 y:32 width:col parent:p];[self option:Fov title:@"Фиксированный FOV" x:col+22 y:65 width:col parent:p];[self slider:@"fov" title:@"Угол обзора" min:60 max:120 x:col+22 y:105 width:col parent:p];
    UILabel* note=[self label:@"Скорость и боезапас меняются у живого локального игрока. Сервер может исправлять эти значения. При выключении модуль восстанавливает сохранённые настройки скорости, разброса, отдачи и FOV." frame:CGRectMake(0,180,w,100) size:12 bold:NO parent:p];note.numberOfLines=0;return p;
}
- (UIView*)makeSkins{
    UIView* p=[[UIView alloc]initWithFrame:_content.bounds];CGFloat w=p.bounds.size.width,h=p.bounds.size.height;
    _allWeapons=FlameCatalog()[@"weapons"];_weapons=_allWeapons;
    _search=[[UITextField alloc]initWithFrame:CGRectMake(0,0,w*.55,29)];_search.placeholder=@"Поиск оружия / скина";_search.borderStyle=UITextBorderStyleRoundedRect;_search.font=[UIFont systemFontOfSize:12];_search.clearButtonMode=UITextFieldViewModeWhileEditing;_search.delegate=self;[_search addTarget:self action:@selector(searchChanged) forControlEvents:UIControlEventEditingChanged];[p addSubview:_search];
    _weaponLabel=[self label:@"В руках: неизвестно" frame:CGRectMake(w*.57,0,w*.43,29) size:10 bold:YES parent:p];_weaponLabel.numberOfLines=2;
    _categories=[[UISegmentedControl alloc]initWithItems:@[@"Все",@"Ножи",@"Пист.",@"Винт.",@"Снайп.",@"Другое"]];_categories.frame=CGRectMake(0,35,w,27);_categories.selectedSegmentIndex=0;[_categories addTarget:self action:@selector(searchChanged) forControlEvents:UIControlEventValueChanged];[p addSubview:_categories];
    CGFloat tableH=fmax(70,h-109);_weaponTable=[[UITableView alloc]initWithFrame:CGRectMake(0,68,w*.44,tableH) style:UITableViewStylePlain];_skinTable=[[UITableView alloc]initWithFrame:CGRectMake(w*.46,68,w*.54,tableH) style:UITableViewStylePlain];
    for(UITableView* t in @[_weaponTable,_skinTable]){t.dataSource=self;t.delegate=self;t.rowHeight=31;t.backgroundColor=[UIColor colorWithWhite:1 alpha:.28];t.layer.borderWidth=.5;t.layer.borderColor=[UIColor colorWithWhite:.7 alpha:1].CGColor;[p addSubview:t];}
    NSArray* titles=@[@"Применить скин",@"Модель ножа",@"Скин ↶",@"Нож ↶"];
    for(NSInteger i=0;i<4;i++){CGFloat bw=(w-18)/4;UIButton* b=[self button:titles[i] frame:CGRectMake(i*(bw+6),h-34,bw,32) target:@selector(skinAction:) tag:300+i parent:p];if(i==0)_applySkin=b;if(i==1)_applyModel=b;}
    _selectedWeapon=_allWeapons.firstObject;[self filterSkins];return p;
}
- (UIView*)makeConfig{
    UIScrollView* p=[self scrollPage];CGFloat w=p.bounds.size.width;p.contentSize=CGSizeMake(w,300);
    NSArray* names=@[@"Сохранить настройки",@"Загрузить настройки",@"Сбросить настройки",@"Выключить и восстановить"];
    for(NSInteger i=0;i<4;i++)[self button:names[i] frame:CGRectMake(0,i*43,w*.6,35) target:@selector(configAction:) tag:400+i parent:p];
    UILabel* note=[self label:@"Открыть/закрыть: два касания тремя пальцами.\nМеню можно перетаскивать за заголовок.\nСохранение не включает модуль при запуске." frame:CGRectMake(0,180,w,65) size:12 bold:NO parent:p];note.numberOfLines=0;
    UISegmentedControl* fps=[[UISegmentedControl alloc]initWithItems:@[@"30 FPS",@"60 FPS"]];fps.frame=CGRectMake(0,255,w*.6,30);fps.selectedSegmentIndex=[_config[@"fps"] intValue]==30?0:1;[fps addTarget:self action:@selector(changeFPS:) forControlEvents:UIControlEventValueChanged];[p addSubview:fps];return p;
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
- (void)changeColor:(UISegmentedControl*)s{_config[@"accent"]=@(s.selectedSegmentIndex);[self.delegate flameSetNumber:@"accent" value:s.selectedSegmentIndex];[self refreshPreview];}
- (void)changeFPS:(UISegmentedControl*)s{double value=s.selectedSegmentIndex==0?30:60;_config[@"fps"]=@(value);[self.delegate flameSetNumber:@"fps" value:value];}
- (void)refreshPreview{
    _preview.boxes=[_config[@"options"][FlameOptionKey(Boxes)] boolValue];_preview.skeleton=[_config[@"options"][FlameOptionKey(Skeleton)] boolValue];_preview.corners=[_config[@"options"][FlameOptionKey(CornerBox)] boolValue];_preview.filled=[_config[@"options"][FlameOptionKey(FilledBox)] boolValue];_preview.health=[_config[@"options"][FlameOptionKey(Health)] boolValue];_preview.head=[_config[@"options"][FlameOptionKey(HeadCircle)] boolValue];_preview.accent=[_config[@"accent"] integerValue];[_preview setNeedsDisplay];
}
- (void)setRuntimeOption:(NSInteger)tag value:(BOOL)value{_switches[@(tag)].on=value;_config[@"options"][FlameOptionKey(tag)]=@(value);[self refreshPreview];}
- (void)sync:(NSDictionary*)config{
    _config=[config mutableCopy];_config[@"options"]=[config[@"options"] mutableCopy];for(NSNumber* tag in _switches)[self setRuntimeOption:tag.integerValue value:[config[@"options"][FlameOptionKey(tag.integerValue)] boolValue]];
    for(NSString* key in _sliders){_sliders[key].value=[config[key] floatValue];[self refreshValue:key];}[self refreshPreview];
}
- (void)searchChanged{
    NSString* query=[_search.text lowercaseString]?:@"";NSInteger category=_categories.selectedSegmentIndex;
    _weapons=[_allWeapons filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDictionary* w,NSDictionary*){
        NSString* c=w[@"category"];BOOL match=category==0||(category==1&&[c isEqual:@"knives"])||(category==2&&[c isEqual:@"pistols"])||(category==3&&[c isEqual:@"rifles"])||(category==4&&[c isEqual:@"snipers"])||(category==5&&![c isEqual:@"knives"]&&![c isEqual:@"pistols"]&&![c isEqual:@"rifles"]&&![c isEqual:@"snipers"]);
        if(!match)return NO;if(!query.length||[[w[@"label"] lowercaseString] containsString:query]||[[w[@"name"] lowercaseString] containsString:query])return YES;
        for(NSDictionary* s in w[@"skins"])if([[s[@"label"] lowercaseString] containsString:query])return YES;return NO;
    }]];
    if(![_weapons containsObject:_selectedWeapon]){_selectedWeapon=_weapons.firstObject;_selectedSkin=nil;}[_weaponTable reloadData];[self filterSkins];
}
- (void)filterSkins{
    NSString* query=[_search.text lowercaseString]?:@"";NSArray* list=_selectedWeapon[@"skins"]?:@[];
    BOOL weaponMatch=[[_selectedWeapon[@"label"] lowercaseString] containsString:query]||[[_selectedWeapon[@"name"] lowercaseString] containsString:query];
    _skins=!query.length||weaponMatch?list:[list filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(NSDictionary* s,NSDictionary*){return [[s[@"label"] lowercaseString] containsString:query]||[[s[@"key"] lowercaseString] containsString:query];}]];
    if(![_skins containsObject:_selectedSkin])_selectedSkin=nil;[_skinTable reloadData];[self refreshActions];
}
- (void)refreshActions{
    _applySkin.enabled=_selectedSkin&&_currentWeapon==[_selectedWeapon[@"id"] integerValue];_applySkin.alpha=_applySkin.enabled?1:.4;
    BOOL currentKnife=NO;for(NSDictionary* w in _allWeapons)if([w[@"id"] integerValue]==_currentWeapon){currentKnife=[w[@"category"] isEqual:@"knives"];break;}
    _applyModel.enabled=currentKnife&&[_selectedWeapon[@"category"] isEqual:@"knives"];_applyModel.alpha=_applyModel.enabled?1:.4;
}
- (void)updateWeapon:(NSInteger)weapon name:(NSString*)name skin:(NSString*)skin{
    if(_currentWeapon!=weapon){_currentWeapon=weapon;for(NSDictionary* w in _allWeapons)if([w[@"id"] integerValue]==weapon){_selectedWeapon=w;_selectedSkin=nil;[_weaponTable reloadData];[self filterSkins];break;}}
    _weaponLabel.text=[NSString stringWithFormat:@"В руках: %@ · ID %ld\n%@",name.length?name:@"—",(long)weapon,skin.length?skin:@"стандартный материал"];[self refreshActions];
}
- (NSInteger)tableView:(UITableView*)table numberOfRowsInSection:(NSInteger)section{return table==_weaponTable?_weapons.count:MAX((NSUInteger)1,_skins.count);}
- (UITableViewCell*)tableView:(UITableView*)table cellForRowAtIndexPath:(NSIndexPath*)index{
    UITableViewCell* cell=[table dequeueReusableCellWithIdentifier:@"row"];if(!cell)cell=[[UITableViewCell alloc]initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@"row"];
    cell.textLabel.font=[UIFont systemFontOfSize:11];cell.textLabel.textColor=[self textColor];cell.backgroundColor=UIColor.clearColor;cell.accessoryType=UITableViewCellAccessoryNone;
    if(table==_weaponTable){NSDictionary* w=_weapons[index.row];cell.textLabel.text=[NSString stringWithFormat:@"%@  [%@]",w[@"label"],w[@"id"]];if(w==_selectedWeapon)cell.accessoryType=UITableViewCellAccessoryCheckmark;}
    else if(_skins.count){NSDictionary* s=_skins[index.row];cell.textLabel.text=s[@"label"];if(s==_selectedSkin)cell.accessoryType=UITableViewCellAccessoryCheckmark;}
    else cell.textLabel.text=@"Нет материалов в каталоге";return cell;
}
- (void)tableView:(UITableView*)table didSelectRowAtIndexPath:(NSIndexPath*)index{
    if(table==_weaponTable){_selectedWeapon=_weapons[index.row];_selectedSkin=nil;[_weaponTable reloadData];[self filterSkins];}
    else if(_skins.count){_selectedSkin=_skins[index.row];[_skinTable reloadData];[self refreshActions];}
}
- (void)skinAction:(UIButton*)b{NSArray* actions=@[@"skin",@"model",@"restoreSkin",@"restoreModel"];[self endEditing:YES];[self.delegate flameSkinAction:actions[b.tag-300] weapon:_selectedWeapon skin:_selectedSkin];}
- (void)configAction:(UIButton*)b{NSArray* actions=@[@"save",@"load",@"reset",@"stop"];[self.delegate flameConfigAction:actions[b.tag-400]];}
- (BOOL)textFieldShouldReturn:(UITextField*)textField{[textField resignFirstResponder];return YES;}
@end
