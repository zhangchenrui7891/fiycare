最终结果（2026-11-03）：Linux Assimp ON完整Release构建和44/44 CTest通过；严格C++17便携检查及新增ASan/UBSan通过。Windows2022/MSVC x64 Release主程序编译链接成功，17/17检查通过，含模型恢复、GDI和HLSL；Assimp OFF、资源复制OFF，8条既有警告保留。未运行游戏、GPU或HD6000。Windows证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/41516530040 。受检提交282a271eb58376f3438afe390a35960275215afe，后续仅补记录。

本轮验证修正：Linux初轮3个旧测试因可执行权限丢失无法启动，恢复权限后完整重跑44/44通过；Windows初轮新增测试缺少显式<string>头文件，已补齐并重跑。初轮失败日志保留于validation_v060，未作为功能通过证据。

## v0.60 · 2026-11-03（默认机模恢复失败保护）
基线：实际读取v0.59（文件版本40）、PROGRESS/TODAY_REVIEW及模型加载/菜单代码。原ZIP CRC与2234项哈希通过，1661项assets/third_party逐字节一致。上轮是HDG接管NAV修复，Linux43/43和Windows16/16属于上轮证据。

当前功能：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维驾驶舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东影像、示意地景、FXAA与账号网站入口。账号未绑定游戏存档。本轮不改变网站公开设置。

问题：恢复默认机模时若默认文件缺失/损坏，异常逃出菜单、进入程序顶层错误处理，结束游戏。改动：model_restore.h先读取校验再提交，main的ResetModel边界捕获失败并显示中文提示，保留当前模型、自定义标记与文件；正常恢复及删除失败提示保留。没有改变启动前资源完整性要求，缺资源自动下载尚未实现。不增加持续每帧工作或资源，不宣称帧率提升。

测试：真实临时文件覆盖缺失、坏头、截断、成功替换及新插入；新检查严格C++17及ASan/UBSan通过（detect_leaks=0）。实际生产菜单分支经便携UI替身执行，旧版复现异常逃出，修复版保留模型/文件/标记并设置提示。初版测试适配器因enum switch缺少其他分支触发Werror，已修正为int选择器并重跑；该适配器问题未修改产品逻辑。全套便携脚本退出0。最终构建结果见页首。

GitHub：codex/v0.60-model-restore，受检产品提交282a271eb58376f3438afe390a35960275215afe；核心目录沿用maintenance/v0.54-glyph-compaction，内部版本0.60；main未改，完整资源与依赖在ZIP。

差距：2026-11-03核对MSFS2024官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，其物理、航电、天气和职业体验仍为长期参考。本项目缺真实机型系统/FMS、精细机场/DEM、复杂天气、气动标定和完整航班实机验收。没有接近成品的可核查画面对比或HD6000性能证据，不承诺5350U/8GB达到同等画质功能。

下一步：带资源构建启动，按MODEL_RESTORE.md在启动后临时移走默认机模，验证恢复失败不退出、修复文件后可重试；复验座舱NAV→HDG、航班重开与结算。采固定场景60秒帧时/RAM和截图。包内无EXE；未运行Windows游戏或HD6000，继续迭代。
