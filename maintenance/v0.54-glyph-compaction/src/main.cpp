#include "graphics.h"
#include "windows_launch.h"
#include "scene.h"
#include "storage.h"
#include "tutorial.h"
#include "menus.h"
#include "cockpit.h"
#include "autopilot.h"
#include "autopilot_feedback.h"
#include "frame_timing.h"
#include "performance.h"
#include "benchmark.h"
#include "run_files.h"
#include "flight_review.h"
#include "approach.h"
#include "telemetry.h"
#include "engineering_panel.h"
#include "debug_draw.h"
#include "fleet_assets.h"
#include "model_transform.h"
#include "model_restore.h"
#include "keyboard_state.h"
#include <commdlg.h>
#include <shellapi.h>
#include <map>
#include <set>
#ifdef FCAIR_WITH_ASSIMP
#include "asset_import.h"
#endif
#include <chrono>
#include "terrain_stream.h"
#include <thread>
#include <deque>
#include <iomanip>

inline input::KeyboardState keyboardInput;
inline bool fullscreen=false;inline WINDOWPLACEMENT windowedPlacement{sizeof(WINDOWPLACEMENT)};
inline HMENU windowedMenu=nullptr;
inline void toggleFullscreen(HWND h){
 keyboardInput.suspend();
 cockpitInput.cancel();if(GetCapture()==h)ReleaseCapture();
 if(!fullscreen){
  windowedPlacement.length=sizeof(WINDOWPLACEMENT);GetWindowPlacement(h,&windowedPlacement);windowedMenu=GetMenu(h);
  MONITORINFO m{sizeof(MONITORINFO)};if(!GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&m))return;
  SetMenu(h,nullptr);SetWindowLongPtrW(h,GWL_STYLE,WS_POPUP|WS_VISIBLE);
  SetWindowPos(h,HWND_TOP,m.rcMonitor.left,m.rcMonitor.top,m.rcMonitor.right-m.rcMonitor.left,m.rcMonitor.bottom-m.rcMonitor.top,SWP_FRAMECHANGED|SWP_NOOWNERZORDER);fullscreen=true;
 }else{
  SetWindowLongPtrW(h,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);SetMenu(h,windowedMenu);SetWindowPlacement(h,&windowedPlacement);
  SetWindowPos(h,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);fullscreen=false;
 }
}
inline bool cockpitMouseDown=false;inline int lookX=0,lookY=0;
bool key(int k){return (GetAsyncKeyState(k)&0x8000)!=0;}
LRESULT CALLBACK windowProc(HWND h,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_KEYDOWN&&w==VK_F11&&!(l&(1LL<<30))){menuCommands.push_back(ui::Fullscreen);return 0;}
 if(msg==WM_SYSKEYDOWN&&w==VK_RETURN&&!(l&(1LL<<30))){menuCommands.push_back(ui::Fullscreen);return 0;}
 if(msg==WM_CLOSE){menuCommands.push_back(ui::Exit);return 0;}
 if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
 if(msg==WM_COMMAND){menuCommands.push_back(LOWORD(w));return 0;}
 if(msg==WM_ENTERMENULOOP){keyboardInput.suspend();cockpitInput.cancel();cockpitMouseDown=false;if(GetCapture()==h)ReleaseCapture();inNativeMenu=true;return 0;}
 if(msg==WM_EXITMENULOOP){keyboardInput.suspend();inNativeMenu=false;nativeMenuJustClosed=true;return 0;}
 if(msg==WM_ENTERSIZEMOVE){keyboardInput.suspend();cockpitInput.cancel();cockpitMouseDown=false;if(GetCapture()==h)ReleaseCapture();}
 if(msg==WM_KEYDOWN&&w==VK_ESCAPE&&!(l&(1LL<<30))){menuCommands.push_back(ui::ToggleMenu);return 0;}
 if(msg==WM_KEYDOWN||msg==WM_KEYUP){
  if(GetForegroundWindow()==h&&!IsIconic(h)&&!inNativeMenu){
   if(msg==WM_KEYDOWN)keyboardInput.keyDown(static_cast<std::size_t>(w),(l&(1LL<<30))!=0,
    {(GetKeyState(VK_SHIFT)&0x8000)!=0,(GetKeyState(VK_CONTROL)&0x8000)!=0});
   else if((w!=VK_SHIFT&&w!=VK_CONTROL)||(GetKeyState(static_cast<int>(w))&0x8000)==0)
    keyboardInput.keyUp(static_cast<std::size_t>(w));
  }else keyboardInput.suspend();
 }
 if(msg==WM_KILLFOCUS){keyboardInput.suspend();cockpitInput.cancel();cockpitMouseDown=false;if(GetCapture()==h)ReleaseCapture();}
 if(msg==WM_CAPTURECHANGED){cockpitInput.loseCapture();}
 if(msg==WM_RBUTTONDOWN&&cockpitInput.enabled){if(cockpitInput.beginLook()){lookX=(short)LOWORD(l);lookY=(short)HIWORD(l);SetCapture(h);}return 0;}
 if(msg==WM_RBUTTONUP&&cockpitInput.enabled){if(cockpitInput.looking){cockpitInput.endLook();if(GetCapture()==h)ReleaseCapture();}return 0;}
 if((msg==WM_LBUTTONDOWN||msg==WM_LBUTTONUP)&&cockpitInput.looking){cockpitMouseDown=false;return 0;}
 if(msg==WM_MOUSEMOVE&&cockpitInput.looking){int x=(short)LOWORD(l),y=(short)HIWORD(l);cockpit::lookYaw=std::clamp(cockpit::lookYaw+(x-lookX)*.004f,-.85f,.85f);cockpit::lookPitch=std::clamp(cockpit::lookPitch-(y-lookY)*.004f,-.8f,.55f);lookX=x;lookY=y;return 0;}
 if(msg==WM_LBUTTONDOWN||msg==WM_MOUSEMOVE){
  RECT b{};GetClientRect(h,&b);float x=(short)LOWORD(l)*960.f/std::max(1L,b.right),y=(short)HIWORD(l)*540.f/std::max(1L,b.bottom);
  if(msg==WM_LBUTTONDOWN&&cockpitInput.press(x,y)){cockpitMouseDown=true;if(cockpitInput.dragging)SetCapture(h);return 0;}
  if(msg==WM_MOUSEMOVE&&cockpitInput.dragging){cockpitInput.move(y);return 0;}
 }
 if(msg==WM_LBUTTONUP&&cockpitMouseDown){RECT b{};GetClientRect(h,&b);cockpitInput.move((short)HIWORD(l)*540.f/std::max(1L,b.bottom));cockpitInput.release();cockpitMouseDown=false;if(GetCapture()==h)ReleaseCapture();return 0;}
 if(msg==WM_LBUTTONUP){
  RECT bounds{};GetClientRect(h,&bounds);float x=(short)LOWORD(l)*960.f/std::max(1L,bounds.right),y=(short)HIWORD(l)*540.f/std::max(1L,bounds.bottom);
  for(const auto& b:clickableButtons)if(x>=b.x&&x<=b.x+b.w&&y>=b.y&&y<=b.y+b.h){menuCommands.push_back(b.command);break;}return 0;
 }
 return DefWindowProc(h,msg,w,l);
}
std::filesystem::path dataFolder(){
 wchar_t local[32768]{};DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
 if(!n||n>=32768)throw std::runtime_error("LOCALAPPDATA unavailable");
 auto folder=std::filesystem::path(local)/L"FlightCareerAir";std::filesystem::create_directories(folder);return folder;
}
void help(HWND window){
 MessageBoxW(window,
  L"第一步：W加油门；教练机110、支线客机220、大客机245～255km/h抬头。\n"
  L"客机建议F按两次，放50%襟翼后滑跑；抬头后等待离地。\n"
  L"第二步：爬升到200m，取得执照任务检查点。\n"
  L"第三步：返场对准X=0、Z=-1800..1800的跑道，G放起落架，F放襟翼，S减油门。下降速度不超过5m/s，空格刹停。\n\n"
  L"W/S油门；方向键俯仰/滚转；A/D方向舵；空格刹车。\n"
  L"F襟翼；G起落架；I发动机；L辅助回正；C视角；E步行。\n"
  L"F11 或 Alt+Enter：全屏/窗口；默认原生分辨率，无帧率上限。\n"
  L"驾驶舱：鼠标点击开关，拖动右侧油门杆；V停留刹车。\n"
  L"切回窗口、关闭对话框或调整窗口后，原来按住的键需松开再按。\n"
  L"单次动作保留两帧间短按；同一键一帧内多次短按合并一次。\n"
  L"F2工程面板；F3力矢量；F6/F7左右发失效；F8复位；F9切分翼面模式；Backspace确认警告。\n"
  L"Q电池；M发电机；Y航电。Home/End气压；PageUp/Down配平；U开关8m/s横风。\n"
  L"J：仪表亮度循环；O：座舱泛光灯，也可点击仪表台左右旋钮。\n"
  L"1执照 2运输 3旅客 4计时竞速 5摄影；K摄影；B购买；H维修。\n"
  L"T季节；N时间；P暂停；R重新起飞；F5保存；Tab/Esc打开菜单。\n\n"
  L"卫星影像：Sentinel-2 cloudless https://s2maps.eu by EOX IT Services GmbH\n"
  L"Contains modified Copernicus Sentinel data 2016 & 2017 · CC BY 4.0\n"
  L"https://creativecommons.org/licenses/by/4.0/；机场与高程为简化实现。\n"
  L"A330使用简化附加起落架；A350采用原模型起落架显隐。\n\n"
  L"存档/照片/日志：%LOCALAPPDATA%\\FlightCareerAir\n"
  L"诊断输出另存：diagnostics\\benchmark 或 diagnostics\\smoke；不保存游戏生涯。\n"
  L"DX12初始化失败会自动尝试DX11；可用--dx11强制兼容模式。",
  L"飞行生涯 v0.72 · 中文操作说明",MB_OK);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){
 std::filesystem::path folder;
 try{
  const auto args=launch::currentOptions();const bool benchmark=args.has(L"--benchmark");const bool low=args.has(L"--low")||benchmark;const bool smoke=args.has(L"--smoke-test");int smokeFrames=0;
  const runfiles::RunFiles runFiles(dataFolder(),benchmark,smoke);
  folder=runFiles.folder();std::filesystem::create_directories(folder);
  diagnostics.open(folder/L"startup.log",std::ios::trunc);logLine("Flight Career Air v0.72 startup");
  if(!runFiles.gameSavesEnabled())logLine("Diagnostic output isolated; live game saves disabled");
  sim::Snapshot state;bool loaded=false,primaryValid=false;
  {std::ifstream saved(folder/L"state.txt");if(saved){if(!state.read(saved))logLine("Invalid state.txt ignored; attempting legacy career");else {loaded=primaryValid=true;logLine("Loaded compatible full flight state");}}}
  if(!loaded){std::ifstream backup(folder/L"state.bak");if(backup&&state.read(backup)){loaded=true;logLine("Recovered state.bak");}}
  if(!loaded){std::ifstream legacy(folder/L"career.txt");state.career.load(legacy);logLine("Legacy v2 career migration attempted");}
  if(smoke&&(args.has(L"--airliner")||args.has(L"--a330")||args.has(L"--a350"))){state.flight=sim::Flight{};state.flight.configure(args.has(L"--a350")?3:args.has(L"--a330")?2:1);state.career.stage=state.career.gate=0;}
  if(smoke){state.clock=args.has(L"--sunset")?17.4:12;state.exposureEV=0;state.terrainMode=1;state.haze=.00016;state.shadows=true;}
  if(benchmark){state=sim::Snapshot{};state.flight.configure(3);state.clock=12;state.exposureEV=0;state.terrainMode=1;state.haze=.00016;state.shadows=true;state.antiAlias=!args.has(L"--no-aa");}
  if(smoke){
   if(args.has(L"--night"))state.clock=0;
   if(args.has(L"--flood-off"))state.cockpitFlood=false;
   else state.cockpitFlood=true;
   state.panelBrightness=args.has(L"--panel-dim")?.25:.75;
  }
  if(args.has(L"--surface-flight")){state.flight.setSurfaceModel(true);state.debugPanel=true;state.showForces=true;}
  sim::worldMode.store(state.terrainMode);
  auto& flight=state.flight;auto& career=state.career;bool backupEligible=primaryValid;
  auto save=[&](){
   if(!runFiles.gameSavesEnabled())return;
   auto temp=folder/L"state.tmp",dest=folder/L"state.txt";
   {std::ofstream out(temp);state.write(out);out.flush();if(!out)throw std::runtime_error("Save write failed");out.close();if(!out)throw std::runtime_error("Save close failed");}
   if(backupEligible&&std::filesystem::exists(dest)){
    const auto backupTemp=folder/L"state.bak.tmp",backup=folder/L"state.bak";
    if(!CopyFileW(dest.c_str(),backupTemp.c_str(),FALSE))throw std::runtime_error("Save backup copy failed");
    if(!MoveFileExW(backupTemp.c_str(),backup.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Save backup commit failed");
   }
   if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Save commit failed");
   backupEligible=true;
  };
  SetProcessDPIAware();
  const DWORD style=WS_OVERLAPPEDWINDOW;
  WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=instance;wc.lpszClassName=L"FlightCareerAir";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);if(!RegisterClassW(&wc))throw std::runtime_error("Window registration failed");
  RECT bounds{0,0,low?640:1600,low?360:900};HMENU nativeMenu=createChineseMenu();AdjustWindowRect(&bounds,style,TRUE);
  HWND window=CreateWindowW(wc.lpszClassName,L"飞行生涯 v0.72 · 可操作驾驶舱",style,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nativeMenu,instance,nullptr);
  if(!window)throw std::runtime_error("Window creation failed");ShowWindow(window,SW_SHOW);
  if(args.has(L"--fullscreen"))toggleFullscreen(window);
  renderScale=launch::initialRenderScale(args);
  const auto renderArgs=benchmark?args.with(L"--low"):args;
  auto renderer=createRenderer(window,renderArgs);
  RECT renderClient{};GetClientRect(window,&renderClient);
  LONG clientWidth=renderClient.right,clientHeight=renderClient.bottom;UINT appliedScale=renderScale;
  cockpit::viewportAspect=low?960.f/540:float(clientWidth)/std::max(1L,clientHeight);
  std::map<int,ImportedModel> aircraftModels;std::set<int> customModels;
  for(int id=0;id<sim::aircraftCount;id++){
   const auto* profile=sim::fleetAsset(id);
   const auto defaultPath=profile?assetFolder()/profile->nativeName:std::filesystem::path{};
   const auto customPath=folder/("custom_aircraft_"+std::to_string(id)+".fcmesh");
   auto selected=loadStartupModel(defaultPath,customPath);
   if(!selected.warning.empty())logLine("Custom model ignored (aircraft "+std::to_string(id)+"): "+selected.warning);
   if(selected.model)aircraftModels.emplace(id,std::move(*selected.model));
   if(selected.custom)customModels.insert(id);
  }
  logLine("Aircraft catalog ready: "+std::to_string(sim::aircraftCount)+" profiles, "+std::to_string(aircraftModels.size())+" loaded meshes, "+std::to_string(customModels.size())+" custom meshes");
  struct Mark{float x,z;};std::deque<Mark> marks;
  {std::ifstream in(folder/L"tracks.txt");float x,z;while(marks.size()<400&&in>>x>>z)if(std::isfinite(x)&&std::isfinite(z))marks.push_back({x,z});}
  auto saveTracks=[&](){if(!runFiles.gameSavesEnabled())return;auto temp=folder/L"tracks.tmp",dest=folder/L"tracks.txt";{std::ofstream out(temp);for(auto m:marks)out<<m.x<<' '<<m.z<<'\n';out.flush();if(!out)throw std::runtime_error("Track save write failed");out.close();if(!out)throw std::runtime_error("Track save close failed");}if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Track save commit failed");};
  int camera=smoke?0:1;
  if((smoke&&args.has(L"--cockpit"))||benchmark)camera=1;
  if(args.has(L"--no-aa"))state.antiAlias=false;else if(smoke)state.antiAlias=true;
  bool paused=smoke,walking=false,powerSaver=low&&!benchmark;ui::Page menuPage=(smoke||benchmark)?ui::Page::None:ui::Page::Home;XMFLOAT3 walker{};float walkYaw=0;
  presentation::TerrainStream<std::vector<Vertex>> terrainStream;
  presentation::FrameGeometry<Vertex> frameGeometry;frameGeometry.scene.reserve(140000);
  auto& mesh=frameGeometry.scene;auto& cabin=frameGeometry.cabin;auto& hud=frameGeometry.hud;
  std::filesystem::path pendingPhoto;std::wstring notice=L"欢迎：在主菜单选择飞机，再开始飞行。";
  // Save recovery: disk failures must not destroy the in-memory flight.
  std::wstring saveError;
  auto trySave=[&](){
   try{save();saveTracks();
    if(!saveError.empty())notice=L"保存已恢复，可继续飞行。";
    saveError.clear();return true;
   }catch(const std::exception& error){
    logLine(std::string("Save failed: ")+error.what());
    saveError=L"保存失败，飞行已暂停。检查磁盘空间或权限后按F5重试；退出会丢失未保存进度。";
    notice=saveError;paused=true;return false;
   }
  };
  // End save recovery.
  std::wstring lastOverlayNotice;double overlayNoticeUntil=0;
  std::ofstream perf(folder/L"performance.csv",std::ios::trunc);
  perf<<"elapsed_s,backend,aircraft,camera,terrain,fxaa,low_mode,active,samples,fps,p95_ms,max_ms,render_width,render_height,software_frame_cap\n";
  if(!perf)logLine("Performance CSV unavailable");
  std::ofstream aeroLog(folder/L"flight_diagnostics.csv",std::ios::trunc);int aeroSamples=0;double diagnosticTime=0;
  aeroLog<<sim::FlightSample::header()<<'\n';
  sim::AlertManager flightAlerts;
  if(!aeroLog)logLine("Flight diagnostics CSV unavailable");
  timing::FrameWindow frameWindow;timing::FrameContextGate frameContextGate;bool previousMeasured=false;double reportTime=0,wallElapsed=0;
  bool benchmarkStarted=false;timing::BenchmarkSamples benchmarkSamples;
  sim::LandingResult lastLanding;
  double markTimer=0,autoSave=0;
  auto last=std::chrono::steady_clock::now();timing::FixedSteps physicsTime;presentation::FlightHistory renderHistory;renderHistory.reset(flight);bool wasActive=false;MSG msg{};
  bool exitApproved=false,exitWithoutSave=false;
  // Exit recovery: save while the flight window still exists.
  auto requestExit=[&](){
   paused=true;keyboardInput.suspend();cockpitInput.cancel();
   if(GetCapture()==window)ReleaseCapture();
   const bool saved=smoke||benchmark||trySave();
   const bool leave=saved||MessageBoxW(window,
    L"保存失败，当前航班仍保留在内存中。\n选择“否”返回，修复磁盘空间或权限后按 F5 重试。\n\n仍要退出并放弃未保存的进度吗？",
    L"保存失败 · 是否仍然退出",MB_YESNO|MB_ICONWARNING|MB_DEFBUTTON2)==IDYES;
   keyboardInput.suspend();cockpitInput.cancel();menuCommands.clear();
   physicsTime.reset();wasActive=false;previousMeasured=false;last=std::chrono::steady_clock::now();
   if(!leave){logLine("Exit cancelled after save failure");return false;}
   if(!DestroyWindow(window)){notice=L"无法关闭窗口，请重试。";logLine("DestroyWindow failed");return false;}
   exitWithoutSave=!saved;return true;
  };
  // End exit recovery.
  while(msg.message!=WM_QUIT){
   while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
   if(msg.message==WM_QUIT)break;
   // Handle close before resize, keyboard polling and the minimized-window wait.
   if(std::find(menuCommands.begin(),menuCommands.end(),ui::Exit)!=menuCommands.end()){
    exitApproved=requestExit();if(exitApproved)break;continue;
   }
   const auto autopilotBeforeInput=sim::autopilotModes(state);
   RECT currentClient{};GetClientRect(window,&currentClient);
   if(!IsIconic(window)&&currentClient.right>0&&currentClient.bottom>0&&(currentClient.right!=clientWidth||currentClient.bottom!=clientHeight||appliedScale!=renderScale)){
    keyboardInput.suspend();cockpitInput.cancel();renderer.reset();renderer=createRenderer(window,renderArgs);
    clientWidth=currentClient.right;clientHeight=currentClient.bottom;appliedScale=renderScale;cockpit::viewportAspect=low?960.f/540:float(clientWidth)/clientHeight;
    previousMeasured=false;physicsTime.reset();wasActive=false;last=std::chrono::steady_clock::now();
   }
   auto now=std::chrono::steady_clock::now();
   const double wallDt=std::chrono::duration<double>(now-last).count();
   double dt=std::min(.25,wallDt);last=now;
   wallElapsed+=wallDt;
   const bool measured=GetForegroundWindow()==window&&!IsIconic(window)&&!inNativeMenu&&!nativeMenuJustClosed;
   if(measured&&previousMeasured){frameWindow.add(wallDt);reportTime+=wallDt;}
   else{frameWindow.reset();reportTime=0;}
   if(benchmark&&measured&&previousMeasured){benchmarkStarted=true;benchmarkSamples.add(wallDt);}
   previousMeasured=measured;
   if(nativeMenuJustClosed){dt=0;physicsTime.reset();wasActive=false;nativeMenuJustClosed=false;}
   bool focus=GetForegroundWindow()==window;
   const bool inputAvailable=focus&&!IsIconic(window)&&!inNativeMenu;
   input::KeyboardState::Keys physicalKeys{};
   for(int k=0;k<256;k++)physicalKeys[k]=inputAvailable&&key(k);
   keyboardInput.sample(physicalKeys,inputAvailable);
   const auto& down=keyboardInput.down();const auto& pressed=keyboardInput.pressed();
   if(keyboardInput.resumed()&&keyboardInput.blockedCount()){
    notice=L"已恢复输入：原来按住的键需先松开再按。";
    logLine("Keyboard recovery: "+std::to_string(keyboardInput.blockedCount())+" held key(s) gated until release");
   }
   if(IsIconic(window)){std::this_thread::sleep_for(std::chrono::milliseconds(100));physicsTime.reset();wasActive=false;continue;}
   if(pressed[VK_F1])menuCommands.push_back(ui::Help);
   if(pressed[VK_TAB])menuCommands.push_back(ui::ToggleMenu);
   if(menuPage==ui::Page::None){
    if(pressed['P'])paused=!paused;
    if(pressed['C'])camera=(camera+1)%3;
    if(!paused&&!walking&&!flight.crashed){
    if(pressed[VK_F2])state.debugPanel=!state.debugPanel;
    if(pressed[VK_F3])cockpit::apply(state,cockpit::Action::ForceDisplay);
    if(pressed[VK_F6])flight.engines.inject(0,keyboardInput.modifiers(VK_F6).shift?sim::EngineFault::Fire:keyboardInput.modifiers(VK_F6).control?sim::EngineFault::Generator:sim::EngineFault::Flameout,flight.aircraftId);
    if(pressed[VK_F7])flight.engines.inject(1,keyboardInput.modifiers(VK_F7).shift?sim::EngineFault::Fire:keyboardInput.modifiers(VK_F7).control?sim::EngineFault::Generator:sim::EngineFault::Flameout,flight.aircraftId);
    if(pressed[VK_F8])cockpit::apply(state,cockpit::Action::ResetFaults);
    if(pressed[VK_F9])cockpit::apply(state,cockpit::Action::SurfaceModel);
    if(pressed[VK_BACK])flightAlerts.acknowledge();
    if(pressed['Q'])cockpit::apply(state,cockpit::Action::Battery);
    if(pressed['M'])cockpit::apply(state,cockpit::Action::Generator);
    if(pressed['Y'])cockpit::apply(state,cockpit::Action::Avionics);
    if(pressed[VK_HOME])cockpit::apply(state,cockpit::Action::QnhMinus);
    if(pressed[VK_END])cockpit::apply(state,cockpit::Action::QnhPlus);
    if(pressed[VK_PRIOR])cockpit::apply(state,cockpit::Action::TrimPlus);
    if(pressed[VK_NEXT])cockpit::apply(state,cockpit::Action::TrimMinus);
    if(pressed['U'])cockpit::apply(state,cockpit::Action::Wind);
    if(pressed['J'])cockpit::apply(state,cockpit::Action::PanelBrightness);
    if(pressed['O'])cockpit::apply(state,cockpit::Action::CockpitFlood);
    if(pressed['L'])cockpit::apply(state,cockpit::Action::Assist);
    if(pressed['G'])cockpit::apply(state,cockpit::Action::Gear);
    if(pressed['F'])cockpit::apply(state,cockpit::Action::Flaps);
    if(pressed['I'])cockpit::apply(state,cockpit::Action::Engine);
    if(pressed['V'])cockpit::apply(state,cockpit::Action::ParkingBrake);
    }
    if(pressed['T']){state.season=(state.season+1)%4;state.seasonElapsed=0;}
    if(pressed['N'])state.clock=std::fmod(state.clock+3,24.0);
    if(pressed['R'])menuCommands.push_back(ui::Restart);
    if(pressed['E']&&career.parked(flight)&&(!walking||std::hypot(walker.x-flight.x,walker.z-flight.z)<10)){
     walking=!walking;walker={(float)flight.x+(flight.aircraftId>0?8:4),1.8f,(float)flight.z};walkYaw=(float)flight.yaw;
    }
    if(!walking){
     for(int k=0;k<5;k++)if(pressed['1'+k])menuCommands.push_back(ui::License+k);
     if(pressed['B']){notice=career.buy(flight)?L"已购买出租飞机。":L"购买需要：执照、停稳和6000积分。";trySave();}
     if(pressed['H']){notice=career.service(flight)?L"已加油并完成维修。":L"请在跑道停稳，并准备300积分。";trySave();}
     if(pressed['K']){
      if(career.canPhotograph(flight))pendingPhoto=folder/("photo_"+std::to_string(career.photos+1)+".bmp");
      else notice=L"摄影条件：距当前航点350米内，高度80～650米。";
     }
    }
   }
   if(pressed[VK_F5])menuCommands.push_back(ui::Save);
   while(!menuCommands.empty()){
    int cmd=menuCommands.front();menuCommands.pop_front();dt=0;physicsTime.reset();wasActive=false;
    switch(cmd){
     case ui::SuperSample:renderScale=renderScale==1?2:1;notice=renderScale==2?L"2×超采样：宽高各两倍，四倍像素。":L"按窗口原生分辨率渲染。";break;
     case ui::Fullscreen:toggleFullscreen(window);break;
     case ui::Account:{
      const auto result=ShellExecuteW(window,L"open",L"https://flightcareer-pilot-chenrui.downydeck.chatgpt.site/",nullptr,nullptr,SW_SHOWNORMAL);
      notice=reinterpret_cast<INT_PTR>(result)>32?L"已打开网站账号中心。请在浏览器登录；生涯存档仍在本机。":L"无法打开浏览器，请查看ACCOUNT_AND_FLEET.md中的网站地址。";
      keyboardInput.suspend();paused=true;break;
     }
     case ui::Continue:menuPage=ui::Page::None;paused=false;notice=L"已开始飞行。Tab打开菜单，F1查看操作。";break;
     case ui::ToggleMenu:menuPage=menuPage==ui::Page::None?ui::Page::Pause:ui::Page::None;break;
     case ui::Home:menuPage=ui::Page::Home;break;
     case ui::AircraftPage:menuPage=ui::Page::Aircraft;break;
     case ui::Missions:menuPage=ui::Page::Missions;break;
     case ui::LightingPage:menuPage=ui::Page::Lighting;break;
     case ui::ExposureLess:case ui::ExposureMore:state.exposureEV=std::clamp(state.exposureEV+(cmd==ui::ExposureMore?.25:-.25),-2.0,2.0);notice=L"曝光 EV："+std::to_wstring(state.exposureEV);trySave();break;
     case ui::HazeToggle:state.haze=state.haze>.0002?.00016:.00032;trySave();break;
     case ui::ShadowsToggle:state.shadows=!state.shadows;trySave();break;
     case ui::LightReset:state.exposureEV=0;state.haze=.00016;state.shadows=true;trySave();break;
     case ui::Noon:state.clock=12;trySave();break;
     case ui::AntiAlias:state.antiAlias=!state.antiAlias;trySave();break;
     case ui::Sunset:state.clock=17.4;trySave();break;
     case ui::Settings:menuPage=ui::Page::Settings;break;
     case ui::Save:if(trySave())notice=runFiles.gameSavesEnabled()?L"已保存飞行状态与生涯。":L"诊断运行不保存飞行状态与生涯。";break;
     case ui::Load:{
      sim::Snapshot loadedState;std::ifstream in(folder/L"state.txt");
      if(in&&loadedState.read(in)){state=loadedState;flightAlerts.beginFlight(state);sim::worldMode.store(state.terrainMode);backupEligible=true;walking=false;physicsTime.reset();menuPage=ui::Page::Pause;notice=L"已读取飞行，可选择继续。";}else notice=L"读取失败：没有有效存档，当前飞行保持不变。";break;
     }
     case ui::Restart:ui::restart(state);flightAlerts.beginFlight(state);walking=false;physicsTime.reset();notice=L"已重新出发，保留燃油；事故恢复会扣费。";trySave();break;
     case ui::Exit:exitApproved=requestExit();break;
     case ui::Trainer:case ui::Airliner:case ui::A330:case ui::A350:case ui::A320:case ui::B737:case ui::B787:case ui::A319:case ui::A321:case ui::B7879:
      if(ui::switchAircraft(state,ui::aircraftIdForCommand(cmd))){flightAlerts.beginFlight(state);walking=false;physicsTime.reset();notice=L"已切换飞机，并重置本次任务。";trySave();}else notice=L"换机需要在跑道停稳；空中请先返场。";break;
     case ui::ImportModel:{
      if(!career.parked(flight)){notice=L"请先在跑道停稳后导入机模。";break;}
#ifdef FCAIR_WITH_ASSIMP
      cockpitInput.cancel();wchar_t selected[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=window;dialog.lpstrFile=selected;dialog.nMaxFile=32768;
      dialog.lpstrFilter=L"飞机模型 (*.glb;*.gltf;*.obj;*.fbx;*.dae;*.3ds;*.stl)\0*.glb;*.gltf;*.obj;*.fbx;*.dae;*.3ds;*.stl\0全部文件\0*.*\0\0";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
      if(GetOpenFileNameW(&dialog)){
       try{
        AircraftImportOptions options;options.lengthMeters=sim::visualLength(flight.aircraftId);options.groundOffset=std::max(0.,flight.spec().gearHeight-1.5);
        auto imported=importAircraftModel(std::filesystem::path(selected),options);
        auto destination=folder/("custom_aircraft_"+std::to_string(flight.aircraftId)+".fcmesh");auto temporary=folder/L"custom_model.tmp";
        saveAircraftModel(imported.model,temporary);
        if(!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot commit imported model");
        aircraftModels[flight.aircraftId]=std::move(imported.model);customModels.insert(flight.aircraftId);
        notice=L"已导入机模；飞行参数沿用当前所选机型。";logLine(imported.report.warnings);
       }catch(const std::exception& error){notice=L"机模导入失败，保留原模型。";logLine(std::string("Aircraft import failed: ")+error.what());}
      }
#else
      notice=L"此构建未启用Assimp导入功能。";
#endif
      keyboardInput.suspend();physicsTime.reset();previousMeasured=false;last=std::chrono::steady_clock::now();break;
     }
     case ui::RotateModel:{
      if(!career.parked(flight)){notice=L"请先在跑道停稳后调整模型朝向。";break;}
#ifdef FCAIR_WITH_ASSIMP
      auto source=aircraftModels.find(flight.aircraftId);if(source==aircraftModels.end()){notice=L"请先导入机模。";break;}
      try{
       auto rotated=rotateAircraftQuarterTurn(source->second,sim::visualLength(flight.aircraftId),std::max(0.,flight.spec().gearHeight-1.5));
       auto destination=folder/("custom_aircraft_"+std::to_string(flight.aircraftId)+".fcmesh");auto temporary=folder/L"custom_model.tmp";saveAircraftModel(rotated,temporary);
       if(!MoveFileExW(temporary.c_str(),destination.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot commit rotated model");
       source->second=std::move(rotated);customModels.insert(flight.aircraftId);notice=L"机模已旋转90度并重新匹配长度。";
      }catch(const std::exception& error){logLine(error.what());notice=L"模型旋转失败，保留原模型。";}
#else
      notice=L"此构建未启用模型导入编辑功能。";
#endif
      break;
     }
     case ui::ResetModel:{
      if(!career.parked(flight)){notice=L"请先在跑道停稳后恢复模型。";break;}
      try{
       if(const auto* profile=sim::fleetAsset(flight.aircraftId))restoreDefaultModel(aircraftModels,flight.aircraftId,assetFolder()/profile->nativeName);else aircraftModels.erase(flight.aircraftId);
      }catch(const std::exception& error){
       logLine(std::string("Default model restore failed: ")+error.what());
       notice=L"默认模型读取失败，已保留当前模型和自定义文件。请检查资源后重试。";break;
      }
      std::error_code removeError;std::filesystem::remove(folder/("custom_aircraft_"+std::to_string(flight.aircraftId)+".fcmesh"),removeError);customModels.erase(flight.aircraftId);notice=removeError?L"默认模型已恢复，但旧自定义文件未能删除。":L"已恢复当前机型默认模型。";break;
     }
     case ui::PassengersLess:case ui::PassengersMore:case ui::PassengersFull:case ui::PassengersEmpty:{
      int pax=cmd==ui::PassengersFull?flight.spec().seats:cmd==ui::PassengersEmpty?0:flight.passengers+(cmd==ui::PassengersMore?10:-10);
      if(ui::setPassengers(state,pax)){notice=L"客舱载荷已更新，乘客重量会影响起降。";trySave();}else notice=L"请驾驶客机并在跑道停稳后调整乘客。";break;
     }
     case ui::TerrainSatellite:case ui::TerrainMountains:
      if(career.parked(flight)){state.terrainMode=cmd==ui::TerrainSatellite?1:0;sim::worldMode.store(state.terrainMode);marks.clear();notice=state.terrainMode?L"已切换浦东离线卫星图，周边建筑与道路为示意地景。":L"已切换虚构山海地形。";trySave();}else notice=L"切换地形需要先在跑道停稳。";break;
     case ui::PowerSaver:powerSaver=!powerSaver;break;
     case ui::Assist:flight.assist=!flight.assist;break;
     case ui::CameraChase:camera=0;break;
     case ui::CameraCockpit:camera=1;break;
     case ui::CameraTop:camera=2;break;
     case ui::Help:help(window);keyboardInput.suspend();last=std::chrono::steady_clock::now();dt=0;break;
     default:if(cmd>=ui::License&&cmd<=ui::Photo){bool selected=career.select(cmd-ui::License,flight);notice=selected?L"已选择任务，点击继续飞行即可开始。":L"请在跑道停稳；高级任务需要先取得执照。";if(selected&&career.mission==2&&flight.aircraftId>0&&flight.passengers==0)flight.passengers=50;if(selected)trySave();}break;
    }
   }
   if(exitApproved)break;
   if(!IsWindow(window))continue;
   bool cockpitEnabled=camera==1&&!walking&&!paused&&menuPage==ui::Page::None&&!inNativeMenu&&focus&&!flight.crashed&&pendingPhoto.empty();
   if(cockpitEnabled){for(const auto& event:cockpitInput.pending)cockpit::apply(state,event.action,event.value);cockpitInput.pending.clear();cockpitInput.enabled=true;}
   else{cockpitInput.cancel();if(GetCapture()==window)ReleaseCapture();}

   if(!state.avionics.powered(flight)){
    const auto lost=sim::disconnectedAutopilotModes(autopilotBeforeInput,sim::autopilotModes(state));
    if(lost.any())notice=sim::autopilotDisconnectMessage(lost);
   }

   if(inNativeMenu){last=std::chrono::steady_clock::now();dt=0;}
   updateChineseMenu(nativeMenu,flight,powerSaver);
   if(!paused&&menuPage==ui::Page::None&&!inNativeMenu&&focus&&pendingPhoto.empty()){
    if(!wasActive){dt=0;renderHistory.reset(flight);}
    int physicsSteps=physicsTime.advance(dt,!walking);
    if(!walking)dt=physicsTime.acceptedSeconds;
    wasActive=true;
    state.clock=std::fmod(state.clock+dt/90,24.0);state.seasonElapsed+=dt;
    if(state.seasonElapsed>=300){state.season=(state.season+1)%4;state.seasonElapsed-=300;}
    if(walking){
     walkYaw+=(down['D']-down['A'])*(float)dt;float movement=(down['W']-down['S'])*(float)dt*4;
     walker.x+=std::sin(walkYaw)*movement;walker.z+=std::cos(walkYaw)*movement;walker.y=terrain(walker.x,walker.z)+1.8f;
    }else{
     flight.payload=career.mission==1?250:(career.mission==2?150:0);flight.condition=career.maintenance;
     sim::Controls controls;controls.pitch=down[VK_DOWN]-down[VK_UP];controls.roll=down[VK_RIGHT]-down[VK_LEFT];controls.yaw=down['D']-down['A'];controls.throttle=down['W']-down['S'];controls.brake=down[VK_SPACE]||state.parkingBrake;controls.parkingBrake=state.parkingBrake;
     for(int stepIndex=0;stepIndex<physicsSteps;++stepIndex){
      const bool wasGrounded=flight.grounded();const double impactVy=flight.vy,impactRoll=flight.roll,impactYaw=flight.yaw;
      const auto autopilotBefore=sim::autopilotModes(state);
      sim::Controls commanded=controls;
      sim::updateAutopilot(state,commanded);
      flight.step(.01,commanded);
      renderHistory.push(flight);
      state.avionics.step(.01,flight);
      if(!state.avionics.powered(flight))state.headingHold=state.altitudeHold=state.navigationHold=false;
      const auto autopilotLost=sim::disconnectedAutopilotModes(autopilotBefore,sim::autopilotModes(state));
      if(autopilotLost.any())notice=sim::autopilotDisconnectMessage(autopilotLost);
      diagnosticTime+=.01;
      if(++aeroSamples%10==0&&aeroLog){
       auto sample=sim::FlightSample::capture(state);sample.values[0]=diagnosticTime;sample.write(aeroLog);
       if(aeroSamples%100==0)aeroLog.flush();
      }
      if(wasGrounded&&!flight.grounded()&&career.mission==2&&career.stage==1)career.arrivalLandingScore=-1;
      if(!wasGrounded&&flight.grounded()){
       lastLanding=sim::assessLanding(flight,impactVy,impactRoll,impactYaw);
       career.recordTouchdown(flight,lastLanding);
       if(lastLanding.valid)notice=L"着陆评分 "+std::to_wstring(lastLanding.score)+L"分 · "+sim::landingGrade(lastLanding.score)+L"（下降率 "+std::to_wstring(int(std::lround(lastLanding.sinkRate)))+L"m/s）";
      }
      int before=career.completed,beforeCredits=career.credits;career.update(flight,.01);
      if(career.completed>before&&career.mission==2){
       std::ofstream report(folder/L"flight_reports.txt",std::ios::app);
       career.writeFlightReport(report,flight,career.credits-beforeCredits);
       if(!report)logLine("Flight report write failed; settlement remains valid");
      }
      if(career.completed>before){notice=career.mission==2?L"航班完成：综合评分 "+std::to_wstring(career.lastFlightScore)+L"，票价与奖励已到账。":L"任务完成：奖励与出租收入已到账。";trySave();}
     }
     markTimer+=dt;
     if(flight.grounded()&&flight.speed()>2&&markTimer>.25){marks.push_back({(float)flight.x,(float)flight.z});if(marks.size()>400)marks.pop_front();markTimer=0;}
    }
    autoSave+=dt;if(autoSave>=30){trySave();autoSave=0;}
   }else{physicsTime.reset();renderHistory.reset(flight);wasActive=false;}
   if(walking)renderHistory.reset(flight);
   const auto renderFlight=renderHistory.sample(flight,physicsTime.fraction());
   sim::Snapshot renderState=state;renderState.flight=renderFlight;
   flightAlerts.update(state);
   const float daylight=(float)std::clamp(std::sin((state.clock-6)/24*2*sim::pi)*.8+.3,.08,1.0);
   const XMFLOAT3 colors[]={{.32f,.62f,.28f},{.24f,.5f,.2f},{.65f,.4f,.17f},{.83f,.88f,.92f}};
   float anchorX=walking?walker.x:(float)flight.x,anchorZ=walking?walker.z:(float)flight.z;
   int tx=(int)std::floor(anchorX/400),tz=(int)std::floor(anchorZ/400);
   const presentation::TerrainKey wantedTerrain{tx,tz,state.season,state.terrainMode};
   auto terrainUpdate=terrainStream.update(wantedTerrain,[colors](presentation::TerrainKey key){
    return landscape(key.x,key.z,colors[key.season],key.mode);
   });
   if(terrainUpdate==presentation::TerrainUpdate::Failed){
    logLine(std::string("Terrain generation failed; retaining last mesh: ")+terrainStream.lastError());
    notice=L"地景加载失败，已保留上次地景；请查看startup.log。";
   }
   frameGeometry.beginFrame(terrainStream.mesh());XMMATRIX cabinView=XMMatrixIdentity();
   for(auto m:marks)box(mesh,m.x,terrain(m.x,m.z)+.14f,m.z,.2f,.01f,1,{.04f,.04f,.04f});
   if(career.mission>0&&career.mission!=2&&career.stage<2&&career.gate<4){auto p=career.route[career.gate];gateMesh(mesh,(float)p.x,(float)p.z,140);}
   box(mesh,0,.03f,0,35,.03f,1800,{.12f,.13f,.15f});
   box(mesh,3500,.03f,2500,24,.03f,700,{.13f,.14f,.16f});
   for(int z=-1750;z<1800;z+=80)box(mesh,0,.11f,(float)z,.6f,.02f,12,{.9f,.9f,.82f});
   for(int z=-1780;z<=1780;z+=100){box(mesh,-34,.2f,(float)z,.25f,.1f,.25f,{1,.9f,.7f});box(mesh,34,.2f,(float)z,.25f,.1f,.25f,{1,.9f,.7f});}
   airportBuildings(mesh);
   auto plane=XMMatrixRotationRollPitchYaw(-(float)renderFlight.pitch,(float)renderFlight.yaw,-(float)renderFlight.roll)*XMMatrixTranslation((float)renderFlight.x,(float)renderFlight.y,(float)renderFlight.z);
   const auto selectedModel=aircraftModels.find(renderFlight.aircraftId);
   if(camera!=1||walking){if(selectedModel!=aircraftModels.end())importedPlaneMesh(mesh,plane,renderFlight,selectedModel->second);else if(renderFlight.aircraftId==1)airlinerMesh(mesh,plane,renderFlight);else planeMesh(mesh,plane,renderFlight);}
   if(renderFlight.gear&&(camera!=1||walking)&&(renderFlight.aircraftId>=4||customModels.count(renderFlight.aircraftId))&&selectedModel!=aircraftModels.end()&&std::none_of(selectedModel->second.gear.begin(),selectedModel->second.gear.end(),[](unsigned char value){return value!=0;}))supplementalGear(mesh,plane,renderFlight);
   if(renderFlight.aircraftId==2&&renderFlight.gear&&(camera!=1||walking))a330Gear(mesh,plane);
   if(state.showForces&&camera!=1&&!walking)drawFlightForces(mesh,renderFlight);
   XMVECTOR position=XMVectorSet((float)renderFlight.x,(float)renderFlight.y,(float)renderFlight.z,1);
   auto orientation=XMMatrixRotationRollPitchYaw(-(float)renderFlight.pitch,(float)renderFlight.yaw,-(float)renderFlight.roll);
   XMVECTOR forward=XMVector3TransformNormal(XMVectorSet(0,0,1,0),orientation),eye,viewDirection,up=XMVectorSet(0,1,0,0);
   if(walking){eye=XMLoadFloat3(&walker);viewDirection=XMVectorSet(std::sin(walkYaw),0,std::cos(walkYaw),0);}
   else if(camera==1){auto anchor=XMMatrixTranslation(0,renderFlight.aircraftId>0?1.1f:.7f,renderFlight.aircraftId>=2?float(sim::visualLength(renderFlight.aircraftId)*.43):renderFlight.aircraftId==1?11.f:0.f)*plane;eye=XMVector3TransformCoord(XMVectorZero(),anchor);auto look=cockpit::ray(480,270);viewDirection=XMVector3TransformNormal(XMVectorSet(look.x,look.y,look.z,0),orientation);up=XMVector3TransformNormal(XMVectorSet(-std::sin(cockpit::lookYaw)*std::sin(cockpit::lookPitch),std::cos(cockpit::lookPitch),-std::cos(cockpit::lookYaw)*std::sin(cockpit::lookPitch),0),orientation);cockpitMesh(cabin,renderState,XMMatrixIdentity());
    const auto localLook=XMVectorSet(look.x,look.y,look.z,0);
    const auto localUp=XMVectorSet(-std::sin(cockpit::lookYaw)*std::sin(cockpit::lookPitch),std::cos(cockpit::lookPitch),-std::cos(cockpit::lookYaw)*std::sin(cockpit::lookPitch),0);
    cabinView=XMMatrixLookAtLH(XMVectorZero(),localLook,localUp)*reversedPerspective(XM_PI/3,cockpit::viewportAspect,presentation::cabinNear,presentation::cabinFar);cockpitInput.throttle=renderFlight.throttle;}
   else if(camera==2){eye=position+XMVectorSet(0,renderFlight.aircraftId>=2?280:renderFlight.aircraftId==1?200:120,-20,0);viewDirection=position-eye;}
   else{eye=position-forward*(renderFlight.aircraftId>=2?125:renderFlight.aircraftId==1?85:22)+XMVectorSet(0,renderFlight.aircraftId>=2?38:renderFlight.aircraftId==1?25:7,0,0);viewDirection=forward*((renderFlight.aircraftId>=2?125:renderFlight.aircraftId==1?85:22)+15)-XMVectorSet(0,renderFlight.aircraftId>=2?38:renderFlight.aircraftId==1?25:7,0,0);}
   // Input can change configuration after the interval was sampled. Discard
   // that old interval and the next transition frame before reporting again.
   const auto frameExtent=launch::renderExtent(UINT(std::max(1L,clientWidth)),UINT(std::max(1L,clientHeight)),appliedScale,low);
   const timing::FrameContext frameContext{flight.aircraftId,camera,state.terrainMode,static_cast<int>(menuPage),state.antiAlias,low,wasActive,walking,powerSaver,frameExtent.width,frameExtent.height};
   if(benchmark)benchmarkSamples.observeContext(frameContext);
   if(frameContextGate.changed(frameContext)){frameWindow.reset();reportTime=0;previousMeasured=false;}
   if(!saveError.empty())notice=saveError;
   const auto frameStats=frameWindow.summary();
   if(reportTime>=5&&perf){
    const auto loggedExtent=launch::renderExtent(UINT(std::max(1L,clientWidth)),UINT(std::max(1L,clientHeight)),appliedScale,low);
    perf<<std::fixed<<std::setprecision(3)<<wallElapsed<<','<<renderer->name()<<','<<flight.aircraftId<<','<<camera<<','<<state.terrainMode<<','<<state.antiAlias<<','<<low<<','<<wasActive<<','<<frameStats.samples<<','<<frameStats.fps<<','<<frameStats.p95Ms<<','<<frameStats.maxMs<<','<<loggedExtent.width<<','<<loggedExtent.height<<','<<(powerSaver?20:0)<<'\n';
    perf.flush();reportTime=0;
   }
   const bool inCockpit=camera==1&&!walking;
   if(menuPage==ui::Page::None){
    if(inCockpit)drawCockpit(hud,state,flight.crashed?wideUtf8(flight.crashReason):notice,paused);
    else if(!walking){
     const auto data=sim::airData(flight,state.avionics.qnh);
     const auto primary=instruments::primary(data.ias,data.altitude,data.verticalSpeed,flight.yaw);
     screenQuad(hud,260,482,440,30,{.018f,.028f,.038f});
     chineseFont().draw(hud,primary.speedAltitude+L"  HDG "+instruments::heading(flight.yaw),270,488,{.85f,.95f,1},420);
     if(flight.crashed||flight.stallWarning||!state.avionics.powered(flight))drawCockpit(hud,state,flight.crashed?wideUtf8(flight.crashReason):notice,paused);
    }
    drawEngineeringPanel(hud,state,flightAlerts);
    if(!flight.crashed&&!notice.empty()){
     // Show each changed notification once for five seconds; do not cover the glass.
     if(notice!=lastOverlayNotice){lastOverlayNotice=notice;overlayNoticeUntil=wallElapsed+5;}
     if(wallElapsed<overlayNoticeUntil||!saveError.empty()){screenQuad(hud,12,425,936,26,{.018f,.028f,.038f});chineseFont().draw(hud,notice.substr(0,52),20,429,{.84f,.91f,.97f},920);}
    }
    if(state.debugPanel){
     std::wostringstream label;label<<std::fixed<<std::setprecision(1)<<wideUtf8(renderer->name())<<L"  "<<frameStats.fps<<L" FPS  P95 "<<frameStats.p95Ms<<L" ms";
     chineseFont().draw(hud,label.str(),650,498,{.85f,.9f,.95f},300);
    }
    chineseFont().draw(hud,L"C 视角  ·  F1 帮助  ·  Tab 菜单",12,495,{.55f,.65f,.72f},400);
   }
   drawMenu(hud,menuPage,state,powerSaver,notice);
   // Keep required asset attribution visible, below the flight UI.
   std::wstring credit=state.terrainMode?L"地景示意 · EOX / Copernicus 2016–2017 · CC BY 4.0":L"山海地形 · 虚构环境";
   if(flight.aircraftId>=4&&!customModels.count(flight.aircraftId))credit+=L" · 机模 amvlab CC BY 4.0";
   chineseFont().draw(hud,credit,8,520,{.55f,.65f,.72f},940);
   if(smoke&&smokeFrames==2)pendingPhoto=folder/(std::string("smoke_")+renderer->name()+"_"+std::to_string(flight.aircraftId)+(state.clock<6?"_night":state.clock>16?"_sunset":"_noon")+(state.cockpitFlood?"_flood":"_dark")+(state.panelBrightness<.5?"_dim":"_bright")+(camera==1?"_cockpit":"")+(state.antiAlias?"_fxaa":"_noaa")+".bmp");
   lightingFrame=makeLighting(state.clock,state.exposureEV,state.haze,state.shadows,state.panelBrightness,state.cockpitFlood&&state.avionics.busPowered(flight));
   auto vector4=[](XMVECTOR v){XMFLOAT4 f;XMStoreFloat4(&f,v);return LightVec{f.x,f.y,f.z,f.w};};
   const float activeFov=(camera==1&&!walking)?XM_PI/3:XM_PIDIV4;const float activeTan=std::tan(activeFov*.5f);
   XMVECTOR viewForward=XMVector3Normalize(viewDirection),viewRight=XMVector3Normalize(XMVector3Cross(up,viewForward)),viewUp=XMVector3Cross(viewForward,viewRight);
   lightingFrame.eye=vector4(eye);lightingFrame.forward=vector4(viewForward);lightingFrame.right=vector4(viewRight*(activeTan*cockpit::viewportAspect));lightingFrame.up=vector4(viewUp*activeTan);
   lightingFrame.plane={(float)renderFlight.x,(float)renderFlight.y,(float)renderFlight.z,(float)renderFlight.yaw};
   const auto dimensions=sim::airframeDimensions(flight.aircraftId);lightingFrame.shape=LightVec{float(std::max(.65,dimensions.span*.045)),float(dimensions.length*.5),float(dimensions.span*.5),float(flight.spec().gearHeight)};
   antiAliasEnabled=state.antiAlias;
   renderer->draw(mesh,hud,XMMatrixLookToLH(XMVectorZero(),viewDirection,up)*reversedPerspective(activeFov,cockpit::viewportAspect,presentation::worldNear,presentation::worldFar),daylight,pendingPhoto,cabin,cabinView);
   if(!pendingPhoto.empty()){if(!smoke){career.photograph(flight);notice=L"照片已保存，摄影航点已确认。";trySave();}pendingPhoto.clear();}
   if(smoke&&++smokeFrames>=3){logLine("Smoke test: three rendered frames and BMP capture succeeded");DestroyWindow(window);}
   if(benchmark&&timing::benchmarkFinished(benchmarkStarted,benchmarkSamples)){
    std::ofstream summary(folder/L"performance_summary.txt",std::ios::trunc);
    const auto result=timing::writeBenchmarkSummary(summary,"v0.76",renderer->name(),state.antiAlias,powerSaver,benchmarkSamples);
    summary.flush();if(!summary)logLine("Benchmark summary write failed; no report file success claimed");
    pendingPhoto=folder/L"benchmark_final.bmp";renderer->draw(mesh,hud,XMMatrixLookToLH(XMVectorZero(),viewDirection,up)*reversedPerspective(activeFov,cockpit::viewportAspect,presentation::worldNear,presentation::worldFar),daylight,pendingPhoto,cabin,cabinView);pendingPhoto.clear();
    logLine(result.complete?"Benchmark completed with valid full-run statistics":"Benchmark ended without a valid performance verdict; inspect summary status/counts");DestroyWindow(window);
   }
   double spent=std::chrono::duration<double>(std::chrono::steady_clock::now()-now).count();
   const bool pacingFocused=GetForegroundWindow()==window;
   if(!pacingFocused)previousMeasured=false; // Exclude a background wait even if focus returns before the next frame.
   const double delay=timing::frameDelay(spent,powerSaver,pacingFocused);
   if(delay>0)std::this_thread::sleep_for(std::chrono::duration<double>(delay));
  }
  logLine("Reusable CPU geometry capacity bytes (not total RAM/GPU memory): "+std::to_string(frameGeometry.retainedBytes()));
  // Only abnormal WM_QUIT/WM_DESTROY reaches this fallback without an exit decision.
  if(!smoke&&!benchmark&&!exitApproved&&!trySave())exitWithoutSave=true;
  logLine(exitWithoutSave?"Shutdown with unsaved progress":"Normal shutdown");
 }catch(const std::exception& e){
  logLine(e.what());std::wstring message=L"启动或运行失败：\n"+wideUtf8(e.what())+L"\n\n日志："+(folder.empty()?std::wstring(L"%LOCALAPPDATA%\\FlightCareerAir"):folder.wstring())+L"\\startup.log\n可尝试兼容启动脚本 run_compat.bat。";
  MessageBoxW(nullptr,message.c_str(),L"飞行生涯 · 错误",MB_OK|MB_ICONERROR);return 1;
 }
 return 0;
}

