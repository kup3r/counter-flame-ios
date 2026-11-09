#pragma once
enum Option {Master=1,Enemies,Boxes,Skeleton,Health,Names,Distance,Tracers,Debug,Aim,Recoil,NoSpread,Speed,Ammo,Reserved,Fov,Crosshair,CornerBox,FilledBox,HeadCircle,FPSCounter};
static NSString* FlameOptionKey(NSInteger tag){return [NSString stringWithFormat:@"%ld",(long)tag];}
static NSDictionary* FlameDefaultConfig(){return @{
    @"options":@{FlameOptionKey(Master):@NO,FlameOptionKey(Enemies):@YES,FlameOptionKey(Boxes):@YES,FlameOptionKey(Skeleton):@YES,FlameOptionKey(Health):@YES,FlameOptionKey(Names):@NO,FlameOptionKey(Distance):@YES,FlameOptionKey(Tracers):@NO,FlameOptionKey(Debug):@NO,FlameOptionKey(Aim):@NO,FlameOptionKey(Recoil):@NO,FlameOptionKey(NoSpread):@NO,FlameOptionKey(Speed):@NO,FlameOptionKey(Ammo):@NO,FlameOptionKey(Fov):@NO,FlameOptionKey(Crosshair):@NO,FlameOptionKey(CornerBox):@YES,FlameOptionKey(FilledBox):@NO,FlameOptionKey(HeadCircle):@NO,FlameOptionKey(FPSCounter):@YES},
    @"aimRadius":@100,@"aimSmoothing":@10,@"speedFactor":@1.5,@"fov":@90,@"maxDistance":@0,@"lineWidth":@1.2,@"accent":@0,@"fps":@60
};}

