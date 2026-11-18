#include "bbhost_plugin.h"
#include "doll_script.h"
#include "gcn/container.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
namespace {
const BbHostApi* api=nullptr;
constexpr uint32_t kDream=0x15000000;
constexpr uint64_t kDataMan=0x593b130, kWorldMan=0x593e878, kApply=0x2013d00;
const char* names[]={"vitality","endurance","strength","skill","bloodtinge","arcane"};
struct Build { int64_t level=0,stat[6]={},echoes=0; };
Build saved; bool have_saved=false, ready=false; uint32_t last_block=0;
template<class T> bool read(uint64_t addr,T& val){return addr && api->read(addr,&val,sizeof(val))==0;}
bool read_build(Build& b){if(api->player_stat_get("level",&b.level)||api->player_stat_get("echoes",&b.echoes))return false;for(int i=0;i<6;i++)if(api->player_stat_get(names[i],&b.stat[i]))return false;return true;}
bool apply(const Build& b){alignas(16) uint32_t c[10]={};c[2]=(uint32_t)b.level;for(int i=0;i<6;i++)c[i+3]=(uint32_t)b.stat[i];BbCallRegs r={};r.arg[0]=(uint64_t)(uintptr_t)c;return api->call_guest(kApply,&r)==0;}
void heal(){uint64_t m=0,c=0,mods=0,hp=0;int32_t mx=0,cur=0;if(!read(kWorldMan,m)||!read(m+0x60,c)||!read(c+0x3b0,mods)||!read(mods+0x20,hp)||!read(hp+0xfc,mx)||mx<=0||!read(hp+0xf8,cur))return;std::memcpy((void*)(uintptr_t)(hp+0xf8),&mx,4);api->player_stat_set("hp",mx);}
bool reset(){
 Build cur,base;uint64_t m=0,rec=0;uint8_t origin=255;
 if(!read_build(cur)||!read(kDataMan,m)||!read(m+8,rec)||!read(rec+0xce,origin)||origin>8)return false;
 size_t n=0,gn=0;auto* row=(const uint8_t*)api->param_row("CharaInitParam",3000u+origin,&n);auto* graph=(const uint8_t*)api->param_row("CalcCorrectGraph",200,&gn);
 if(!row||n<0xc9||!graph||gn<0x4c)return false;
 int16_t lvl;std::memcpy(&lvl,row+0xc0,2);base.level=lvl;const size_t pos[]={0xc2,0xc3,0xc5,0xc6,0xc7,0xc8};int64_t gained=0;
 if(base.level<1||base.level>100||cur.level<1||cur.level>544||cur.echoes<0||cur.echoes>999999999)return false;
 for(int i=0;i<6;i++){base.stat[i]=row[pos[i]];if(base.stat[i]<1||base.stat[i]>99||cur.stat[i]<base.stat[i]||cur.stat[i]>99)return false;gained+=cur.stat[i]-base.stat[i];}
 float g[4];std::memcpy(g,graph+0x3c,sizeof g);for(float f:g)if(!std::isfinite(f))return false;
 int64_t refund=0;for(int64_t l=base.level;l<base.level+gained;l++){float x=(float)(l+81),k=x-g[3];if(k<0)k=0;k=k*g[2]+g[0];float v=x*x*k+g[1]+0.5f;if(!std::isfinite(v)||v<0||v>1000000000.0f)return false;refund+=(int64_t)v;if(refund>999999999-cur.echoes)return false;}
 base.echoes=cur.echoes+refund;if(!apply(base))return false;api->player_stat_set("echoes",base.echoes);heal();saved=cur;have_saved=true;api->log("doll-rebirth: level %lld to %lld, refund %lld",(long long)cur.level,(long long)base.level,(long long)refund);return true;
}
void frame(void*){
 uint32_t block=0;if(api->player_block(&block)!=0){ready=false;last_block=0;have_saved=false;return;}
 if(block!=kDream){ready=false;last_block=block;have_saved=false;return;}
 if(!ready||last_block!=block){last_block=block;int v=0;if(api->event_flag_get(doll::kReset,&v)||api->event_flag_get(doll::kUndo,&v)||api->event_flag_get(doll::kFail,&v))return;if(api->event_flag_set(doll::kReset,0)||api->event_flag_set(doll::kUndo,0)||api->event_flag_set(doll::kFail,0))return;have_saved=false;ready=true;}
 int v=0;if(!api->event_flag_get(doll::kReset,&v)&&v){bool ok=reset();api->event_flag_set(doll::kFail,!ok);api->event_flag_set(doll::kReset,0);if(!ok)api->log("doll-rebirth: reset refused; no stats changed");}
 if(!api->event_flag_get(doll::kUndo,&v)&&v){if(have_saved && apply(saved)){api->player_stat_set("echoes",saved.echoes);heal();have_saved=false;api->log("doll-rebirth: undone");}api->event_flag_set(doll::kUndo,0);}
}
bool ends(const std::vector<uint8_t>& v,const char* tail){std::string s;for(size_t i=0;i+1<v.size();i+=2){if(v[i+1])return false;s+=(char)v[i];}size_t n=std::strlen(tail);return s.size()>=n&&s.compare(s.size()-n,n,tail)==0;}
}
extern "C" {
extern BB_PLUGIN_EXPORT const BbPluginInfo bb_plugin_info={9,BB_PLUGIN_GAMEPLAY,"doll_rebirth","Doll Rebirth","1.0.0","Community","Respec at the Doll with a refund and undo."};
BB_PLUGIN_EXPORT int bb_plugin_image(const BbHostApi* a){
 if(!a||a->version<9||a->size<offsetof(BbHostApi,overlay_file)+sizeof(a->overlay_file)||!a->eboot_is_109())return 1;
 api=a; const unsigned char want[]={0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec};unsigned char have[sizeof want];if(api->read(kApply,have,sizeof have)||std::memcmp(want,have,sizeof want)){api->log("doll-rebirth: stat apply function does not match");return 1;}
 const char* path="/dvdroot_ps4/script/talk/m21_00_00_00.talkesdbnd.dcx";const void* p=nullptr;size_t n=0;if(api->game_file(path,&p,&n)||!p||!n)return 1;
 std::vector<uint8_t> bytes((const uint8_t*)p,(const uint8_t*)p+n);gcn::Bnd4Archive b;std::string why;if(!gcn::bnd4_read(bytes,b,&why)){api->log("doll-rebirth: archive invalid: %s",why.c_str());return 1;}
 int found=0;for(auto& f:b.files)if(ends(f.name,"t210304.esd")){++found;if(!doll::rewrite(f.data,&why)){api->log("doll-rebirth: %s",why.c_str());return 1;}}
 if(found!=1){api->log("doll-rebirth: expected one Doll script");return 1;}
 auto out=gcn::bnd4_pack(b);if(api->overlay_file("doll_rebirth",path,out.data(),out.size()))return 1;
 if(api->on_frame(frame,nullptr)){api->log("doll-rebirth: cannot register frame handler");return 1;}
 api->log("doll-rebirth: Doll menu enabled");return 0;
}
}
