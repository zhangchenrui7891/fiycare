## v0.64 · 2026-11-07（benchmark配置失效保护）

基线：最新v0.63（文件版本44），读取PROGRESS/TODAY_REVIEW及benchmark/main实现。CRC及2294项SHA256通过。上轮45/45 Linux、18/18 Windows是历史结果，仅解决滚动窗口，不保证完整benchmark配置一致。

功能特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件驾驶舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东影像、FXAA与账号网站入口保留。机型外观/参数仍为原型，网站账号未绑定游戏存档。

本轮：BenchmarkSamples观察首次FrameContext，之后任一已跟踪字段变化即永久标记configuration_valid=0。valid/summary/结束条件和报告统一拒绝该次性能判定；切回原配置也不能恢复。主循环在生成报告前调用观察函数。报告改称initial_scene，显式声明continuous_scene_not_locked及FrameContext_11_fields覆盖范围。无效报告输出NA和NOT_EVALUATED，而非PASS/FAIL。没有实测FPS收益。

检查：11种配置变化、33011次稳定配置采样、切回初始配置、提前变化及报告抑制检查通过。生产main语句替身检查通过，旧v0.63 main在“Changed production context still permits benchmark verdict”断言退出1。新增ASan/UBSan检查（detect_leaks=0）通过；严格C++17全套便携检查退出0。

Linux Assimp ON Release完整282步构建成功，45/45 CTest通过（13.59秒）。1661项资源/第三方文件逐字节保持不变。

Windows2022/MSVC x64 Release主程序编译链接成功，19/19 CPU检查通过（含benchmark、GDI与HLSL），8条既有警告。Assimp OFF、资源复制OFF；未启动游戏或验证GPU/HD6000。证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/42276538544 ，关键日志见validation_v064/windows_excerpt.log。

GitHub：codex/v0.64-benchmark-guard，受检代码38559e00c6dbf7a70f89f10f7a9c7a1a0b432585；目录继承maintenance/v0.54-glyph-compaction，内部版本0.64，默认main分支不变。完整资源和依赖在ZIP，无EXE。

限制：此保护以第一次观察为基线，不验证第一次观察之前用户是否已改配置。11字段为机型、视角模式、地形、菜单页、FXAA、low、模拟活动、步行、限帧及渲染宽高。不监控飞行位置、观察角、时间/光照、模型替换等全部场景变化，不锁定交互。configuration_valid=1只表示未发现所跟踪字段改变，不证明固定场景；完整benchmark依然是640×360、CPU墙钟，非GPU帧时。报告initial_scene是预设初始场景说明，非全程实景证明。未增加资源/逐帧堆分配，未宣称性能提升。

差距：对照2026-11-07检索的微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，当前仍缺真实机型系统/FMS、精细机场/DEM、复杂天气、气动标定及完整航班验收。没有接近成品的画面/性能证据，不承诺5350U/HD6000/8GB达到相同画质功能。

下一步：优先带资源Windows启动和座舱验收，固定机场/机型/视角/光照，记录原生与2×各60秒帧时、RAM和截图；benchmark中按C或切换FXAA应得到NOT_EVALUATED并结束，重新启动才能重新测。当前没有目标Boot Camp机器连接，未进行Windows游戏/GPU/HD6000实测。尚非成品，继续迭代。
