最终验证（2026-11-02）：Linux Assimp开启完整Release构建成功，41/41 CTest通过。Windows2022/MSVC x64 Release主程序编译链接成功，14/14检查通过，含新告警选择、真实GDI与HLSL检查；Assimp OFF、资源复制OFF。8条既有MSVC警告仍在。未运行游戏/真实键鼠或测试HD6000。Windows结果：https://github.com/zhangchenrui7891/fiycare/actions/runs/41392088100 。受检产品代码7da631ecb0633c6b4bfb82a5d9c994da388928b5，后续仅补资料。

## v0.57 · 2026-11-02（新告警显示优先级）
基线：实际读取v0.56（文件版本37）、连续记录和当前告警/绘制代码。ZIP CRC及2198项SHA256通过，原assets与third_party逐字节一致。十机型、三屏航电、27控件共用驾驶舱、训练导航/进近、双机场任务、生涯、浦东影像、示意地景、FXAA和网站入口保留。网站账号未绑定游戏存档，本轮未改站点公开设置。

问题：告警栏直接显示active.front()，当首项已经确认但故障持续、后续同等级新故障出现时，告警栏仍显示旧项。以一号发动机失效已确认、二号发动机随后失效为例，第二条警告无法成为显示项。冻结v0.56 engineering_panel.h并运行新版绘制检查，确实在新告警可见性断言失败。

修复：AlertManager增加headline()选择器，优先最高严重程度；同等级优先未确认项；其余按原收集顺序稳定选择。engineering_panel绘制选择结果。选择器不重排或修改告警、不分配内存；确认操作仍确认全部当前告警，故障不因此消除。已确认Warning仍优先于未确认Caution，这是明确保留的严重程度策略；未增加告警浏览、声音、真实ECAM/EICAS或机型专属故障程序。

检查：九种双发机型共63次实际故障/确认/恢复/再次发生转换、36种等级与确认状态组合通过；覆盖空列表、最高等级、稳定顺序、不修改active以及故障再次发生重新需要确认。实际main覆盖层与engineering_panel函数使用CPU字体适配执行，确认debugPanel关闭时新告警仍可显示、确认后故障仍在。严格C++17全套便携回归及新增ASan/UBSan通过（detect_leaks=0）。最终CMake/CTest及Windows结果另补，不以旧结果替代。

GitHub：codex/v0.57-alert-headline，受检产品提交7da631ecb0633c6b4bfb82a5d9c994da388928b5；继承目录maintenance/v0.54-glyph-compaction，内部版本0.57。核心源码和检查上传，完整资源/依赖在ZIP。main未改。

差距：真实机型系统/FMS、精细机场/高程、复杂天气、气动标定和完整航班仍不足。2026-11-02核对官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，其物理、航电、天气及职业体验仍是长期参考。当前没有达到成品的图形/性能/完整功能证据，不承诺5350U/HD6000/8GB同等画质功能。

下一步验收：Windows完整构建并带资源启动，在双发机型依次触发一发失效、Backspace确认、二发失效，确认告警栏改为二发且提示未确认；再次确认后持续故障仍保留，F8复位后消失。再验收NAV和座舱实际画面，采固定场景60秒帧时、内存与截图。CPU检查不等于真实键盘消息/GDI显示/GPU或HD6000结果。无游戏EXE，继续迭代。
