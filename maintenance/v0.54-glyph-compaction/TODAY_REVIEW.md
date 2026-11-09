## v0.77 · 2026-11-09（旅客航班进行中锁定人数）

基线：读取v0.76完整工程、TODAY_REVIEW及连续记录，ZIP CRC及2440项SHA256通过。上轮可选机模路径异常回退保留；历史Linux47/47、Windows21/21通过。沿用连续版本，提示的v0.8不是本轮确认的工程版本。

当前特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维驾驶舱及PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、FXAA。机型系统及气动仍是原型，周边地景为示意，网站账号未绑定本地生涯。

本轮问题：旅客航班开始后只要停稳仍可改变人数，可能在抵达结算前增载并改变收入。ui::setPassengers现在拒绝旅客任务阶段1/2的调整；阶段0/3、其他任务仍按客机与停稳条件处理。拒绝不修改状态；存档无需迁移，已有阶段字段决定限制。原生及游戏菜单共享检查，飞机页面和拒绝提示说明原因。项目与benchmark版本同步为v0.77。

验证：80组机型/机场/阶段检查及存档往返，拒绝时全Snapshot不变，其他任务规则保留；400组重开/换机/任务选择、40组模拟旅客生命周期保留，目的地结算前增载被拒绝。旧版头文件配新测试失败，新版通过。初版测试fixture含stage1/gate4不合法组合造成读档失败，已改为合法gate0并重跑；未修改存档校验。没有真实飞行轨迹或Windows菜单操作验收。

本轮验证：Linux Assimp ON Release完整282步构建成功，47/47 CTest通过（15.75秒），增量检查无待构建内容；严格C++17便携检查与相关ASan/UBSan检查退出0（detect_leaks=0）。Windows2022/MSVC x64 Release主程序编译成功，21/21 CPU/GDI/HLSL检查通过（1.66秒），8条既有警告。Assimp OFF、资源复制OFF，未运行游戏/GPU。最终CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42613828749 。日志见validation_v077。

GitHub分支codex/v0.77-passenger-lock，受检代码d20db39784a3e713d76dac5a9522bd47e934e72f；目录maintenance/v0.54-glyph-compaction，默认main未改。最终记录提交不改变产品代码。完整源码资源ZIP无EXE；1661项资源及第三方文件未改。

差距：对照微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 所述世界、航电、天气、生涯，本项目仍缺真实机型系统/FMS、气动标定、精细机场/DEM、复杂天气和完整航班实机证据。未运行Windows游戏/HD6000，没有FPS/RAM/画面对比验收。不会承诺5350U/HD6000/8GB/256GB达到MSFS2024同等画质功能；尚非成品，继续迭代。

下一步：按PASSENGER_FLIGHT_LOCK.md在带资源Windows构建验收正常登机、返航停稳拒绝改人数、读档后限制、落地结算后重新登机，并核对收入。继续收集Boot Camp启动、帧时、内存及完整航班证据；无目标机连接，上述实机步骤未执行。
