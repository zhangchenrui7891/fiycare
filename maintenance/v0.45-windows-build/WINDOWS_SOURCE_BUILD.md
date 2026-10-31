# v0.45 · Windows主程序源码编译与显卡名称日志

日期：2026-10-31。基线：v0.44文件版本25，ZIP CRC与1963项SHA256通过。

## 本轮改进

新增tools/check_windows_source.ps1及Windows源码CI。原CMake新增FCAIR_COPY_RUNTIME_ASSETS开关，默认ON，正常构建依然复制资源。仅源码检查显式关闭Assimp和资源复制，编译真实src/main.cpp、DX11/DX12类和全部所需头文件，并执行真实HLSL编译及Windows模块检查。没有伪造运行资产、没有启动游戏。

首次Windows构建成功并产生FlightCareerAir.exe，3项测试通过。编译器同时报告DX11/12显卡名称以wstring迭代器构造string，会把非ASCII名称截成单字节。本轮新增windows_text.h的Windows API UTF-16→UTF-8转换，接入两后端日志，先查询输出字节数再分配；空串直接返回、非法UTF-16给出明确占位、API长度超限抛异常。没有改变设备选择或图形设置。

新增windows_text_tests.cpp：ASCII、中文、非BMP字符、嵌入NUL、空串和孤立高/低代理项共7例，直接调用Windows API。将其纳入主工程Windows CTest和源码检查脚本。API参考：https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte （2026-10-31核对）。

## 验证边界

Windows检查使用Windows Server2022、MSVC、x64 Release，FCAIR_WITH_ASSIMP=OFF、FCAIR_COPY_RUNTIME_ASSETS=OFF。编译包含主循环、中文GDI绘制代码、座舱、原生fcmesh加载、DX11与DX12渲染器，但不包括Assimp外部GLB/FBX导入实现，也不复制模型/贴图。不是默认Assimp开启的整套Windows发行构建，不能直接当作可运行成品。

HLSL测试用D3DCompiler在CPU上编译四个SM5着色器并验证80次缓存命中，不创建GPU、不输出画面。Unicode测试不读取真实显卡；只证明转换函数和接线已编译、API用例正确。

Linux仍按默认Assimp开启的完整工程构建和26项CTest验证。Linux不会编译main.cpp/DirectX/Windows转换函数；Windows条件代码以真实Windows结果为准。本轮未新增ASan/UBSan证据，不把上轮结果算成本轮新证据。构建日志及最终结果见validation_v045与PROGRESS.md。

仍有7条既有MSVC /W4警告：D3D12局部footprint遮蔽成员及菜单/摄像机int→float转换。本轮不声称所有警告已清零。实际数量以最终日志为准。

## 本机验收

备份玩家目录。先check_release.bat，再build_vs.bat进行默认Assimp开启且带资源复制的完整构建。若只需要复现源码检查，可从Developer PowerShell运行tools/check_windows_source.ps1，但它不复制资源，也不启动游戏。

在真正运行DX11/DX12后核对startup.log的显卡名、中文菜单与27个座舱控件，再采同机型/机场/时刻默认画质60秒帧时、RAM和截图。云端CPU编译/HLSL通过不证明HD6000可玩或FPS提高。

## 当前目标差距

七机型、27控件共用座舱、航电、近似气动、训练NAV/几何进近、双机场任务、生涯、浦东离线影像与FXAA保留。真实机型系统/FMS/EFB、精细机场地形、复杂天气、气动实机标定及完整航班验收仍不足。MSFS官方指南作为物理、航电、天气与生涯方向参考：https://www.flightsimulator.com/guide/new-flight-simmer-guide/ （2026-10-31核对）。没有新的实机画面对比或HD6000性能证据，未接近成品，不承诺目标设备同等画质/功能。

GitHub分支codex/v0.45-windows-build含可编译主程序的源码快照和本轮变更、检查、日志；未包含运行资源或Assimp依赖，不是完整镜像。完整ZIP保留全部资源、依赖和历史记录，源码包不含游戏EXE。
