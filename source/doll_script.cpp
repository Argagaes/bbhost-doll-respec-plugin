#include "doll_script.h"
#include "engine/esd.h"
#include "core/sha256.h"
#include <initializer_list>
namespace doll {
using Code=std::vector<std::uint8_t>;
static Code cat(std::initializer_list<Code> parts){Code x;for(auto&p:parts)x.insert(x.end(),p.begin(),p.end());return x;}
static Code eq(const Code& lhs,int rhs){return esd::done(cat({lhs,esd::lit(rhs),Code{0x95}}));}
static Code arg(int x){return esd::done(esd::lit(x));}
bool rewrite(std::vector<std::uint8_t>& b,std::string* why){
 auto fail=[&](const char* s){if(why)*why=s;return false;};
 if(sha256_hex(b.data(),b.size())!="53c4a470529f1a6d6455e6a8ca0dcb53a8412afccfe154623f337226552cf458")return fail("not the supported 1.09 Doll script");
 esd::Script s;if(!esd::parse(b,s,why))return false;
 for(auto spec:{std::initializer_list<int>{2147483641,5,2,7,1},std::initializer_list<int>{2147483634,6,3,9,2}}){
  auto p=spec.begin();int gid=*p++, listid=*p++, choiceid=*p++, levelid=*p++, restartid=*p++;
  auto* group=s.group(gid);if(!group)return fail("missing Doll menu group");
  int list=s.state_in(*group,listid), choice=s.state_in(*group,choiceid), level=s.state_in(*group,levelid), restart=s.state_in(*group,restartid);
  if(list<0||choice<0||level<0||restart<0)return fail("missing Doll menu state");
  auto cmd=[&](int id,std::initializer_list<int> vals){std::vector<Code>a;for(int v:vals)a.push_back(arg(v));return s.cmd(1,id,a);};
  int st[8]; for(int i=0;i<8;i++){st[i]=s.state(1000+i); group=s.group(gid);group->states.push_back(st[i]);}
  auto set=[&](int i,int cond,int entry){auto&x=s.states[st[i]];x.conds=cond;if(entry>=0)x.entry=entry;};
  auto listitems=s.runs[s.states[list].entry].items;if(listitems.empty())return fail("empty Doll menu");
  listitems.insert(listitems.end()-1,cmd(19,{4,kOption,-1}));s.states[list].entry=s.run(listitems);
  auto choices=s.runs[s.states[choice].conds].items;if(choices.empty())return fail("empty Doll choices");
  choices.insert(choices.begin()+1,s.cond(st[0],eq(Code{0xaf},4)));s.states[choice].conds=s.run(choices);
  const auto truth=esd::done(esd::lit(1));
  // Ask before resetting. f22 is the game's Yes response, f21 says the box is open.
  set(0,s.run({s.cond(st[1],eq(esd::call(22,{}),1)),s.cond(restart,eq(esd::call(21,{}),0))}),s.run({cmd(17,{2,kOption,3,4,2})}));
  set(1,s.run({s.cond(st[2],eq(esd::call(15,{kReset}),0))}),s.run({cmd(11,{kFail,0}),cmd(11,{kReset,1})}));
  set(2,s.run({s.cond(st[3],eq(esd::call(15,{kFail}),1)),s.cond(st[4],truth)}),-1);
  set(3,s.run({s.cond(restart,eq(esd::call(21,{}),0))}),s.run({cmd(17,{7,kFailText,1,0,1})}));
  const auto original=s.runs[s.states[level].conds].items;if(original.size()!=1)return fail("unexpected level-up close condition");
  auto close=s.conds[original[0]];
  set(4,s.run({s.cond(st[5],s.exprs[close.eval].code)}),s.run({cmd(31,{})}));
  set(5,s.run({s.cond(st[7],truth)}),s.run({cmd(20,{}),cmd(19,{1,kAccept,-1}),cmd(19,{2,kUndoText,-1})}));
  set(7,s.run({s.cond(restart,esd::done(cat({esd::lit(23),Code{0x84,0xa7},esd::lit(1),Code{0x95}}))),s.cond(st[6],eq(Code{0xaf},2)),s.cond(st[4],eq(esd::call(53,{}),0))}),s.run({cmd(76,{})}));
  set(6,s.run({s.cond(restart,eq(esd::call(15,{kUndo}),0))}),s.run({cmd(11,{kUndo,1})}));
 }
 auto result=esd::serialize(s);esd::Script check;if(!esd::parse(result,check,why))return false;b=std::move(result);return true;
}
}
