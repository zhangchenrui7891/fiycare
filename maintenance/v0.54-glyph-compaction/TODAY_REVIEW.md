## v0.69 · 2026-11-08（启动恢复备份保护）

基线：最新可读完整包v0.68，文件版本49，CRC和2356项SHA256核对通过。读取TODAY_REVIEW与连续记录，继承上轮保存失败可取消退出、46/46 Linux和19/19 Windows历史结果。本次接续尚未打包的v0.69代码提交a128d0c1a4501a7d13c8a90fc5d7806699ba1f60，恢复其准确源码重新构建；此前本地构建输出不可访问，不据此宣称通过。

功能：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维座舱与PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像和FXAA保留。1661项资源/第三方文件字节未改。

缺陷与改动：此前CopyFileW直接覆盖state.bak，复制中途失败可能损坏上次备份。现在复制到state.bak.tmp，成功后MoveFileExW替换备份，再提交新state.txt；复制/备份提交失败进入既有暂停与F5重试路径。状态临时文件显式close并检查结果，失败不继续提交。存档格式与启动加载顺序未改，从备份恢复后首次保存仍不以坏主存档覆盖备份。

验证：实际save/trySave函数的CPU API替身模拟复制写入INCOMPLETE后失败、备份替换失败，核对原主存档/备份字节保持、暂停提示及解除故障后重试。旧v0.68同一回归失败，原因见validation_v069/baseline_failure.log。关闭失败分支经代码检查/编译，未注入真实延迟close故障。

重新运行的Linux Assimp ON Release完整282步构建成功，46/46 CTest通过（14.49秒）；严格C++17便携检查及ASan/UBSan存储检查退出0。全部证据位于validation_v069，CPU替身测试不等于Windows文件系统实机验证。

Windows2022/MSVC x64 Release主程序编译成功，19/19 CPU/GDI/HLSL检查通过（1.05秒），8条既有警告；Assimp OFF、资源复制OFF。https://github.com/zhangchenrui7891/fiycare/actions/runs/42384364558 。未运行Windows游戏、真实文件API故障场景或HD6000。GitHub分支codex/v0.69-backup-save，目录maintenance/v0.54-glyph-compaction，默认main未改；最终文档提交不改产品代码。源码资源包无EXE。

限制：未保证断电持久性、跨文件事务或多实例安全。失败临时备份可能残留，重试覆盖它；每次保存增加一次备份重命名，没有新增GPU资源，未测得FPS或保存延迟提升。真实飞机专属系统/FMS、导航数据库、气动标定、精细机场/DEM、复杂天气、完整航班实机验收仍缺。对照微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 的世界、航电、天气和生涯目标，差距明显；不承诺5350U/HD6000/8GB达到同等画质功能。尚非成品，未停止持续迭代。

下一步：在独立测试存档目录验收带资源Windows启动，阻塞state.bak.tmp或备份替换，确认失败保留文件/暂停/重试；成功后在该测试目录损坏主存档，重启确认备份恢复。随后验收取消退出及完整航班，采集Boot Camp固定场景60秒帧时、RAM和截图。当前没有目标机连接，上述实机验收均未执行，不直接在玩家正式存档上进行破坏性试验。
