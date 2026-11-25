"""Compile the production Win32 menu creation and synchronization code with MSVC."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
menu = (root / 'src/menus.h').read_text(encoding='utf-8')
native = menu[menu.index('inline HMENU createChineseMenu'):menu.index('inline void drawMenu')]
source = r'''
#define NOMINMAX
#include <windows.h>
#include <optional>
#include <tuple>
#include "ui_state.h"
''' + native + r'''
int main(){
 HMENU menu=createChineseMenu();
 if(!menu)return 1;
 sim::Flight flight;
 NativeMenuChecks checks;
 auto checked=[&](int command){
  UINT flags=GetMenuState(menu,command,MF_BYCOMMAND);
  if(flags==UINT(-1))return -1;
  return (flags&MF_CHECKED)?1:0;
 };
 flight.configure(2);flight.assist=true;sim::worldMode.store(1);
 checks.update(menu,flight,true);
 if(!checks.applied||checked(ui::A330)!=1||checked(ui::A350)!=0||
    checked(ui::TerrainSatellite)!=1||checked(ui::Assist)!=1||checked(ui::PowerSaver)!=1)return 2;
 flight.configure(3);flight.assist=false;sim::worldMode.store(0);
 checks.update(menu,flight,false);
 if(!checks.applied||checked(ui::A350)!=1||checked(ui::A330)!=0||
    checked(ui::TerrainMountains)!=1||checked(ui::TerrainSatellite)!=0||
    checked(ui::Assist)!=0||checked(ui::PowerSaver)!=0)return 3;
 return DestroyMenu(menu)?0:4;
}
'''
with tempfile.TemporaryDirectory() as td:
    folder = Path(td)
    (folder/'check.cpp').write_text(source, encoding='utf-8')
    subprocess.run(['cl','/nologo','/EHsc','/utf-8','/std:c++17','/I'+str(root/'src'),
                    'check.cpp','user32.lib'], cwd=folder, check=True)
    subprocess.run([str(folder/'check.exe')], cwd=folder, check=True)
print('PASS real Win32 native menu states before and after aircraft, terrain, assist and frame cap changes.')
