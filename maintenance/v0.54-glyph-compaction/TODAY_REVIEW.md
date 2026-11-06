## v0.63 · 2026-11-06（滚动性能统计配置边界）

基线：读取最新v0.62（文件版本43）、PROGRESS/TODAY_REVIEW与主循环/性能记录代码；原ZIP CRC与2279项SHA256通过，1661项assets/third_party逐字节一致。上轮修正low_mode并追加渲染尺寸与限帧字段，其44/44 Linux和17/17 Windows属于历史证据。

功能特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350，共用27控件三维驾驶舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、示意地景、FXAA与账号网站入口。机模和气动参数仍是近似原型；网站账号未绑定游戏存档，本轮未改变网站访问设置。

发现与改动：主循环先采样后处理输入，新配置CSV可能混入旧配置帧时。新增固定大小FrameContext，在统计/输出前检测机型、视角、地形、菜单、FXAA、low、模拟活动、步行、限帧和已应用宽高变化，清空300帧窗口与五秒计时，排除下一过渡帧。稳定配置重新累计再报告。CSV15列兼容，不增加资产或逐帧堆分配，不宣称FPS提升。

本轮验证：Linux Assimp ON Release完整282步构建成功，45/45 CTest通过（14.23秒）；全套严格C++17便携检查退出0。新增11种配置边界/6600稳定帧检查，实际main采样与重置语句覆盖8种切换、过渡帧、五秒重新累计、失焦恢复；ASan/UBSan通过（detect_leaks=0）。旧v0.62 main在相同检查中退出1，复现旧样本/计时未清空。v0.62 CSV16组兼容检查通过。日志见validation_v063。

Windows2022/MSVC x64 Release主程序编译链接成功，18/18 CPU检查通过（含新增配置边界、GDI及HLSL），8条既有警告。Assimp OFF、资源复制OFF；未运行Windows游戏、GPU或HD6000。证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/42230227288 ，日志摘录validation_v063/windows_excerpt.log。

GitHub：codex/v0.63-performance-segment，受检产品代码09a332cb9499cd6602fda769974e5982f81f58cb；继承目录maintenance/v0.54-glyph-compaction，内部版本0.63；GitHub默认main分支未改，完整资源与依赖在ZIP。后续提交仅补记录。

限制：只处理列出的11字段，不自动固定位置、观察方向、光照或模型替换；整段benchmark仍可混合不同配置，未强制锁定场景。统计仍为CPU墙钟，切换帧被排除，不适合测切换卡顿。没有Windows游戏/GPU/HD6000实测、内存测量或画面验收；启动资源缺失仍可能阻止启动。包内无EXE。

差距：2026-11-06检索微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，其物理、航电、天气及职业体验作为参考。当前缺真实机型系统/FMS、精细机场/DEM、复杂天气、气动标定及完整航班实机验收。没有接近成品的可核查画面和性能证据，不承诺5350U/HD6000/8GB达到同等画质或功能。

下一步：优先取得带完整资源的Windows启动证据，在目标Boot Camp固定机场/机型/视角/光照，比较原生与2×超采样各60秒帧时、内存和截图，并验证切换后CSV重新累计。代码侧后续可处理benchmark场景变更的有效性标记；这是待办，尚未实现。仍非成品，继续迭代。
