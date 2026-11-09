## v0.78 · 2026-11-10 Asia/Shanghai（窗口移动/缩放后的计时恢复）

基线：读取最新可用 v0.77 完整工程、TODAY_REVIEW、PROGRESS 和上轮改进结果，ZIP CRC 与 2452 项 SHA256 通过。上轮旅客航班人数锁定保留，历史 Linux 47/47 与 Windows CI 21/21 检查通过；没有目标机运行证据。沿用实际连续版本，提示的 v0.8 不是本轮确认的工程版本。

当前特点：C++17、DX12/DX11、中文菜单、十机型含 A330/A350、共用 27 控件三维驾驶舱及 PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像和 FXAA。机型系统和气动仍属原型，机场/周边地景简化，网站账号未绑定本地生涯。

本轮实际修复：拖动窗口标题栏、取消缩放或拖回相同尺寸时，旧版未走渲染器重建路径，消息循环的等待时间会混入帧时/benchmark 并触发最多 80ms 物理补算。现在处理 WM_EXITSIZEMOVE，用一次性标志在性能采样前排除该间隔，清空物理余量并恢复输入边界；保留真实 wallElapsed 与普通慢帧。窗口标题、帮助、startup.log、CMake、benchmark 标识同步 v0.78。未增加资源、存档字段或渲染质量开销。

验证：新增实际 main.cpp 语句提取检查，覆盖 12 组 benchmark 开/关、是否重建渲染器、0.001/1/30 秒模拟等待，以及普通慢帧、原生菜单、恢复采样、按键释放重按。v0.77 main.cpp 配新检查可编译但断言失败；新版及 ASan/UBSan 通过。初版检查误用了不存在的 BenchmarkSamples::values，改用公开 storedSamples() 后才完成有效对照；该测试编译错误不作为旧版缺陷证据。Win32 API 为 CPU 替身，不是 Windows 消息/鼠标实测。

本轮测试结果：Linux Assimp ON Release 全量 282 步构建成功，48/48 CTest 通过（16.57 秒），增量构建无待处理任务。严格 C++17 便携检查退出 0；相关 ASan/UBSan 检查退出 0（detect_leaks=0）。Windows 2022/MSVC x64 Release 主程序编译成功，21/21 CPU/GDI/HLSL 检查通过（1.39 秒），8 条既有警告；Assimp OFF、资源复制 OFF，没有游戏/GPU执行。CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42661704767 。日志见 validation_v078。

GitHub：分支 codex/v0.78-window-timing；受检代码 c0d2d101dcd1db95706918325584b16cf66fcb84；目录 maintenance/v0.54-glyph-compaction。最终记录提交只增加文档/证据，默认 main 未改。完整源码资源 ZIP 无 EXE；1661 项资源及第三方文件逐项比对未改。

与目标差距：微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 描述全球世界、进阶物理/航电/天气及动态生涯。本项目仍缺真实机型系统/FMS、经标定的气动、精细机场/DEM、复杂天气和完整航班实机证据。没有可核查的实机画面对比、HD6000 FPS/RAM/温度数据，不能判断接近成品。不会承诺 i5-5350U/HD6000/8GB/256GB 达到 MSFS2024 同等画质或功能。

下一步：按 WINDOW_MODAL_TIMING.md 验收带资源 Windows 构建，检查标题栏移动、取消缩放、DX11/可用 DX12、后台恢复及 benchmark 数据；同时完成上轮登机锁定到落地结算的完整航班验收。仍无目标 Boot Camp 设备连接，所有上述实机步骤未执行。继续迭代，不认定成品、不停止每日任务。
