## v0.48 · 2026-11-01（Assimp开启的Windows构建与导入验证）

基线：实际读取最新v0.47文件版本28，ZIP CRC与2023项清单SHA256通过，读取上轮AP通知、Windows Assimp关闭编译、仪表/GDI和恢复边界。七机型、27控件共用座舱、航电、近似气动、训练NAV/几何进近、双机场任务、生涯、浦东离线底图、FXAA及上轮接管反馈保留。

本轮高优先级改进是补齐Assimp开启的Windows源码构建。新增check_windows_importer.ps1、固定依赖恢复工具/清单/原覆盖文件和CI，CMake新增自包含实际导入检查目标及恢复工具检查。模型导入实现原已使用u8path，完整阅读后纠正了最初误判，没有重复修复既有Unicode代码；src/asset_import.cpp及其余运行逻辑/资源/原依赖均逐字节保留，产品源码仅main版本文字更新，游戏玩法和画质没有新增。

依赖验证：现有Assimp5.4.3固定上游提交c35200e38ea8f058812b83de2ef32c6093b0ece2，源码包SHA256=66dfbaee288f2bc43172440a55d0235dfc7bf885dda6435c038e8000e79582cb，与PROVENANCE一致。1628项与官方包逐字节相同，另外两项是原CMake修补和来源记录。恢复脚本复制且校验完整1630项，拒绝覆盖本地修改、未知文件、不安全路径/归档链接，暂存树完成后替换；完整源码包不下载、不改写依赖。真实固定包恢复到新目录逐项通过，12项离线故障/保留测试通过。

本轮Windows实际结果：Windows Server2022、MSVC19.44、x64 Release、FCAIR_WITH_ASSIMP=ON、FCAIR_COPY_RUNTIME_ASSETS=OFF，Assimp/zlib、真实asset_import.cpp静态库、主程序FlightCareerAir.exe和asset_import.exe均成功编译链接，10/10 CTest通过。新自包含样例实际导入ASCII及中文目录/模型/MTL/外部PPM，验证贴图解码、RGB顶点烘焙、中文路径fcmesh读回、关闭烘焙与无效缩放拒绝。原AP/仪表/Unicode/命令行/HLSL/GDI检查保留。成功运行：https://github.com/zhangchenrui7891/fiycare/actions/runs/41228015869 ，测试源码23641d298f230b860e2fbf6573dd539dc9b19683。仍有7条既有MSVC警告；没有虚称零警告。

本轮Linux：Assimp开启的完整源码构建、最终30/30 CTest、JUnit零失败零跳过、严格C++17全套通过；最终增量无工作。初次测试夹具使用退化UV，颜色检查失败，改为非退化UV后真实烘焙通过；未修改导入器掩盖差异。两个旧二进制另因本地执行权限未启动，恢复权限后最终全套通过。保留初始/修正日志。本轮没有新增ASan/UBSan记录，不把上轮结果计作本轮。构建与验证证据见validation_v048。

GitHub：codex/v0.48-assimp-build包含核心源码、自包含检查、固定依赖清单/原覆盖文件/恢复工具及CI，依赖可按同一官方包与哈希完整恢复；不上传运行资源，仍非完整发行镜像。完整ZIP带全部原资源与依赖，原有功能/存档/控制律不变。最终资料提交不修改已验证产品源码。

验证边界：资源复制关闭，未启动游戏或创建真实GPU画面，未验证HD6000驱动/帧时/RAM、真实键鼠和完整航班。合成OBJ/PPM不等同于所有复杂GLB/FBX/纹理/动画均正确，机模不提供真实航电或气动系统。默认带资源构建与整程序启动仍需目标机验收。源码包无游戏EXE，没有宣称FPS或画质提高。

差距：完整机型系统/FMS/EFB、精细机场地形、复杂天气、气动实机标定及完整航班仍不足。2026-11-01核对MSFS官方新玩家指南（https://www.flightsimulator.com/guide/new-flight-simmer-guide/，2026-01-05）及Assimp官方5.4.3发布来源。没有新增可比较的成品画面或HD6000性能证据，未接近目标，不承诺5350U/HD6000/8GB同等画质/功能。

下一步：目标Windows先check_release.bat，再build_vs.bat完成带资源的默认构建；验收实际机模/贴图/27控件/中文与AP提示，采默认画质固定机场/机型/时刻60秒帧时、RAM和截图。继续迭代，不宣告成品。

