Windows最终验证：2026-11-02，Windows Server2022/MSVC x64 Release主程序编译链接成功，12/12检查通过，包含新旧格式化对照、真实GDI及HLSL检查。Assimp OFF、资源复制OFF，没有运行游戏。8条既有MSVC警告仍在。运行：https://github.com/zhangchenrui7891/fiycare/actions/runs/41339897687 。受检代码16557dbc4affc613830088e00a861f45cd74d2da；后续只补资料。

## v0.55 · 2026-11-02（仪表整数格式化开销）
基线：实际读取v0.54，文件版本35，ZIP CRC及2168项SHA256核验通过。十机型、27控件共用驾驶舱、三屏玻璃航电、训练导航/进近、生涯、浦东离线影像、示意地景、FXAA和网站入口保留；网站账号未绑定游戏存档。本轮没有更改网站发布状态。

改动：flight_instruments.h的whole()对绝对值不超过1e15的有限数值，使用有界栈缓冲逐位构造十进制整数，避免每个仪表标签创建wostringstream和设置locale。先round再转换，保证符号和半数舍入；极大有限数保留旧流路径，NaN/Inf仍显示---，负零仍显示0。返回wstring仍可能分配，不能称为零分配。垂直速度/航向格式、物理、航电和存档未改。

验证：120027组新旧输出对照通过，覆盖整数及半数边界、正负数、±1e15边界、随机值、极大数、NaN/Inf、负零、非默认全局locale。严格C++17新测试及完整便携回归通过；新增ASan/UBSan通过（detect_leaks=0）。Linux Assimp开启Release构建、39/39 CTest通过。机模/贴图和第三方文件与基线逐字节一致。validation_v055含证据和可重复的format_benchmark.cpp。

性能边界：本轮Linux主机微基准，GCC O2、5轮交替次序中位数、每轮200000个正常标签，旧路径81.1021ms、新路径5.17762ms。含返回字符串，未固定CPU频率、未隔离其他负载；仅说明这个格式化函数在当前主机的测量结果，不是游戏帧率、GPU、Windows或HD6000结果。整帧还包含大量几何、文字、地景与渲染开销。

GitHub：独立分支codex/v0.55-instrument-format，源码沿用maintenance/v0.54-glyph-compaction目录（目录名是继承路径，CMake/main已更新0.55）。代码提交16557dbc4affc613830088e00a861f45cd74d2da。核心源码/测试上传；完整资源和第三方仍在ZIP，main未改。Windows检查结果随后补入；不将旧版Windows结果算作本轮结果。

差距：与MSFS2024官方指南介绍的物理、机型系统、航电、天气和职业体验相比，当前仍缺真实机型系统/FMS/EFB、精细机场/高程、复杂天气、经实机数据标定的气动以及完整航班流程。2026-11-02核对 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 。这次优化没有增加这些功能，也不能证明已经接近成品；不承诺5350U/HD6000/8GB同等画质或功能。

下一步：目标Windows完整构建、带资源启动；核对十机型三屏数字和中文排版、通断电与控制操作，再采集固定机场/机型/时刻/画质60秒帧时、RAM与截图。根据实际瓶颈再决定优化几何、图形上传或地景，不以微基准倍数推算游戏FPS。包内无游戏EXE，目标机器与游戏图形尚未验收，继续迭代。
