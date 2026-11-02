最终验证（2026-11-02）：Linux Assimp ON完整Release构建、42/42 CTest与严格便携检查通过；新增ASan/UBSan通过。Windows Server2022/MSVC x64 Release主程序编译链接成功，15/15检查通过，包含任务重置、真实GDI和HLSL；Assimp OFF、资源复制OFF，8条既有警告保留。没有游戏运行/GPU/HD6000或完整航班实机验证。CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/41426107927 ，受检产品提交2ef8c00e743fe05087c9c55526a999ebe2a386bc，之后只补资料。

## v0.58 · 2026-11-02（航班重开状态一致性）
基线：实际读取最新v0.57（文件版本38）、PROGRESS、TODAY_REVIEW、任务/菜单/进近/存档代码。原ZIP CRC及2210项SHA256通过，1661项assets/third_party文件逐字节一致。上轮是新告警显示优先级修复，Linux41/41及Windows14/14均属已记录结果，不作为本轮结果。

当前功能：C++17、DX12/DX11、中文菜单，十机型含A330/A350，共用27控件三维驾驶舱、PFD/NAV/ENGINE、训练导航/几何进近、双机场任务与生涯、浦东离线影像、示意地景、FXAA及网站入口。账号未与游戏存档绑定。没有真实机型专用驾驶舱系统或成品验收证据。

实际缺陷：东海出发的返程任务在重开/换机后，飞机回浦东出生点但任务仍为东海→浦东；旧着陆与综合评分也残留。冻结v0.57头文件运行新增检查，分别在机场对与旧评分断言失败。修复Progress::resetFlight并接入任务选择、换机、重开，按实际出生位置重建机场对，清空本次进度/时间/评分。生涯资产、历史、选定任务和既有事故扣款不变。保留原先回浦东出生点的行为；未增加原地换机、调度或自动驾驶全状态重置。

本轮Linux：Assimp ON完整Release构建276步成功，42/42 CTest通过；严格C++17全套便携检查通过，新增ASan/UBSan通过（detect_leaks=0）。新增400组重置/任务选择、40组旅客任务状态转换及存档往返覆盖。状态由测试构造，不是完整物理试飞，更不是Windows游戏或HD6000实测。日志见validation_v058。

GitHub：codex/v0.58-career-reset，受检产品提交2ef8c00e743fe05087c9c55526a999ebe2a386bc；核心路径继承maintenance/v0.54-glyph-compaction，内部版本0.58。源代码/检查上传；完整资源与依赖在ZIP；main未改。Windows CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/41426107927 ，已完成，最终结果见本页首段。

差距：MSFS2024官方资料描述了全球场景、复杂天气、细化机型系统、航电与职业任务联动；官方EFB还含航线规划、机场图、清单及天气。本项目仍是局部双机场训练世界，缺完整FMS/导航数据库、真实系统深度、精细机场/DEM、复杂天气、气动标定与完整航班验收。参考：https://news.xbox.com/en-us/2024/11/15/microsoft-flight-simulator-2024-creating-career-mode-for-all-players/ 和 https://flightsimulator.zendesk.com/hc/en-us/articles/16959472213916-Electronic-Flight-Bag-EFB-introduction （本轮已检索）。不承诺i5-5350U/HD6000/8GB达到相同画质或功能。

下一步：目标Windows带资源构建启动，按CAREER_RESET.md执行返程重开/换机→保存载入→起飞返落出发地不结算→飞往到达地仅结算一次；同时验收告警、NAV和座舱鼠标操作，采同场景同配置60秒帧时、RAM和截图。没有Windows游戏运行、Boot Camp/HD6000或完整航班实机证据；包内无EXE，未达到成品，继续迭代。
