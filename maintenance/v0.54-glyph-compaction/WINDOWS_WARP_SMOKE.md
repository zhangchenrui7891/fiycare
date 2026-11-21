# v0.102 Windows DX11 WARP 启动检查

v0.101 交付了真实 Windows EXE，但验证范围仍停留在编译、CPU/GDI/HLSL、启动路径与 PE 依赖。本版新增执行真实 FlightCareerAir.exe 的 Windows CI 启动检查，以获取程序启动、渲染读回、诊断存储和退出行为的证据。

测试将当前构建复制到独立 fixture-runtime 目录，仅生成 64×64 棋盘纹理 atlas.fctex，选择程序生成训练机 0。明确使用 --dx11 --warp --low --smoke-test，不使用正式卫星图/模型资源，也不使用目标 GPU。追尾和座舱各绘制三帧，要求程序返回 0、记录正常关闭，生成一张有效 640×360、32 位 BMP 且采样颜色不为空白。两个结果图名以 CI_FIXTURE_WARP 开头；它们是实际 Windows 软件渲染读回，不是正式资源画面对比、HD6000 截图或性能基准。

每个子进程使用隔离的 LOCALAPPDATA，建立 state.txt/state.bak/career.txt/tracks.txt 哨兵，要求诊断过程不修改它们。第三个用例移除必需 atlas，要求退出码 1 和明确日志。为避免自动诊断卡在错误框，新开关 --diagnostic-no-dialog 仅在 --smoke-test/--benchmark 模式有效；普通启动仍保留可见错误框。精确开关解析和诊断门控由 C++ 测试覆盖。

Windows 工作流先完成静态运行库兼容 EXE 和 25 项 CPU/GDI/HLSL检查，再运行 WARP 启动检查；仅通过后才把 game_executed=true 及具体 fixture 环境写入 build_info.json。gpu_validated 和 target_hardware_validated 保持 false；smoke_summary.json 明确 full_assets_validated=false、performance_verdict=NOT_EVALUATED。保留二进制哈希、日志、截图与检查来源。

此检查增加持续集成覆盖，不改变正常飞行、画面设置、存档格式或物理。完整兼容包继续提供正式资源和新构建 EXE，Assimp 导入/旋转编辑关闭，完整源码仍可重建 Assimp ON。目标电脑仍需执行 run_compat.bat，点检正式资源、中文菜单与座舱，然后采集 DX11 P95/工作集，并完成起飞、航线、进近和落地验收。与 MSFS2024 的专用航电、真实机场、天气和成品体验仍有明显差距。

## 本轮结果

工作流 44911844676、源码 54a2581cd27cb1f6a24db32a4685094a1721849b 成功完成上述三种实际执行情形，以及 Windows 25/25 检查和四种 cmd 路径测试。两张 640×360 BMP 哈希、二进制哈希、PE 结构与来源已独立核对；证据在 validation_v102，包括原始 CI_FIXTURE_WARP_EVIDENCE.zip。实际软件截图显示仪表偏小、底部提示条遮挡部分仪表，需要继续改善低分辨率可读性。无正式资源图形、硬件 GPU/HD6000 性能或完整航班结论。
