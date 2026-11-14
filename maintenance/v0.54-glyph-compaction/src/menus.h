#pragma once
#include "chinese_text.h"
#include "ui_state.h"
#include <deque>
struct MenuButton{int command;float x,y,w,h;std::wstring label;};
inline std::deque<int> menuCommands;
inline std::vector<MenuButton> clickableButtons;
inline bool inNativeMenu=false,nativeMenuJustClosed=false;
inline HMENU createChineseMenu(){
 HMENU root=CreateMenu(),game=CreatePopupMenu(),aircraft=CreatePopupMenu(),mission=CreatePopupMenu(),settings=CreatePopupMenu();
 AppendMenuW(game,MF_STRING,ui::Continue,L"开始 / 继续飞行");AppendMenuW(game,MF_STRING,ui::ToggleMenu,L"主菜单 / 暂停\tTab");
 AppendMenuW(game,MF_STRING,ui::Save,L"保存飞行\tF5");AppendMenuW(game,MF_STRING,ui::Load,L"读取飞行");AppendMenuW(game,MF_STRING,ui::Restart,L"重新出发\tR");
 AppendMenuW(game,MF_STRING,ui::Account,L"公开机库网站");
 AppendMenuW(game,MF_SEPARATOR,0,nullptr);AppendMenuW(game,MF_STRING,ui::Exit,L"保存并退出");
 AppendMenuW(aircraft,MF_STRING,ui::Trainer,L"轻型教练机");AppendMenuW(aircraft,MF_STRING,ui::Airliner,L"双发支线客机 · 70座");AppendMenuW(aircraft,MF_STRING,ui::A330,L"空客 A330-300 · 300座");AppendMenuW(aircraft,MF_STRING,ui::A350,L"空客 A350-900 · 330座");AppendMenuW(aircraft,MF_STRING,ui::A320,L"空客 A320 · 下载机模");AppendMenuW(aircraft,MF_STRING,ui::B737,L"波音 B737 · 下载机模");AppendMenuW(aircraft,MF_STRING,ui::B787,L"波音 B787 · 下载机模");AppendMenuW(aircraft,MF_STRING,ui::A319,L"空客 A319 · 近似机模");AppendMenuW(aircraft,MF_STRING,ui::A321,L"空客 A321 · 近似机模");AppendMenuW(aircraft,MF_STRING,ui::B7879,L"波音 B787-9 · 近似机模");AppendMenuW(aircraft,MF_STRING,ui::B777,L"波音 777-300ER · FlightGear");AppendMenuW(aircraft,MF_STRING,ui::ImportModel,L"导入自定义机模...");AppendMenuW(aircraft,MF_STRING,ui::ResetModel,L"恢复当前机型默认模型");AppendMenuW(aircraft,MF_STRING,ui::RotateModel,L"旋转当前机模90度");AppendMenuW(aircraft,MF_SEPARATOR,0,nullptr);
 AppendMenuW(aircraft,MF_STRING,ui::PassengersLess,L"减少10名乘客");AppendMenuW(aircraft,MF_STRING,ui::PassengersMore,L"增加10名乘客");AppendMenuW(aircraft,MF_STRING,ui::PassengersFull,L"乘客满载");AppendMenuW(aircraft,MF_STRING,ui::PassengersEmpty,L"清空乘客");
 const wchar_t* tasks[]={L"执照训练",L"货物运输",L"浦东—东海旅客航班",L"计时竞速",L"摄影巡游"};for(int i=0;i<5;i++)AppendMenuW(mission,MF_STRING,ui::License+i,tasks[i]);
 AppendMenuW(settings,MF_STRING,ui::PowerSaver,L"帧率限制：20帧 / 不限制");AppendMenuW(settings,MF_STRING,ui::Assist,L"滚转辅助回正\tL");AppendMenuW(settings,MF_SEPARATOR,0,nullptr);
 AppendMenuW(settings,MF_STRING,ui::CameraChase,L"追尾视角");AppendMenuW(settings,MF_STRING,ui::CameraCockpit,L"三维驾驶舱 · 鼠标操作");AppendMenuW(settings,MF_STRING,ui::CameraTop,L"俯视视角");
 AppendMenuW(settings,MF_STRING,ui::TerrainSatellite,L"上海浦东 · 离线卫星图");AppendMenuW(settings,MF_STRING,ui::TerrainMountains,L"山海地形 · 虚构环境");
 AppendMenuW(settings,MF_STRING,ui::SuperSample,L"切换 2×超采样 / 原生分辨率");
 AppendMenuW(settings,MF_STRING,ui::Fullscreen,L"全屏 / 窗口\tF11");
 AppendMenuW(settings,MF_STRING,ui::AntiAlias,L"切换 FXAA抗锯齿");
 AppendMenuW(settings,MF_STRING,ui::LightingPage,L"亮度与光影...");
 AppendMenuW(root,MF_POPUP,(UINT_PTR)game,L"游戏");AppendMenuW(root,MF_POPUP,(UINT_PTR)aircraft,L"飞机与乘客");AppendMenuW(root,MF_POPUP,(UINT_PTR)mission,L"任务");AppendMenuW(root,MF_POPUP,(UINT_PTR)settings,L"设置");AppendMenuW(root,MF_STRING,ui::Help,L"中文说明");return root;
}
inline void updateChineseMenu(HMENU menu,const sim::Flight& f,bool powerSaver){
 for(int command:{ui::A319,ui::A321,ui::B7879,ui::B777})CheckMenuItem(menu,command,MF_BYCOMMAND|(f.aircraftId==ui::aircraftIdForCommand(command)?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(menu,ui::Trainer,MF_BYCOMMAND|(f.aircraftId==0?MF_CHECKED:MF_UNCHECKED));CheckMenuItem(menu,ui::Airliner,MF_BYCOMMAND|(f.aircraftId==1?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(menu,ui::A330,MF_BYCOMMAND|(f.aircraftId==2?MF_CHECKED:MF_UNCHECKED));CheckMenuItem(menu,ui::A350,MF_BYCOMMAND|(f.aircraftId==3?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(menu,ui::A320,MF_BYCOMMAND|(f.aircraftId==4?MF_CHECKED:MF_UNCHECKED));CheckMenuItem(menu,ui::B737,MF_BYCOMMAND|(f.aircraftId==5?MF_CHECKED:MF_UNCHECKED));CheckMenuItem(menu,ui::B787,MF_BYCOMMAND|(f.aircraftId==6?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(menu,ui::TerrainSatellite,MF_BYCOMMAND|(sim::worldMode.load()==1?MF_CHECKED:MF_UNCHECKED));CheckMenuItem(menu,ui::TerrainMountains,MF_BYCOMMAND|(sim::worldMode.load()==0?MF_CHECKED:MF_UNCHECKED));
 CheckMenuItem(menu,ui::PowerSaver,MF_BYCOMMAND|(powerSaver?MF_CHECKED:MF_UNCHECKED));CheckMenuItem(menu,ui::Assist,MF_BYCOMMAND|(f.assist?MF_CHECKED:MF_UNCHECKED));
}
inline void drawMenu(std::vector<Vertex>& out,ui::Page page,const sim::Snapshot& state,bool powerSaver,const std::wstring& notice){
 clickableButtons.clear();if(page==ui::Page::None)return;
 screenQuad(out,180,100,600,415,{.025f,.045f,.07f});screenQuad(out,180,100,600,3,{.2f,.7f,.85f});
 auto& font=chineseFont();std::wstring title;
 if(page==ui::Page::Aircraft)title=L"选择飞机与客舱载荷";else if(page==ui::Page::Missions)title=L"选择任务";else if(page==ui::Page::Lighting)title=L"亮度、空气透视与投影";else title=L"设置与视角";

 if(page==ui::Page::Home||page==ui::Page::Pause){
  const bool paused=page==ui::Page::Pause;
  // Draw over the paused 3D view using the existing HUD vertex batch.
  screenQuad(out,180,100,600,76,{.043f,.083f,.12f});
  screenQuad(out,180,100,600,3,{.34f,.83f,.72f});
  font.draw(out,L"FLIGHTCAREER AIR",210,116,{.45f,.86f,.78f},330);
  font.draw(out,paused?L"飞行暂停":L"飞行简报",210,140,{.95f,.98f,1.f},330);
  font.draw(out,L"v0.82",702,120,{.64f,.75f,.83f},52);

  screenQuad(out,508,191,242,235,{.049f,.082f,.11f});
  screenQuad(out,508,191,2,235,{.18f,.48f,.50f});
  const auto& flight=state.flight;
  const auto& career=state.career;
  const XMFLOAT3 label{.44f,.75f,.73f},value{.91f,.96f,1.f},muted{.65f,.75f,.83f};
  font.draw(out,L"当前机型",526,207,label,206);
  font.draw(out,flight.spec().name,526,230,value,206);
  font.draw(out,L"载客 "+std::to_wstring(flight.passengers)+L" / "+std::to_wstring(flight.spec().seats)+L" 人",526,254,muted,206);
  screenQuad(out,526,282,206,1,{.14f,.25f,.29f});
  font.draw(out,L"当前任务",526,296,label,206);
  font.draw(out,career.name(),526,319,value,206);
  font.draw(out,L"生涯积分 "+std::to_wstring(career.credits),526,343,muted,206);
  font.draw(out,powerSaver?L"画面 · 20 FPS 限帧":L"画面 · 不限帧",526,390,muted,206);

  auto button=[&](int command,float x,float y,float width,float height,const wchar_t* text,bool primary=false){
   clickableButtons.push_back({command,x,y,width,height,text});
   screenQuad(out,x,y,width,height,primary?XMFLOAT3{.34f,.83f,.72f}:XMFLOAT3{.085f,.145f,.19f});
   font.draw(out,text,x+12,y+(height-18)/2,primary?XMFLOAT3{.02f,.12f,.13f}:value,int(width-24));
  };
  button(ui::Continue,210,191,282,54,paused?L"返回飞行":L"开始 / 继续飞行",true);
  button(ui::AircraftPage,210,255,136,37,L"选择飞机");
  button(ui::Missions,356,255,136,37,L"任务中心");
  button(ui::Settings,210,301,136,37,L"设置与视角");
  button(ui::Help,356,301,136,37,L"操作指南");
  button(ui::Save,210,347,136,37,L"保存飞行");
  button(ui::Load,356,347,136,37,L"读取飞行");
  button(ui::Restart,210,393,136,33,L"重新出发");
  button(ui::Exit,356,393,136,33,L"保存并退出");
  button(ui::Account,508,436,242,31,L"公开机库网站");
  font.draw(out,notice.substr(0,33),210,450,{.95f,.85f,.5f},286);
  font.draw(out,L"Tab / Esc 返回飞行    F5 保存",210,493,muted,540);
  return;
 }
 font.draw(out,title,210,117,{.75f,.9f,1});font.draw(out,state.flight.spec().name,210,144,{.7f,.75f,.8f});
 auto add=[&](int command,const wchar_t* text){
  size_t i=clickableButtons.size();const int columns=page==ui::Page::Aircraft?3:2;float x=210+(i%columns)*(columns==3?190.f:285.f),y=185+(i/columns)*(page==ui::Page::Aircraft?39.f:48.f);
  clickableButtons.push_back({command,x,y,columns==3?175.f:255.f,page==ui::Page::Aircraft?34.f:41.f,text});
 };
 if(page==ui::Page::Aircraft){
  add(ui::Trainer,L"驾驶轻型教练机");add(ui::Airliner,L"驾驶70座支线客机");add(ui::A330,L"驾驶 A330-300");add(ui::A350,L"驾驶 A350-900");add(ui::A320,L"驾驶 A320");add(ui::B737,L"驾驶 B737");add(ui::B787,L"驾驶 B787");add(ui::A319,L"A319 · 近似");add(ui::A321,L"A321 · 近似");add(ui::B7879,L"B787-9 · 近似");add(ui::B777,L"驾驶 777-300ER");add(ui::ImportModel,L"导入自定义机模");add(ui::ResetModel,L"恢复默认机模");add(ui::RotateModel,L"旋转机模90度");add(ui::PassengersLess,L"乘客 -10人");add(ui::PassengersMore,L"乘客 +10人");add(ui::PassengersFull,L"乘客满载");add(ui::PassengersEmpty,L"清空乘客");add(ui::Home,L"返回主菜单");add(ui::Continue,L"开始 / 继续飞行");
 }else if(page==ui::Page::Missions){
  add(ui::License,L"执照训练");add(ui::Cargo,L"货物运输");add(ui::Passenger,L"双机场旅客航班");add(ui::Race,L"计时竞速");add(ui::Photo,L"摄影巡游");add(ui::Fullscreen,L"全屏 / 窗口（F11）");add(ui::Home,L"返回主菜单");
 }else if(page==ui::Page::Lighting){
  add(ui::ExposureLess,L"曝光 -0.25 EV");add(ui::ExposureMore,L"曝光 +0.25 EV");add(ui::HazeToggle,state.haze>.0002?L"空气透视：薄雾":L"空气透视：晴朗");add(ui::ShadowsToggle,state.shadows?L"地面投影：开":L"地面投影：关");add(ui::Noon,L"正午光照");add(ui::Sunset,L"傍晚光照");add(ui::LightReset,L"重置曝光与光影");add(ui::Settings,L"返回设置");
 }else{
  add(ui::PowerSaver,powerSaver?L"帧率限制：20帧":L"帧率限制：不限制");add(ui::Assist,state.flight.assist?L"辅助回正：已开":L"辅助回正：已关");add(ui::CameraChase,L"追尾视角");add(ui::CameraCockpit,L"三维驾驶舱 · 鼠标操作");add(ui::CameraTop,L"俯视视角");add(ui::TerrainSatellite,L"上海浦东 · 卫星图");add(ui::TerrainMountains,L"山海地形 · 虚构");add(ui::LightingPage,L"亮度与光影");add(ui::SuperSample,L"2×超采样 / 原生分辨率");add(ui::AntiAlias,state.antiAlias?L"FXAA抗锯齿：开":L"FXAA抗锯齿：关");add(ui::Fullscreen,L"全屏 / 窗口（F11）");add(ui::Home,L"返回主菜单");
 }
 if(page==ui::Page::Missions){
  const auto& c=state.career;
  font.draw(out,std::wstring(c.name())+L" · 阶段 "+std::to_wstring(c.stage)+L"/3 · 航点 "+std::to_wstring(std::clamp(c.gate+1,1,4))+L"/4",210,389,{.48f,.9f,.78f},550);
  font.draw(out,L"积分 "+std::to_wstring(c.credits)+L" · 机队 "+std::to_wstring(c.fleet)+L" · 计时 "+std::to_wstring(int(c.elapsed))+L" 秒",210,413,{.7f,.8f,.86f},550);
 }
 for(const auto& b:clickableButtons){screenQuad(out,b.x,b.y,b.w,b.h,{.08f,.16f,.23f});font.draw(out,b.label,b.x+12,b.y+11,{.95f,.97f,1},int(b.w-24));}
 font.draw(out,notice.substr(0,33),210,474,{.95f,.85f,.5f},550);
 std::wstring foot=page==ui::Page::Aircraft?(ui::passengerFlightActive(state)?L"旅客航班进行中，完成或重新出发后才能调整乘客。":L"换机与登机需要在跑道停稳。乘客重量影响起降。"):L"鼠标点击选择 · Tab / Esc关闭菜单 · 飞行已冻结";
 font.draw(out,foot,210,499,{.65f,.75f,.82f},550);
}
