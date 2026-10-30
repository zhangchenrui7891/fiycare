# v0.43 · 帧时仪表计算

日期：2026-10-31。基线：v0.42 恢复版（文件版本23）。

main.cpp每帧调用FrameWindow::summary，供HUD与CSV使用。旧实现完整排序最多300个样本；新实现使用nth_element选出nearest-rank P95，并在原求和遍历中求最大值。继续复制固定数组，不修改环形数据；保留FPS求和顺序、P95排名、最长卡顿、样本数、过期和重置行为。不改变60秒全程基准BenchmarkSamples、渲染或画质。

performance_window_tests.cpp以冻结v0.42算法为独立参考，进行85002次精确字段比较：覆盖1–300个样本、连续环形覆盖、重复查询、重置、等值/递增/递减/随机/极端有限数、无效输入与长卡顿过期。CTest和严格便携入口均注册该检查；不是Windows主程序或GPU测试。

CPU探针源码：tools/probe_performance_window.cpp。g++ -std=c++17 -O2编译后执行。五轮各200000次add+summary、确定性随机帧时：旧1460–1492ms，新282–311ms。只证明本环境此函数的合成工作量变化，单次约节省6微秒，不能推断整体FPS、HD6000性能或Windows标准库表现，不能表述成游戏快了五倍。探针顺序与其他构建负载也影响计时。

Windows验收：先备份玩家文件，在新目录check_release.bat、build_vs.bat；相同机场/机型/时刻检查HUD和CSV。对比v0.42与本版同画质60秒帧时和进程RAM，并保留startup.log、截图。低负载run_benchmark_dx11.bat的数据单列，不当作默认画质性能。

七机型、27控件共用简化座舱、航电电源、训练NAV/几何进近、双机场任务、近似气动、生涯、离线底图、FXAA保留。真实机型系统/FMS/EFB、精细机场、复杂天气与完整航班仍有明显缺口；没有新的实机画面对比，不认为接近MSFS2024成品，也不承诺HD6000达到同等画质或功能。

最终构建结果见validation_v043与PROGRESS.md最新条目。v0.42缺失历史检查和旧日志的范围继续见RECOVERY_V042.md，不重新声称已恢复。
