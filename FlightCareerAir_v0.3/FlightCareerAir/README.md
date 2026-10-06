# Flight Career Air v0.3：分阶段实现 / 基础可运行阶段

面向 MacBook Air + Boot Camp、i5-5350U / HD Graphics 6000、8GB 内存的低负载 C++17 工程。
这是 v0.2 的后续源码版本；Windows 主程序还没有在真实 Windows/GPU 上编译运行。本包没有 EXE。
已有功能是游戏原型，不能用于真实飞行训练或认证。

## 这次实际完成的步骤

1. 拆分渲染、地形、飞行、任务、存档和操作指引模块。
2. DX12 优先；初始化失败自动回退 DX11 硬件。增加启动日志、低画质模式和图形自检入口。
3. 修正升力方向、控制输入惯性、地面接触、关发动机时燃油消耗、事故原因。
4. 加入滚转辅助回正、中文帮助窗口、实时起飞/返场提示、航点方位显示。
5. 加入完整飞行存档：位置、速度、燃油、发动机、起落架、任务进度、时间与季节。
6. 旧生涯存档迁移、自动保存、前一版本备份恢复；摄影截图写入成功后才确认打卡。
7. 保留五种任务和机队抽象经营；计时赛计时从起飞至最后航点，返场后才发奖。
8. 飞机外形从长方体改为分段机身，增加螺旋桨、机翼弹性倾斜、树木和跑道灯标。

默认 960×540 / 30 FPS 上限；--low 为 640×360 / 20 FPS 上限。
这是性能预算，尚未测量你机器的实际帧率。游戏离线运行，不要求高网速。

## 第一步：安装和编译

安装 Visual Studio Community，选择“使用 C++ 的桌面开发”，包括 MSVC x64、Windows SDK、C++ CMake 工具。
构建基准为 Visual Studio Community 2022；使用 CMake/Ninja，不依赖固定 VS 生成器名称。

1. 完整解压压缩包，进入 FlightCareerAir 文件夹。
2. 双击 build_vs.bat。脚本定位已有的 Visual Studio、设置 x64 环境、生成、编译、运行数值检查，然后启动游戏。
3. 也可以在 Visual Studio：文件 → 打开 → 文件夹 → 选择本目录。
4. 使用 Windows x64 Release 配置，生成，启动 FlightCareerAir.exe。

在“x64 Native Tools Command Prompt”中手动构建：

```bat
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release
out\build\release\FlightCareerAir.exe
```

CMake最低版本3.21，VS附带的版本应满足要求。构建后的EXE在 out\build\release。
如果缺少 CMake 或 Ninja，检查 VS 安装器内的 C++ CMake 工具组件。

## 第二步：验证显卡

正常启动先尝试DX12。若初始化失败，自动尝试DX11硬件；HUD第一行显示实际后端。
程序记录 %LOCALAPPDATA%\FlightCareerAir\startup.log，包含后端选择、显卡描述和错误。

- run_compat.bat：强制DX11 + 低画质。
- check_graphics.bat：依次验证DX11、DX12，各渲染三帧并输出BMP，然后退出；不推进或保存生涯。
- 成功时检查 smoke_DX11.bmp / smoke_DX12.bmp，确认画面和HUD可见。
- DX12失败、DX11成功时，可继续使用兼容模式，无需为了DX12切换到软件渲染。
- 软件渲染仅诊断：FlightCareerAir.exe --dx11 --warp --smoke-test。

支持的启动参数：--dx11、--dx12、--low、--warp、--smoke-test。
--dx12禁止自动回退，可用于单独诊断DX12。
--warp是WARP软件渲染，没有默认启用；不要把它当作低端显卡优化方案。
Windows须提供d3d12.dll和相应SDK/运行时，本工程不是Windows 7兼容程序。

## 第三步：实际起降

1. 初始在跑道南侧朝北，默认发动机打开、起落架放下。先按F1看中文帮助。
2. 按住W约2秒把油门加满，保持跑道中心。
3. 速度达到约110–140km/h，短按↓抬头，HUD俯仰约7度；避免一直按住。
4. 继续爬升到200m，执照任务从Stage 0推进到Stage 2。
5. 开始转弯返场：←/→滚转，松开会辅助回正。L关闭/开启辅助。坡度建议低于30度。
6. 对准跑道X=0、Z=-700..700；G放起落架、F放襟翼、S逐渐减油门，温和下降。
7. 接地下降速度需低于5m/s，坡度小于约17度。落地后空格刹车到低于1m/s，完成任务。

