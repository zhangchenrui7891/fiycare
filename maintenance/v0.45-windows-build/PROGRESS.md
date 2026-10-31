## v0.45 · 2026-10-31（Windows主程序编译与显卡日志）

基线：最新v0.44文件版本25，ZIP CRC与1963项SHA256核验通过；读取连续记录、上轮Windows参数模块证据及恢复边界。保留七机型、27控件共用座舱、航电、训练NAV/几何进近、双机场任务、近似气动、生涯、浦东离线影像与原画质/FXAA配置。

本轮优先补齐主程序Windows构建证据。新增check_windows_source.ps1及独立Windows CI，使用原主工程CMake、真实main.cpp与所有核心头文件。新增FCAIR_COPY_RUNTIME_ASSETS选项，默认ON；仅源码CI显式关闭Assimp和资源复制。正常用户构建默认仍开启Assimp并复制资源，未降级默认功能。

首次Windows源构建成功、3项检查通过，MSVC警告发现DX11/12把显卡名称UTF-16字符直接缩成char。本轮实际修复两后端日志：通过windows_text.h统一Windows API UTF-8转换，按输出字节数分配，明确处理非法代理项；新Windows测试7例覆盖空串、ASCII、中文、非BMP、嵌入NUL及孤立高/低代理项。不改变设备选择或画质，不声明FPS收益。

最终Windows实测：Windows Server2022、MSVC19.44、x64 Release，主程序成功编译链接；4/4 CTest通过（Unicode、完整参数、Windows命令行解析、真实HLSL）。HLSL在CPU上编译4个SM5程序并验证80次缓存命中；没有GPU。成功运行：https://github.com/zhangchenrui7891/fiycare/actions/runs/41162739903 ，源码提交25a66c6ee50f72ec4e44c6336d3124ee3feb282e。两个设备名日志截断的模板警告消失；仍保留7条既有/W4警告（footprint遮蔽和菜单/摄像机int→float），没有虚称零警告。初次运行41162546641及最终编译/测试日志节选保留在validation_v045。

明确范围：Windows构建FCAIR_WITH_ASSIMP=OFF、FCAIR_COPY_RUNTIME_ASSETS=OFF。编译了主程序、DX11/DX12、原生fcmesh读取、GDI文字/座舱代码，但未编译Assimp外部格式导入，也没有运行游戏或提供资源完整的发行包。不能称为默认Assimp开启的完整Windows发行构建，更不能称为图形或HD6000实测。源码包不含游戏EXE。

本轮Linux：默认Assimp与资源复制选项均ON，完整Release构建254步、26/26 CTest、严格C++17全套通过；Unicode修改后再次构建/26项复验通过，最终增量ninja无工作。本轮未新增ASan/UBSan记录，不把上轮证据重新计数。所有模型/贴图/第三方文件与v0.44逐字节相同。

GitHub：codex/v0.45-windows-build已上传核心源码快照，可在不下载二进制资源/Assimp依赖的情况下复现本轮Windows编译；另附本轮修复、脚本、补丁、说明、日志及源码哈希。不是资源/依赖齐全的完整镜像；完整ZIP继续包含全部资源和依赖。旧main不变。最终资料提交以交付链接为准，已验证产品源码与25a66c6一致。

对标与缺口：2026-10-31核对MSFS官方2026-01-05新玩家指南（https://www.flightsimulator.com/guide/new-flight-simmer-guide/），其物理/航电/天气/生涯方向作为长期参考。本项目真实机型系统/FMS/EFB、精细机场/地形、复杂天气、气动标定和完整航班仍不足，没有可比较的实际画面和HD6000帧时/RAM证据，未接近目标，不承诺5350U/HD6000/8GB同等画质/功能。

下一步：在目标Windows运行check_release.bat、build_vs.bat验证默认Assimp开启构建；优先处理实际启动/图形反馈，核对两个后端startup.log显卡名、27控件、中文字体，再取得同机型/机场/时刻默认画质60秒帧时、RAM与截图。无实机证据前不加重渲染负担，不宣告成品。

