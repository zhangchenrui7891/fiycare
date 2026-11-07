## v0.67 · 2026-11-07（保存失败暂停与重试）

基线：读取最新v0.66（文件版本47）、PROGRESS/TODAY_REVIEW、main保存调用及存储检查脚本。CRC及2331项哈希通过，1661资源/第三方文件保持不变。上轮痕迹临时文件提交保护、45/45 Linux与19/19 Windows为历史结果；保存失败仍会进入顶层异常处理退出。

特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件座舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、FXAA和网站入口保留。机型/气动仍为原型，账号未绑定本地生涯。

改动：新增统一trySave边界，依次保存状态与痕迹，捕获std::exception、记录具体错误、保留当前内存航班并暂停。所有原有游戏内保存入口改用该边界；手动保存仅成功时显示成功文字。错误提示持续显示，磁盘空间/权限恢复后按F5重试；成功清除错误但保持暂停，由玩家决定继续。自动保存仍每30秒活动模拟时间触发，无逐帧磁盘重试。退出保存失败单独弹窗并记录Shutdown with unsaved progress，不再记录Normal shutdown。

验证：扩展实际生产lambda的CPU API替身检查，注入状态提交失败与痕迹提交失败，确认不向调用者抛出、暂停标记、错误文字、内存状态保留、旧文件保护，解除故障后重试并读取刚保存的内存航班。四种诊断/普通模式、重复提交、路径阻塞、空痕迹与既有备份检查保留；ASan/UBSan通过（detect_leaks=0）。生产菜单/退出调用的源码断言同步更新。没有执行真实Windows按键、弹窗或文件系统故障测试。

初轮Linux CTest与便携脚本在旧HUD测试替身编译失败：新增saveError未声明。补齐替身并加入持久错误/清除检查后重跑，初轮日志保留为ctest_initial.log和portable_initial.log。产品代码未因此修改。

最终结果：Linux Assimp ON Release完整282步构建成功，修复测试替身后45/45 CTest通过（12.50秒），全套严格C++17便携检查退出0。Windows2022/MSVC x64 Release主程序编译成功，19/19 CPU检查通过，8条既有警告；Assimp OFF、资源复制OFF，没有Windows游戏/GPU/HD6000验证。证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/42362539428 ，关键日志validation_v067/windows_excerpt.log。Windows受检提交之后仅补Linux测试替身和文档/证据，产品代码未变。

GitHub：codex/v0.67-save-recovery，受检代码58592df70e4f735b2c732ac6a21de1e83073a26e；目录继承maintenance/v0.54-glyph-compaction，内部0.67；默认main分支未改，完整资源依赖在ZIP，无EXE。

边界：主动退出时窗口已关闭，失败弹窗只是告知，不支持取消退出回到航班。状态与痕迹仍是两个独立提交，第二个失败时第一个可能已更新，不保证跨文件事务、断电持久性或多实例并发安全。重试写入当前内存状态，不回滚玩家已执行的任务/购买。所有游戏内保存现在同时提交痕迹（最多400条），设置变更会多一次小文件写入；没有新增每帧工作或声称FPS提升。可手动继续未保存的航班，错误提示保持到保存成功；自动失败再次暂停。

差距：对照微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，仍缺真实机型系统/FMS、气动标定、精细机场/DEM、复杂天气及完整航班实机验收。没有成品画面对比或HD6000性能证据，不承诺5350U/HD6000/8GB达到相同画质功能。

下一步：带资源Windows启动，临时阻塞state.tmp或tracks.tmp后按F5，确认游戏仍在、暂停与中文提示；解除阻塞再按F5，恢复后手动继续并重启读取。另验收自动保存失败及退出警告。采集目标Boot Camp固定场景60秒帧时、RAM和截图；当前没有目标机连接，未运行Windows游戏/GPU/HD6000。尚非成品，继续迭代。
