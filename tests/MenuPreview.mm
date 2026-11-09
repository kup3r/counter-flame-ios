#import <UIKit/UIKit.h>
#include <cmath>
#include <cassert>
#include "../FlameMenu.hpp"
@interface PreviewController:UIViewController<FlameMenuDelegate>
@end
@implementation PreviewController
- (UIInterfaceOrientationMask)supportedInterfaceOrientations{return UIInterfaceOrientationMaskLandscape;}
- (BOOL)shouldAutorotate{return YES;}
- (void)viewDidLoad{[super viewDidLoad];self.view.backgroundColor=[UIColor colorWithRed:.37 green:.42 blue:.4 alpha:1];}
- (void)viewDidAppear:(BOOL)animated{
    [super viewDidAppear:animated];
    assert([NSPropertyListSerialization propertyList:FlameDefaultConfig() isValidForFormat:NSPropertyListBinaryFormat_v1_0]);
    CGRect bounds=self.view.bounds;CGFloat width=fmin(740,bounds.size.width-42),height=fmin(355,bounds.size.height-30);
    FlameMenu* menu=[[FlameMenu alloc]initWithFrame:CGRectMake((bounds.size.width-width)/2,(bounds.size.height-height)/2,width,height) config:FlameDefaultConfig()];menu.delegate=self;[self.view addSubview:menu];
    [menu updateWeapon:9 name:@"AK-47" skin:@""];[menu showTab:[NSUserDefaults.standardUserDefaults integerForKey:@"FlamePreviewTab"]];
    menu.statusLabel.text=@"Предпросмотр UIKit · данные каталога из IPA · игровые функции здесь не выполняются";
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,2*NSEC_PER_SEC),dispatch_get_main_queue(),^{
        UIGraphicsImageRenderer* renderer=[[UIGraphicsImageRenderer alloc]initWithBounds:self.view.bounds];UIImage* image=[renderer imageWithActions:^(UIGraphicsImageRendererContext* ctx){[self.view drawViewHierarchyInRect:self.view.bounds afterScreenUpdates:YES];}];
        [UIImagePNGRepresentation(image) writeToFile:[NSTemporaryDirectory() stringByAppendingPathComponent:@"Flame-menu.png"] atomically:YES];
    });
}
- (void)flameSetOption:(NSInteger)tag value:(BOOL)value{}
- (void)flameSetNumber:(NSString*)key value:(double)value{}
- (void)flameSkinAction:(NSString*)action weapon:(NSDictionary*)weapon skin:(NSDictionary*)skin{}
- (void)flameConfigAction:(NSString*)action{}
@end
@interface PreviewDelegate:UIResponder<UIApplicationDelegate>
@property(nonatomic,strong)UIWindow* window;
@end
@implementation PreviewDelegate
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)options{
    self.window=[[UIWindow alloc]initWithFrame:UIScreen.mainScreen.bounds];self.window.rootViewController=[[PreviewController alloc]init];[self.window makeKeyAndVisible];return YES;
}
@end
int main(int argc,char** argv){@autoreleasepool{return UIApplicationMain(argc,argv,nil,NSStringFromClass(PreviewDelegate.class));}}

