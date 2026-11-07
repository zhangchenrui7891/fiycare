## v0.66 · 2026-11-07（地面痕迹存档提交保护）

基线：读取最新v0.65（文件版本46）、PROGRESS/TODAY_REVIEW、主循环保存函数和存储回归脚本。CRC及2319项SHA256通过，1661项assets/third_party逐字节未改。上轮重开/换机清理指导状态，45/45 Linux与19/19 Windows为历史结果。

功能特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件座舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、FXAA与账号网站入口保留。机型/气动仍为原型，网站账号未与游戏存档绑定。

问题：saveTracks直接打开tracks.txt并截断旧内容，未检查打开/写入/关闭结果。故障可能破坏旧痕迹而没有明确错误。本轮改为tracks.tmp写入，检查flush与close状态后才通过MoveFileExW替换tracks.txt；替换失败抛出明确错误。诊断运行仍不写存档，空痕迹仍可正常提交为空文件。现有state.txt保存逻辑不变，不宣称FPS或画质提升。

检查：执行实际生产save与saveTracks lambda的CPU API替身，覆盖普通/benchmark/smoke/组合四模式、重复提交、状态往返、故意阻塞临时文件路径、注入替换失败、旧内容不变、故障解除后重试成功及空痕迹提交。严格C++17和新增ASan/UBSan（detect_leaks=0）通过。旧v0.65 main在Failed track commit must preserve existing file and report failure断言失败，脚本退出1。临时路径阻塞验证打开失败；没有真实磁盘写满、断电或Windows文件系统故障注入。

Linux Assimp ON Release完整282步构建成功，45/45 CTest通过（13.79秒）；全套严格C++17便携检查退出0。Windows2022/MSVC x64 Release主程序编译链接成功，19/19 CPU检查通过，8条既有警告。Assimp OFF、资源复制OFF；没有Windows游戏/GPU/HD6000实测。Windows证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/42340829388 ，关键日志见validation_v066/windows_excerpt.log。

GitHub：codex/v0.66-track-save，受检产品代码954e5bdf7ac16a6dc34ca55e2fdc368be68be96c，继承目录maintenance/v0.54-glyph-compaction，内部版本0.66。默认main分支未改；完整资源与依赖在ZIP，无EXE。

限制与行为变化：保存失败仍进入现有顶层错误提示并结束游戏，本轮未实现不中断重试UI；测试中的重试是函数级重试。失败时可能留下tracks.tmp，下次保存会重新写入。state.txt和tracks.txt不是一个跨文件事务，可能前者已更新而后者保留旧数据。本轮不承诺断电持久性、并发多实例安全或完整飞行存档事务。相比原先静默失败，无法保存痕迹现在会明确报错，需修复目录权限/磁盘空间后重启。未改写游戏气动、图形和资产。

差距：参考微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，真实机型系统/FMS、气动标定、精细机场/DEM、复杂天气和完整航班实机验收仍有明显不足。当前没有接近成品的画面与性能对比证据，不承诺5350U/HD6000/8GB达到MSFS2024相同画质功能。

下一步：目标Windows带资源启动，保存并重启确认地面痕迹恢复；再验证无写权限时错误提示、旧tracks.txt保留和修复后重试。继续验收重开后NAV及完整航班，并采集固定场景60秒帧时/RAM/截图。当前无目标Boot Camp连接，未运行Windows游戏或实测HD6000；继续迭代。
