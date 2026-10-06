#include "graphics.h"
#include "scene.h"
#include "storage.h"
#include "tutorial.h"
#include <chrono>
#include <future>
#include <thread>
#include <deque>
#include <iomanip>

bool key(int k){return (GetAsyncKeyState(k)&0x8000)!=0;}
LRESULT CALLBACK windowProc(HWND h,UINT msg,WPARAM w,LPARAM l){
 if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
 if(msg==WM_KEYDOWN&&w==VK_ESCAPE){DestroyWindow(h);return 0;}
 return DefWindowProc(h,msg,w,l);
}
std::filesystem::path dataFolder(){
 wchar_t local[32768]{};DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",local,32768);
 if(!n||n>=32768)throw std::runtime_error("LOCALAPPDATA unavailable");
 auto folder=std::filesystem::path(local)/L"FlightCareerAir";std::filesystem::create_directories(folder);return folder;
}
void help(HWND window){
 MessageBoxW(window,
  L"第一步：W加油门，达到约110–140km/h后短按↓抬头。\n"
  L"第二步：爬升到200m，取得执照任务检查点。\n"
  L"第三步：返场对准X=0、Z=-700..700的跑道，G放起落架，F放襟翼，S减油门。下降速度不超过5m/s，空格刹停。\n\n"
  L"W/S油门；方向键俯仰/滚转；A/D方向舵；空格刹车。\n"
  L"F襟翼；G起落架；I发动机；L辅助回正；C视角；E步行。\n"
  L"1执照 2运输 3观光 4计时竞速 5摄影；K摄影；B购买；H维修。\n"
  L"T季节；N时间；P暂停；R重新起飞；F5保存；Esc保存退出。\n\n"
  L"存档/照片/日志：%LOCALAPPDATA%\\FlightCareerAir\n"
  L"DX12初始化失败会自动尝试DX11；可用--dx11强制兼容模式。",
  L"Flight Career Air v0.3 — 中文操作说明",MB_OK);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR command,int){
 std::filesystem::path folder;
 try{
  const std::wstring args(command);const bool low=args.find(L"--low")!=std::wstring::npos;const bool smoke=args.find(L"--smoke-test")!=std::wstring::npos;int smokeFrames=0;
  folder=dataFolder();diagnostics.open(folder/L"startup.log",std::ios::trunc);logLine("Flight Career Air v0.3 startup");
  sim::Snapshot state;bool loaded=false,primaryValid=false;
  {std::ifstream saved(folder/L"state.txt");if(saved){if(!state.read(saved))logLine("Invalid state.txt ignored; attempting legacy career");else {loaded=primaryValid=true;logLine("Loaded v3 full flight state");}}}
  if(!loaded){std::ifstream backup(folder/L"state.bak");if(backup&&state.read(backup)){loaded=true;logLine("Recovered state.bak");}}
  if(!loaded){std::ifstream legacy(folder/L"career.txt");state.career.load(legacy);logLine("Legacy v2 career migration attempted");}
  auto& flight=state.flight;auto& career=state.career;bool backupEligible=primaryValid;
  auto save=[&](){
   auto temp=folder/L"state.tmp",dest=folder/L"state.txt";
   {std::ofstream out(temp);state.write(out);out.flush();if(!out)throw std::runtime_error("Save write failed");}
   if(backupEligible&&std::filesystem::exists(dest)&&!CopyFileW(dest.c_str(),(folder/L"state.bak").c_str(),FALSE))throw std::runtime_error("Save backup failed");
   if(!MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Save commit failed");
   backupEligible=true;
  };
  const DWORD style=WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX;
  WNDCLASSW wc{};wc.lpfnWndProc=windowProc;wc.hInstance=instance;wc.lpszClassName=L"FlightCareerAir";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);if(!RegisterClassW(&wc))throw std::runtime_error("Window registration failed");
  RECT bounds{0,0,low?640:960,low?360:540};AdjustWindowRect(&bounds,style,FALSE);
  HWND window=CreateWindowW(wc.lpszClassName,L"Flight Career Air v0.3",style,CW_USEDEFAULT,CW_USEDEFAULT,bounds.right-bounds.left,bounds.bottom-bounds.top,nullptr,nullptr,instance,nullptr);
  if(!window)throw std::runtime_error("Window creation failed");ShowWindow(window,SW_SHOW);
  auto renderer=createRenderer(window,args);
  struct Mark{float x,z;};std::deque<Mark> marks;
  {std::ifstream in(folder/L"tracks.txt");float x,z;while(marks.size()<400&&in>>x>>z)if(std::isfinite(x)&&std::isfinite(z))marks.push_back({x,z});}
  auto saveTracks=[&](){std::ofstream out(folder/L"tracks.txt");for(auto m:marks)out<<m.x<<' '<<m.z<<'\n';};
  int camera=0,tileX=INT_MAX,tileZ=INT_MAX,cacheSeason=-1;
  bool paused=smoke,walking=false;XMFLOAT3 walker{};float walkYaw=0;
  std::future<std::vector<Vertex>> terrainJob;std::vector<Vertex> cached;
  std::filesystem::path pendingPhoto;std::string notice="F1 CHINESE HELP | W THROTTLE";
  bool previous[256]{};double markTimer=0,autoSave=0,fps=30;
  auto last=std::chrono::steady_clock::now();double accumulator=0;MSG msg{};
  while(msg.message!=WM_QUIT){
   while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
   if(msg.message==WM_QUIT)break;
   auto now=std::chrono::steady_clock::now();double dt=std::min(.25,std::chrono::duration<double>(now-last).count());last=now;
   bool focus=GetForegroundWindow()==window;
   bool down[256]{},pressed[256]{};
   for(int k=0;k<256;k++){down[k]=focus&&key(k);pressed[k]=down[k]&&!previous[k];previous[k]=down[k];}
   if(IsIconic(window)){std::this_thread::sleep_for(std::chrono::milliseconds(100));accumulator=0;continue;}
   if(pressed[VK_F1]){help(window);last=std::chrono::steady_clock::now();dt=0;}
   if(pressed['P'])paused=!paused;
   if(pressed['C'])camera=(camera+1)%3;
   if(pressed['L'])flight.assist=!flight.assist;
   if(pressed['G'])flight.gear=!flight.gear;
   if(pressed['F'])flight.flap=flight.flap==0?1:0;
   if(pressed['I'])flight.engine=!flight.engine;
   if(pressed['T']){state.season=(state.season+1)%4;state.seasonElapsed=0;}
   if(pressed['N'])state.clock=std::fmod(state.clock+3,24.0);
   if(pressed['R']){
    const double fuel=flight.fuel;bool accident=flight.crashed;
    if(accident){career.credits=std::max(0,career.credits-500);career.maintenance=std::max(.3,career.maintenance-.10);}
    flight=sim::Flight{};flight.fuel=fuel;career.stage=career.gate=0;career.elapsed=career.raceLap=0;walking=false;
    notice=accident?"ACCIDENT RECOVERY - UP TO 500 CREDITS CHARGED":"FLIGHT RESTARTED - FUEL PRESERVED";save();
   }
   if(pressed['E']&&career.parked(flight)&&(!walking||std::hypot(walker.x-flight.x,walker.z-flight.z)<10)){
    walking=!walking;walker={(float)flight.x+4,1.8f,(float)flight.z};walkYaw=(float)flight.yaw;
   }
   if(!walking){
    for(int k=0;k<5;k++)if(pressed['1'+k])notice=career.select(k,flight)?"MISSION SELECTED":"PARK ON RUNWAY / LICENSE REQUIRED";
    if(pressed['B']){notice=career.buy(flight)?"AIRCRAFT PURCHASED":"BUY: LICENSE + PARKED + 6000 CREDITS";save();}
    if(pressed['H']){notice=career.service(flight)?"REFUELED AND REPAIRED":"SERVICE: PARKED + 300 CREDITS";save();}
    if(pressed['K']){
     if(career.canPhotograph(flight))pendingPhoto=folder/("photo_"+std::to_string(career.photos+1)+".bmp");
     else notice="PHOTO TOUR: TARGET RANGE 350 M, HEIGHT 80-650 M";
    }
   }
   if(pressed[VK_F5]){save();saveTracks();notice="FLIGHT AND CAREER SAVED";}
   if(!paused&&focus&&pendingPhoto.empty()){
    state.clock=std::fmod(state.clock+dt/90,24.0);state.seasonElapsed+=dt;
    if(state.seasonElapsed>=300){state.season=(state.season+1)%4;state.seasonElapsed-=300;}
    if(walking){
     walkYaw+=(down['D']-down['A'])*(float)dt;float movement=(down['W']-down['S'])*(float)dt*4;
     walker.x+=std::sin(walkYaw)*movement;walker.z+=std::cos(walkYaw)*movement;walker.y=terrain(walker.x,walker.z)+1.8f;
    }else{
     flight.payload=career.mission==1?250:(career.mission==2?150:0);flight.condition=career.maintenance;
     sim::Controls controls;controls.pitch=down[VK_DOWN]-down[VK_UP];controls.roll=down[VK_RIGHT]-down[VK_LEFT];controls.yaw=down['D']-down['A'];controls.throttle=down['W']-down['S'];controls.brake=down[VK_SPACE];
     accumulator+=dt;
     while(accumulator>=.01){
      flight.step(.01,controls);int before=career.completed;career.update(flight,.01);
      if(career.completed>before){notice="MISSION COMPLETE - REWARD AND RENTAL PAID";save();}
      accumulator-=.01;
     }
     markTimer+=dt;
     if(flight.grounded()&&flight.speed()>2&&markTimer>.25){marks.push_back({(float)flight.x,(float)flight.z});if(marks.size()>400)marks.pop_front();markTimer=0;}
    }
    autoSave+=dt;if(autoSave>=30){save();saveTracks();autoSave=0;}
   }else accumulator=0;
   const float daylight=(float)std::clamp(std::sin((state.clock-6)/24*2*sim::pi)*.8+.3,.08,1.0);
   const XMFLOAT3 colors[]={{.32f,.62f,.28f},{.24f,.5f,.2f},{.65f,.4f,.17f},{.83f,.88f,.92f}};const auto ground=colors[state.season];
   float anchorX=walking?walker.x:(float)flight.x,anchorZ=walking?walker.z:(float)flight.z;
   int tx=(int)std::floor(anchorX/400),tz=(int)std::floor(anchorZ/400);
   if(terrainJob.valid()&&terrainJob.wait_for(std::chrono::seconds(0))==std::future_status::ready)cached=terrainJob.get();
   if((tx!=tileX||tz!=tileZ||state.season!=cacheSeason)&&!terrainJob.valid()){
    tileX=tx;tileZ=tz;cacheSeason=state.season;
    terrainJob=std::async(std::launch::async,[tx,tz,ground](){
     std::vector<Vertex> out;out.reserve(14000);
     for(int ix=-20;ix<20;ix++)for(int iz=-20;iz<20;iz++){
      float x=tx*400.f+ix*120,z=tz*400.f+iz*120;
      XMFLOAT3 a{x,terrain(x,z),z},b{x+120,terrain(x+120,z),z},c{x,terrain(x,z+120),z+120},d{x+120,terrain(x+120,z+120),z+120};
      auto shade=ground;float tone=.90f+.10f*std::sin(x*.04f+z*.03f);shade.x*=tone;shade.y*=tone;shade.z*=tone;tri(out,a,c,b,shade);tri(out,b,c,d,shade);
     }
     for(int n=0;n<60;n++){
      float x=tx*400.f+(n%10-5)*370+80,z=tz*400.f+(n/10-3)*530;
      if(std::abs(x)<180&&std::abs(z)<1000)continue;float y=terrain(x,z);
      box(out,x,y+3,z,.65f,3,.65f,{.25f,.17f,.10f});box(out,x,y+8,z,3,4,3,{.16f,.3f,.12f});
     }
     return out;
    });if(cached.empty())cached=terrainJob.get();
   }
   std::vector<Vertex> mesh=cached;mesh.reserve(45000);
   for(auto m:marks)box(mesh,m.x,.14f,m.z,.2f,.01f,1,{.04f,.04f,.04f});
   if(career.mission>0&&career.stage<2&&career.gate<4){auto p=career.route[career.gate];gateMesh(mesh,(float)p.x,(float)p.z,140);}
   box(mesh,0,.03f,0,22,.03f,700,{.12f,.13f,.15f});
   for(int z=-650;z<700;z+=60)box(mesh,0,.11f,(float)z,.6f,.02f,12,{.9f,.9f,.82f});
   for(int z=-680;z<=680;z+=80){box(mesh,-21,.2f,(float)z,.25f,.1f,.25f,{1,.9f,.7f});box(mesh,21,.2f,(float)z,.25f,.1f,.25f,{1,.9f,.7f});}
   box(mesh,70,8,-450,20,8,30,{.55f,.58f,.62f});box(mesh,95,15,-400,6,15,6,{.55f,.6f,.65f});
   auto plane=XMMatrixRotationRollPitchYaw(-(float)flight.pitch,(float)flight.yaw,-(float)flight.roll)*XMMatrixTranslation((float)flight.x,(float)flight.y,(float)flight.z);
   if(camera!=1||walking)planeMesh(mesh,plane,flight);
   XMVECTOR position=XMVectorSet((float)flight.x,(float)flight.y,(float)flight.z,1);
   auto orientation=XMMatrixRotationRollPitchYaw(-(float)flight.pitch,(float)flight.yaw,-(float)flight.roll);
   XMVECTOR forward=XMVector3TransformNormal(XMVectorSet(0,0,1,0),orientation),eye,target,up=XMVectorSet(0,1,0,0);
   if(walking){eye=XMLoadFloat3(&walker);target=eye+XMVectorSet(std::sin(walkYaw),0,std::cos(walkYaw),0);}
   else if(camera==1){eye=position+XMVectorSet(0,.7f,0,0);target=eye+forward;up=XMVector3TransformNormal(up,orientation);}
   else if(camera==2){eye=position+XMVectorSet(0,120,-20,0);target=position;}
   else{eye=position-forward*22+XMVectorSet(0,7,0,0);target=position+forward*15;}
   fps=.92*fps+.08/std::max(.001,dt);
   std::ostringstream display;display<<std::fixed<<std::setprecision(0);
   display<<"FLIGHT CAREER AIR V0.3 | "<<renderer->name()<<" | "<<fps<<" FPS | "<<(walking?"WALK":"FLIGHT")<<(paused?" PAUSED":"")<<"\n";
   display<<"IAS "<<flight.airspeed()*3.6<<" KM/H ALT "<<flight.y<<" M VS "<<flight.vy<<" M/S THR "<<flight.throttle*100<<"%\n";
   display<<"FUEL "<<flight.fuel<<"% RPM "<<flight.rpm<<" GEAR "<<(flight.gear?"DOWN":"UP")<<" FLAP "<<flight.flap*100<<"% ASSIST "<<(flight.assist?"ON":"OFF")<<"\n";
   display<<"PITCH "<<flight.pitch*180/sim::pi<<" BANK "<<flight.roll*180/sim::pi<<" HDG "<<std::fmod(flight.yaw*180/sim::pi+360,360)<<" | "<<career.name()<<" STAGE "<<career.stage<<"/3\n";
   display<<"CREDITS "<<career.credits<<" FLEET "<<career.fleet<<" PHOTO "<<career.photos<<" BEST "<<career.best<<" S TIME "<<career.elapsed<<" S\n";
   if(career.mission&&career.stage<2&&career.gate<4){auto p=career.route[career.gate];double bearing=std::fmod(std::atan2(p.x-flight.x,p.z-flight.z)*180/sim::pi+360,360);display<<"TARGET "<<career.gate+1<<"/4 DIST "<<std::hypot(flight.x-p.x,flight.z-p.z)<<" M BEARING "<<bearing<<" DEG\n";}
   else display<<"RUNWAY X 0 Z -700 TO 700 | LICENSE "<<(career.licensed?"YES":"NO")<<" | SEASON "<<state.season+1<<"\n";
   display<<sim::tutorialHint(flight,career.stage)<<"\n"<<notice<<"\n";
   display<<"1 LICENSE 2 CARGO 3 TOUR 4 RACE 5 PHOTO | B BUY H SERVICE K PHOTO\n";
   display<<"W/S POWER ARROWS ATTITUDE A/D RUDDER SPACE BRAKE | F1 HELP P PAUSE";
   std::vector<Vertex> hud;hudText(hud,display.str());
   if(smoke&&smokeFrames==2)pendingPhoto=folder/(std::string("smoke_")+renderer->name()+".bmp");
   renderer->draw(mesh,hud,XMMatrixLookAtLH(eye,target,up)*XMMatrixPerspectiveFovLH(XM_PIDIV4,960.f/540,.1f,6000),daylight,pendingPhoto);
   if(!pendingPhoto.empty()){if(!smoke){career.photograph(flight);notice="PHOTO SAVED - CHECKPOINT CONFIRMED";save();}pendingPhoto.clear();}
   if(smoke&&++smokeFrames>=3){logLine("Smoke test: three rendered frames and BMP capture succeeded");DestroyWindow(window);}
   const double targetFps=low?20:30;
   double spent=std::chrono::duration<double>(std::chrono::steady_clock::now()-now).count();
   if(spent<1/targetFps)std::this_thread::sleep_for(std::chrono::duration<double>(1/targetFps-spent));
  }
  if(!smoke){save();saveTracks();}logLine("Normal shutdown");
 }catch(const std::exception& e){
  logLine(e.what());std::string message=e.what();message+="\n\nLog: %LOCALAPPDATA%\\FlightCareerAir\\startup.log\nTry --dx11 or --low. See README for build and controls.";
  MessageBoxA(nullptr,message.c_str(),"Flight Career Air error",MB_OK|MB_ICONERROR);return 1;
 }
 return 0;
}