模拟检查验证了受控起飞、爬升至200m、另一组受控进近及刹停；这些不是飞行自动驾驶功能。
手动完整返场需要你在实际运行时测试。第一次不熟悉可以按R复位练习。
R保留燃油，清除当前任务进度；事故恢复最多扣500积分并降低维护状态，不会让余额变负。

## 操作

| 按键 | 功能 |
|---|---|
| W/S | 增减油门；步行时前后移动 |
| ↓/↑ | 抬头/低头 |
| ←/→ | 左右滚转 |
| A/D | 方向舵；步行转向 |
| 空格 | 地面刹车 |
| F/G/I | 襟翼/起落架/发动机 |
| L | 滚转辅助回正 |
| C | 追尾/第一人称/俯视 |
| E | 停稳后离开飞机；距飞机10m内返回 |
| 1/2/3/4/5 | 执照/运输/观光/计时赛/摄影 |
| K | 摄影任务当前航点内截图打卡 |
| B/H | 购买出租飞机/加油维修 |
| T/N | 季节切换/时间前进3小时 |
| P/R | 暂停/重新起飞 |
| F1/F5/Esc | 中文帮助/保存/保存退出 |

失去焦点或最小化时暂停飞行，最小化时降低循环负载。
第一人称当前没有三维驾驶舱，仪表由屏幕HUD提供。

## 生涯与机队

在跑道停稳后用1–5选择任务，2–5需要游戏执照。
执照奖励1500；运输2400、观光1800、竞速2600、摄影2000。
其他任务按顺序经过四航点后返场；运输有250kg载荷，观光有150kg载荷。
飞行航点要求高度80–650m，横向距离运输/观光300m、竞速140m。
摄影航点350m内按K；实际BMP保存成功后才计入摄影次数。HUD显示下一航点方位和距离。
黄色门框用于定位，任务范围由上述半径与高度判定，不是必须穿过门框平面的碰撞判定。

B购买同类出租飞机，6000积分，上限6架；每完成任务，额外飞机各带来120积分出租收入。
H加油维修，300积分；维护状态降低会影响发动机推力。
当前经营是抽象收支，没有可驾驶的不同飞机型号、雇员或AI运输调度。
奖励不会在已完成任务内重复领取；再次选择任务会开始新的任务。

## 存档与恢复

全部位于 %LOCALAPPDATA%\FlightCareerAir：

- state.txt：v3完整飞行与生涯状态，每30秒、F5、任务完成、购买/维修后和退出时保存。
- state.bak：上一次有效保存的备份；主存档损坏时尝试恢复。
- career.txt：v0.2旧存档，仅用于第一次迁移，不覆盖或改写。
- tracks.txt：最多400段轮胎痕迹。
- photo_N.bmp：摄影打卡截图，含HUD。
- startup.log：启动和错误记录，每次启动重新写入。

读取失败先试备份，再试旧版生涯，最后回到新建状态。
写入采用临时文件、备份和原子替换；不会把半解析的错误存档写进内存。
角速度、瞬间机翼弹性位移、步行位置不持久化；重新载入时步行恢复为飞行视角。

## 后续开发顺序

参见“阶段任务清单.md”。真实日食、CFD、结构软体、云端串流尚未完成。
它们不会被季节配色、机翼弹簧或本地离线运行替代为“已实现”。

## 本次验证

已在Linux用g++17、-Wall -Wextra -Werror -pedantic编译并运行tests.cpp。
检查：地面静止、关发动机燃油不变、硬着陆/收轮事故、辅助回正、机翼数值稳定、跑道起飞和200m爬升、受控进近接地刹停、五种任务、奖励防重、机队费用、摄影、旧生涯迁移、完整存档、损坏及越界存档拒绝。

Windows编译、DX11/DX12运行、HLSL编译、屏幕布局、截图、Boot Camp驱动兼容与帧率尚未实机验证。
check_graphics.bat是交付给你执行的检查入口，不能把提供该脚本称为已经验证Windows。

## 代码结构

- main.cpp：Win32窗口、输入、游戏循环、保存协调。
- graphics_common.h / graphics.h：共用图形接口、HLSL、BMP写入、后端选择。
- renderer12.h / renderer11.h：DX12主路径与DX11兼容后端。
- scene.h / world.h：模型、HUD、地形和跑道定义。
- simulation.h：简化飞行、机载状态、碰撞和弹簧。
- career.h / storage.h / tutorial.h：任务、经济、存档和操作提示。
- tests.cpp：可移植物理及流程检查。

## 官方参考

- DX11创建设备：https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-d3d11createdevice
- DX12创建设备：https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice
- Visual Studio CMake：https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio
