#pragma once
#include <algorithm>
#include <cstring>

static UIImage* FlameSkinImage(NSString* key){
    struct Artwork {const char* key;const char* data;};
    static const Artwork catalog[]={
#include "artwork/Skins-0.inc"
#include "artwork/Skins-1.inc"
#include "artwork/Skins-2.inc"
#include "artwork/Skins-3.inc"
#include "artwork/Skins-4.inc"
#include "artwork/Skins-5.inc"
#include "artwork/Skins-6.inc"
#include "artwork/Skins-7.inc"
    };
    static const Artwork gloves[]={
#include "artwork/Gloves.inc"
    };
    const char* lookup=key.UTF8String;
    if(!lookup)return nil;
    auto end=catalog+sizeof(catalog)/sizeof(*catalog);
    auto entry=std::lower_bound(catalog,end,lookup,[](const Artwork& item,const char* target){
        return strcmp(item.key,target)<0;
    });
    if(entry==end||strcmp(entry->key,lookup)){
        end=gloves+sizeof(gloves)/sizeof(*gloves);
        entry=std::lower_bound(gloves,end,lookup,[](const Artwork& item,const char* target){return strcmp(item.key,target)<0;});
        if(entry==end||strcmp(entry->key,lookup))return nil;
    }
    static NSCache<NSString*,UIImage*>* cache;
    static dispatch_once_t once;
    dispatch_once(&once,^{cache=[NSCache new];cache.countLimit=90;});
    UIImage* image=[cache objectForKey:key];
    if(!image){
        NSData* data=[[NSData alloc]initWithBase64EncodedString:@(entry->data) options:0];
        image=[UIImage imageWithData:data];
        if(image)[cache setObject:image forKey:key];
    }
    return image;
}
