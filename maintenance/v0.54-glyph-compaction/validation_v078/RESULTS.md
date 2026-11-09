# v0.78 检查证据

- 基线 v0.77：CRC 通过，2452 项 SHA256 一致。
- Linux：CMake/Ninja Release、Assimp ON，全量 282 步；48/48 CTest，16.57 秒；增量无工作。
- 严格便携 C++17 检查：退出 0。
- 新计时回归：12 组模拟边界检查及普通慢帧/原生菜单/键盘恢复；正常及 ASan/UBSan 退出 0，detect_leaks=0。
- 旧 main.cpp：编译成功、运行断言失败，见 legacy_regression.log；不是编译失败冒充回归复现。
- 初写测试误用私有不存在字段 values；修正为公开 storedSamples() 后执行对照。未改产品逻辑以迎合该测试错误。
- 1661 个 assets/third_party 文件与基线一致。
- Windows 2022 / MSVC x64 Release 主程序编译成功；21/21 CPU/GDI/HLSL，1.39 秒，8 条既有警告。
- Windows CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42661704767
- CI job：129314669632；受检代码 c0d2d101dcd1db95706918325584b16cf66fcb84。
- Windows 构建 Assimp OFF、资源复制 OFF；没有应用、GPU、Boot Camp/HD6000 运行。
- Linux 输出不是 Windows 游戏实测；模拟等待并不是测得的 FPS/功耗改善。

重跑：cmake -S . -B out/check -G Ninja -DCMAKE_BUILD_TYPE=Release；cmake --build out/check；ctest --test-dir out/check --output-on-failure。
便携：bash tools/check_portable.sh /tmp/fcair-portable。
定向：python3 tools/check_window_timing.py。
消毒器：FCAIR_SANITIZE=1 ASAN_OPTIONS=detect_leaks=0 python3 tools/check_window_timing.py。
Windows：powershell -File tools/check_windows_source.ps1。
