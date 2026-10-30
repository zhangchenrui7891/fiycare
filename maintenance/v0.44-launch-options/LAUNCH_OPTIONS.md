# v0.44 · 启动参数完整匹配

日期：2026-10-31。基线：完整v0.43（文件版本24），ZIP CRC及1940项SHA256核对通过。

旧main.cpp和graphics.h使用wstring::find：例如--benchmark-old触发诊断运行，--dx11-extra强制DX11，--low=1降低内部渲染分辨率，路径参数中的--warp也会启用软件渲染。本轮只接受经Windows拆分后的完整参数匹配，不把相似字符串当成有效开关。未知参数继续忽略，不新增参数报错流程。

实现：Windows入口使用GetCommandLineW/CommandLineToArgvW解析完整命令行，跳过argv[0]，用RAII调用LocalFree。主程序与两种渲染器统一使用launch::Options::has；基准模式通过独立Options副本添加--low，窗口重建复用该副本。既有--dx11优先级、DX12自动回退/强制失败、正常画质和原存档结构保持。CMake新增shell32链接。参考微软API文档：https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-commandlinetoargvw （2026-10-31核对）。

检查：launch_options_tests.cpp覆盖18个开关、126种误导参数、大小写、重复参数、强制低负载副本；tools/check_launch_integration.py编译实际graphics.h选择函数，使用CPU替代窗口与渲染器，验证相似参数、后端优先级、回退/强制失败及基准/窗口重建路由。它不运行Windows或GPU。

windows_launch_tests.cpp调用真实CommandLineToArgvW，覆盖带空格中文可执行路径、可执行路径排除、引号开关、空参数、重复与相似开关。独立Windows CI只编译这两个新测试和模块，不含main.cpp、DirectX、字体或场景；不能表述为整程序Windows验证。最终结果与链接见PROGRESS.md和validation_v044。

Windows实机验收：先check_release.bat、build_vs.bat。对比--dx11与--dx11-extra、--low与--low=1、--benchmark与--benchmark-old。相似参数应继续普通运行，不进入诊断目录或改变后端/分辨率；精确参数按原行为工作。测试真实基准前备份玩家文件。检查startup.log、目录路由、默认画质60秒帧时/RAM和截图。

现有七机型、27控件共用简化座舱、航电电源、近似气动、训练NAV/几何进近、双机场任务、生涯、离线浦东影像与FXAA保留。与MSFS2024目标相比，完整机型系统/FMS/EFB、精细机场地形、天气和完整航班体验仍不足，没有新的实机画面对比和HD6000数据，不认为接近成品，不承诺同画质/功能。本轮没有视觉或FPS提升声明。

GitHub是本轮变更集与可独立构建的参数模块测试，包含集成补丁；不是完整工程镜像。完整源码、资产和依赖随源码ZIP交付。先前CLI缺凭据的整工程推送阻碍仍未解决；未改旧main。
