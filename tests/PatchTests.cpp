#include "../FlamePatch.hpp"
#include <map>
#include <cassert>
struct Fake:FlamePatch::Memory {
    std::map<uint32_t,void*> roots;uint32_t next=1;
    uint32_t retain(void* p)override{roots[next]=p;return next++;}
    void* resolve(uint32_t h)override{return roots.at(h);}
    void release(uint32_t h)override{roots.erase(h);}
    void readValue(void*,void* f,void* out,size_t n)override{memcpy(out,f,n);}
    void writeValue(void*,void* f,const void* in,size_t n)override{memcpy(f,in,n);}
};
int main(){
    Fake memory;FlamePatch::Store store;store.bind(&memory);int owner=1;
    float recoil=4,speed=10,zero=0;
    assert(store.set(&owner,&recoil,&zero,4,1));assert(recoil==0);
    assert(store.scale(&owner,&speed,2,3));assert(speed==20);
    for(int i=0;i<100;i++)assert(store.scale(&owner,&speed,2,3));
    assert(speed==20); // Repeated frames must not compound a multiplier.
    assert(store.scale(&owner,&speed,1.5,3));assert(speed==15);
    store.restore(1);assert(recoil==4&&speed==15&&store.count()==1);
    speed=12;assert(store.scale(&owner,&speed,2,3));assert(speed==24);
    store.restore(3);assert(speed==12); // A fresh game-provided base supersedes the previous one.
    assert(store.set(&owner,&recoil,&zero,4,1));recoil=7;store.restore();assert(recoil==7);
    float v[2]={2,3},z[2]={0,0};assert(store.set(&owner,v,z,8,2));assert(v[0]==0&&v[1]==0);
    store.restore();assert(v[0]==2&&v[1]==3);assert(memory.roots.empty());
    assert(!store.scale(&owner,&speed,NAN,3));assert(!store.set(&owner,&recoil,&zero,16,1));
}

