最终结果（2026-11-03）：Linux Assimp ON Release构建与最终44/44 CTest通过，损坏的asset_fixture_check重新链接后通过；全套严格C++17便携检查及新增ASan/UBSan通过（detect_leaks=0）。Windows2022/MSVC主程序编译链接成功，17/17 CPU检查通过（含GDI/HLSL）；Assimp OFF、资源复制OFF，8条既有警告。没有Windows游戏、GPU帧时或HD6000实测。Windows证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/41587187218 。受检产品代码ec8d4afd013677c3924b34ba7fab05285d5b253b，之后仅补资料。

环境修正：Linux初轮asset_fixture_check缺少执行权限而无法运行（BAD_COMMAND）；恢复权限后该文件仍无法执行（file识别为data，shell语法错误），因此删除损坏的构建产物并重新链接，再重跑完整测试。初轮及损坏文件失败日志均保留，不把无法执行计为通过。

## v0.62 · 2026-11-03（渲染配置与性能记录）
基线：读取最新v0.61（文件版本42）、PROGRESS/TODAY_REVIEW与渲染/统计代码。CRC及2264项哈希通过，1661项资源/第三方文件逐字节一致。上轮将默认比例从2改为1，44/44 Linux与17/17 Windows为上轮证据，未证明HD6000帧率提升。

当前特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维驾驶舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、示意地景、FXAA和网站入口。账号未绑定游戏存档，本轮未改变站点公开设置。

问题：performance.csv的low_mode错误记录powerSaver，benchmark固定640×360但不限帧会被标成非low；CSV也没有原生/超采样尺寸证据。本轮修复low_mode语义，追加render_width/render_height/software_frame_cap。创建渲染器和记录共用尺寸函数，CSV使用已应用比例而非待生效菜单请求。每5秒仅多写三个数字，没有新资产或逐帧采样任务；不宣称FPS变化。

验证：新增脚本执行实际main CSV语句，16组配置组合通过；旧版在新增表头断言失败。共享尺寸测试及原有DX11/DX12创建/回退替身检查通过；全套严格C++17便携检查及新增ASan/UBSan通过（detect_leaks=0）。完整构建与Windows结果见首段。测试均不等于真实窗口、GPU或HD6000。

GitHub：codex/v0.62-render-metadata，受检代码ec8d4afd013677c3924b34ba7fab05285d5b253b；继承目录maintenance/v0.54-glyph-compaction，内部版本0.62；main未改，完整资源和依赖在ZIP。

兼容与限制：CSV追加三列，解析器需读取表头；low_mode历史含义改变，旧日志不能按新语义解释。现有统计仍为CPU墙钟/300帧滚动窗口，非GPU帧时；切换FXAA/机型等可能混合样本，需稳定配置后等待窗口刷新。本轮不增加内存采样、自动原生benchmark或实际性能数据。

差距：2026-11-03检索微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，其物理、航电、天气和职业体验仍为参考；当前缺真实机型系统/FMS、精细机场/DEM、复杂天气、气动标定及完整航班实机验收。没有接近成品的画面或性能证据，不承诺5350U/HD6000/8GB达到相同画质功能。

下一步：带资源Windows启动，固定机场/机型/视角/FXAA后分别测原生与超采样60秒帧时、内存和截图，并核对新增CSV列；原生1600×900应记录1600/900，2×为3200/1800，low/benchmark为640/360。包内无EXE，未运行Windows游戏或测HD6000，继续迭代。
