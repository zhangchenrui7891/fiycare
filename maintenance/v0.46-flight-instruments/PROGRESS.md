## v0.46 · 2026-10-31（主仪表单位与小幅升降读数）

基线：实际读取最新v0.45文件版本26，ZIP CRC及1982项清单SHA256通过；读取PROGRESS、上轮Windows源码编译/HLSL与Unicode记录和恢复边界。本轮优先改进座舱仪表可读性，不再仅增加构建基础设施。

实际改动：三维PFD增加IAS km/h、ALT m、VS m/s单位；VS保留一位小数及正负号，+0.4/-0.4m/s不再取整为0。新增flight_instruments.h，统一三维主航向、导航显示、AP目标和外部HUD飞机航向为000–359，避免北向HUD四舍五入成360；非法值显示---、消除负零。物理/存档/气压/导航控制律和公制单位未改，PFD坐标/宽度与航电隐藏条件保留。任务航点方位显示未改，仍是几何航向，不是磁航向或完整FMS。

本轮Linux：默认Assimp开启的完整Release构建256步、27/27 CTest、严格C++17全套通过，增量ninja无工作。新增格式检查40001个航向范围用例及基准方向/近北边界/单位/小幅VS/无效值/正负零检查；新增ASan/UBSan（detect_leaks=0）通过。初轮CTest标准输出末尾缺摘要，LastTest记录通过；再执行一次获得完整ctest_final.log及JUnit：27测试、0失败、0跳过。原始不完整输出保留，没有把它冒充完整日志。

本轮Windows：Windows Server2022、MSVC19.44、x64 Release主程序编译链接成功，6/6检查通过。新增真实GDI检查90行代表性PFD文本的字形几何均落在300×21逻辑文字范围；不是整屏/三维透视截图，目标机字体可替代，仍需真实可读性验收。成功运行：https://github.com/zhangchenrui7891/fiycare/actions/runs/41183414666 ，源码提交c6e789c4db51ff2b0096683c5fcd80b8c414f8a2。既有Unicode/命令行/HLSL检查保留，仍有7条既有MSVC警告。Windows关闭Assimp和资源复制，没有运行游戏或测量GPU/HD6000性能。

GitHub独立分支codex/v0.46-flight-instruments上传84份核心源码/构建文件、CI和本轮资料；不是二进制资源/Assimp齐全的完整镜像。完整ZIP保留全部资源和依赖。模型/贴图/第三方文件逐字节不变。Windows已验证产品源码与最终资料提交一致，最终链接见交付说明。

当前七机型、27控件、电源/航电、近似气动、训练NAV/几何进近、双机场任务、生涯、浦东影像和FXAA保留。真实机型专属系统/FMS/EFB、精细机场、复杂天气、实机气动标定及完整航班仍有明显差距。2026-10-31重新核对官方新玩家指南（https://www.flightsimulator.com/guide/new-flight-simmer-guide/，2026-01-05）作为物理/航电/天气/生涯参考，没有新增可比较的成品画面或性能证据，未接近MSFS2024，不承诺i5-5350U/HD6000/8GB同等画质/功能。

下一步：目标Windows默认Assimp开启构建，实际通断电/27控件/新仪表文字和北向显示验收，再采默认画质同场景60秒帧时、RAM及截图。完整运行/图形/HD6000性能仍待验证，源码包无游戏EXE。未宣称FPS提高或成品完成。

