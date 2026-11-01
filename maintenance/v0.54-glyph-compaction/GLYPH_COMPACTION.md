
本轮Windows最终证据：2026-11-01 GitHub Actions run41315044625成功，Windows2022/MSVC主程序编译链接成功，11/11 CPU检查通过（含真实GDI像素覆盖/字距、HLSL编译）。Assimp OFF、资源复制OFF，没有运行游戏。日志有8条MSVC警告，未宣称零警告。结果：https://github.com/zhangchenrui7891/fiycare/actions/runs/41315044625 。最终代码与受检提交420162c5f9cd1afd7cd611ebcabd912a4caf73f2一致；后续仅补文档证据。

## v0.54 · 2026-11-01（字体几何合并）
基线：读取v0.53（文件版本34），ZIP CRC及2151项SHA256通过；十机型、27控件共用座舱、PFD/NAV/ENGINE三屏、训练导航/进近、生涯、示意地景、浦东影像、FXAA和账号网站入口保留。网站不绑定游戏存档；本轮未更改站点发布状态，不能确认所有地区连通。

实际改动：新增glyph_runs.h，在GDI字形首次缓存时合并纵向相邻且x/宽度相同的矩形。保留覆盖像素、advance、阈值、颜色与排版；不在逐帧路径执行合并。原有HUD和cockpitText都读取同一缓存，减少可合并字形的矩形/顶点生成与上传量。没有测量真实字体整体降幅、CPU帧时、GPU或FPS；不能把合成数据视为HD6000收益。

本轮Linux：Assimp开启完整Release构建通过；最终38/38 CTest通过（首次37通过，1个测试二进制权限拒绝未运行，删除该二进制并重新链接后全量复验通过）。严格C++17便携回归通过；新算法ASan/UBSan通过（detect_leaks=0）。3000组随机掩码覆盖一致且无重叠，空白/满填充、间隙、宽度变化、负bearing与幂等检查通过。合成矩形总数539602降至494983；13行竖笔画78顶点降至6，仅为夹具示例。早期便携脚本运行期间被编辑，读取中断，最终固定版本重新运行退出0；保留原日志。资源与第三方文件逐字节不变。

Windows：增加真实GDI像素/字距对照检查及主程序编译工作流；本段写入时尚无本轮结果。游戏画面、GDI在目标机器的可读性、键鼠、HD6000帧时/RAM、资源启动均未实测；包内无游戏EXE。主程序Windows检查采用Assimp OFF/复制资源OFF，若通过也不代表默认完整构建或游戏运行。

GitHub：核心源码/工具/测试上传独立分支codex/v0.54-glyph-compaction，代码提交420162c5f9cd1afd7cd611ebcabd912a4caf73f2。目录maintenance/v0.54-glyph-compaction。运行资源及完整第三方镜像仍由ZIP提供；main保持原样。

差距：真实机型专属系统/FMS/EFB、精细机场与真实高程、复杂天气、经标定气动和完整航班流程均仍不足。2026-11-01核对MSFS2024官方指南：https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，其强调物理、航电、天气和职业体验。本轮优化不代表这些体验已达到目标，不承诺5350U/HD6000/8GB同等画质功能。

下一步：Windows运行check_release.bat和build_vs.bat，先核对三屏与中文菜单的文字边缘/字距，再对比同机场、机型、画质60秒帧时、内存和截图。只有取得这些证据后，才能判断文字几何是否为实际瓶颈并继续调整。尚不具备成品验收依据，继续迭代。

