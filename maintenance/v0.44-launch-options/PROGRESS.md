## v0.44 · 2026-10-31（启动参数完整匹配）

基线：读取最新v0.43文件版本24，ZIP CRC与1940项SHA256全部通过。读取上轮PROGRESS、LAUNCH/PERFORMANCE相关代码、v0.42恢复边界及v0.43测试日志。已知七机型、27控件座舱、航电、训练导航/几何进近、双机场任务、生涯、近似气动与原有FXAA/画质配置保留。

实际问题：wstring::find把--benchmark-old、--dx11-extra、--low=1及路径中的--warp当成有效开关，可能误切换诊断/后端/分辨率。修复：Windows入口通过GetCommandLineW/CommandLineToArgvW拆分并排除可执行文件名，RAII释放API内存；主程序和graphics.h统一完整参数匹配。基准显式拷贝并添加--low，重建窗口沿用；保留后端回退与--dx11优先级。增加shell32链接，版本更新v0.44。无FPS/画质改善声明。

本轮Linux：Assimp开启的完整Release构建254步通过，先25/25再纳入新增渲染选择集成后26/26 CTest通过，增量ninja无工作；严格C++17全套及新增launch测试通过。18个开关/126种误导参数、拷贝/强制/重复与大小写检查；真实graphics.h函数以CPU窗口/渲染器替代验证误导参数、后端优先级、DX12回退与强制失败、基准/重建路由。新单元与该集成ASan/UBSan（detect_leaks=0）通过。不是GPU/真实鼠标实测。

新增真实Windows证据：独立分支的参数模块在Windows Server2022、MSVC19.44下以/W4 /WX /utf-8 /EHsc编译；2/2 CTest通过，实际调用CommandLineToArgvW验证引号、空白/空参数、中文带空格路径、argv[0]排除、重复和相似参数。成功运行：https://github.com/zhangchenrui7891/fiycare/actions/runs/41135044245 ，测试源码提交d2bf7a1f09656d17a94cb938168e4bd6659fd07b。仅两个新模块测试，不含main.cpp、DirectX或完整程序，不能称为整程序Windows验证。原始编译/CTest日志节选与摘要在validation_v044。

GitHub已通过接口在codex/v0.44-launch-options上传新模块及独立Windows CI，继续补入实际主程序/graphics集成改动、与v0.43的补丁、检查、说明和日志。分支只含本轮变更集和独立模块测试，不是完整工程镜像；资源/依赖随完整ZIP提供。旧main不变，先前CLI凭据阻碍未解决。

缺陷与对标：真实机型系统/FMS/EFB、精细机场地形、复杂天气、实机气动校准与完整航班仍不足。2026-10-31重新读取MSFS官方新飞行员指南（https://www.flightsimulator.com/guide/new-flight-simmer-guide/，页面2026-01-05）作为物理/航电/天气/生涯方向参考；本项目尚未提供可比实机画面及性能证据，未接近成品，不承诺i5-5350U/HD6000/8GB同等画质/功能。

未验证：整个Windows主程序构建与启动、DX11/DX12画面、GDI中文、键鼠/座舱、HD6000默认画质60秒帧时和RAM。源码包无游戏EXE。下一步：check_release.bat、build_vs.bat，验证精确与相似参数启动路由，再验收27控件并采固定场景默认画质60秒帧时/RAM/截图。继续开发，不停止迭代。

