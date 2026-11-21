# v0.101 Windows x64 兼容构建交付

此前完整包仅提供源码和资源，需要本机安装 VS 后编译。本版交付流程在 Windows CI 生成 Release x64 FlightCareerAir.exe，并记录精确源码提交、EXE SHA256、构建配置和 PE 依赖列表。完整包将该 EXE 放在根目录，与 assets 相邻；资源内容沿用 v0.100。解压完整目录后运行 run_compat.bat，使用 DX11 和低分辨率设置。EXE 需要 Windows 10 x64 及支持 DX11 的驱动；HD6000 实际启动和性能尚待目标机验收。

兼容 EXE 使用 Assimp OFF，因此 GLB/GLTF/OBJ 等多格式导入与旋转编辑不可用；已有原生 fcmesh 模型、11 机型、DX11/DX12、中文菜单、驾驶舱、航电、导航、生涯、地景与 FXAA 仍编译在主程序内。完整源码和第三方资源保留，用户可以使用 build_vs.bat 构建 Assimp ON 版本。

CI 的 Release 配置使用 CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded（/MT），并在导出前检查动态依赖中不存在 MSVCP/VCRUNTIME/CONCRT/UCRTBASE DLL。此检查减少外部分发依赖，不证明系统 DLL、驱动或图形功能在目标机可用。依据 CMake 官方说明：https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html 。

运行入口通过 tools/resolve_flight_exe.bat 统一查找：优先 out/build/release 下的本机重建版本，然后回退包根目录 EXE；都不存在时原入口报告缺失。兼容、高画质、工程、DX11 基准、777 FXAA 对照及图形/夜间点检入口共用此规则，避免重新编译后继续误用旧包内 EXE。build_vs.bat 完成编译后仍直接启动新生成物。

Windows 脚本测试在带空格的临时目录中执行真实 cmd resolver，覆盖缺失、包内回退、本地重建优先与删除重建后的回退。测试占位文件不会作为程序执行。Windows CTest 运行现有 25 项 CPU/GDI/HLSL 检查，Linux 继续使用 Assimp ON 构建并运行检查。CI 没有把模拟器启动为图形应用，PE 结构/哈希/依赖与 CPU 检查不能代替 Windows 游戏、DX11/DX12、Boot Camp 或 HD6000 实测。

交付包内 build_info.json、dependencies.txt 和 validation_v101 记录实际二进制来源与检查结果。EXE 尚未作代码签名。下一步在目标机运行 run_compat.bat，保存 startup.log，完成座舱和菜单点检，再运行 run_benchmark_dx11.bat 采集 P95、峰值工作集和截图；仍需完整起飞、航线、进近与落地验收。距离 MSFS2024 的成熟航电/FMS、真实机场数据库、天气、气动标定与成品体验仍有明显差距。
