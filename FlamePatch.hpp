#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cmath>
namespace FlamePatch {
struct Memory {
    virtual uint32_t retain(void*)=0;
    virtual void* resolve(uint32_t)=0;
    virtual void release(uint32_t)=0;
    virtual void readValue(void*,void*,void*,size_t)=0;
    virtual void writeValue(void*,void*,const void*,size_t)=0;
    virtual ~Memory()=default;
};
struct Record {
    uint32_t owner;void* field;int group;size_t size;
    std::array<uint8_t,8> before{},applied{};
};
class Store {
    Memory* memory=nullptr;
    std::vector<Record> records;
    Record* find(void* owner,void* field,int group){
        for(auto& r:records)if(r.field==field&&r.group==group&&memory->resolve(r.owner)==owner)return &r;
        return nullptr;
    }
public:
    void bind(Memory* value){memory=value;}
    bool set(void* owner,void* field,const void* value,size_t size,int group){
        if(!memory||!owner||!field||!value||size==0||size>8)return false;
        std::array<uint8_t,8> now{};memory->readValue(owner,field,now.data(),size);
        Record* r=find(owner,field,group);
        if(!r){records.push_back({memory->retain(owner),field,group,size,now,{}});r=&records.back();}
        else if(r->size!=size)return false;
        else if(memcmp(now.data(),r->applied.data(),size))r->before=now;
        memcpy(r->applied.data(),value,size);memory->writeValue(owner,field,value,size);return true;
    }
    bool scale(void* owner,void* field,float multiplier,int group){
        if(!memory||!owner||!field||!std::isfinite(multiplier)||multiplier<0.1f||multiplier>4)return false;
        float now=NAN;memory->readValue(owner,field,&now,4);if(!std::isfinite(now)||now<=0||now>1000)return false;
        Record* r=find(owner,field,group);float base=now;
        if(r){float applied;memcpy(&applied,r->applied.data(),4);if(now==applied)memcpy(&base,r->before.data(),4);}
        float value=base*multiplier;if(!std::isfinite(value)||value>3000)return false;
        return set(owner,field,&value,4,group);
    }
    void restore(int group=0){
        if(!memory)return;
        for(size_t i=0;i<records.size();){
            Record& r=records[i];if(group&&r.group!=group){i++;continue;}
            void* owner=memory->resolve(r.owner);
            if(owner){std::array<uint8_t,8> now{};memory->readValue(owner,r.field,now.data(),r.size);
                if(!memcmp(now.data(),r.applied.data(),r.size))memory->writeValue(owner,r.field,r.before.data(),r.size);}
            memory->release(r.owner);records.erase(records.begin()+i);
        }
    }
    size_t count()const{return records.size();}
};
}

