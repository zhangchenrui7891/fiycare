## v0.65 · 2026-11-07（重开/换机后的自动驾驶状态）

基线：读取最新v0.64（文件版本45）的PROGRESS/TODAY_REVIEW、ui_state、storage、autopilot与座舱代码。CRC及2308项SHA256通过，1661项资源/第三方文件未改。上轮benchmark失效保护、45/45 Linux与19/19 Windows是历史结果。

当前功能：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件座舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、示意地景、FXAA和网站入口。机模/气动仍为原型，网站账号与游戏存档未绑定。

问题：ui::restart和switchAircraft只重置飞机/任务，旧HDG/ALT/NAV模式、航点及目标残留在Snapshot。虽然地面下一次自动驾驶更新会退出模式，菜单暂停与立即保存期间仍有旧模式；后续再启用NAV仍可能使用旧航点。

修改：新resetFlightGuidance在成功重开/换机后退出HDG/ALT/NAV，导航航点归0，目标航向设为新飞机航向、目标高度恢复默认1000米。拒绝的换机不改状态，单独选择任务不重置航电。燃油/载客/生涯的原有规则及事故费用保持，QNH和面板亮度等偏好保留。不增加资产、持续每帧工作或堆分配，不宣称FPS提升。

验证：扩展现有400组重置/任务选择检查，其中300组成功重开/事故重开/换机检查模式、目标、航点和立即存档往返；100组单独选任务保留指导状态。原40组任务生命周期与拒绝动作不可变检查保留。新增ASan/UBSan（detect_leaks=0）通过。旧v0.64 ui_state.h运行相同测试，在New flight must disengage all autopilot modes断言退出1，实际复现残留。

Linux Assimp ON Release完整282步构建成功，45/45 CTest通过（23.54秒），全套严格C++17便携检查退出0。新增ASan/UBSan通过（detect_leaks=0）。所有状态/任务测试均为CPU检查，不代表完整物理试飞。

Windows2022/MSVC x64 Release主程序编译链接成功，19/19 CPU检查通过（含GDI/HLSL），8条既有警告。Assimp OFF、资源复制OFF，未运行Windows游戏/GPU/HD6000。证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/42315587246 ，摘录见validation_v065/windows_excerpt.log。

GitHub：codex/v0.65-guidance-reset，受检产品代码3e55ea6824b90d1e9a11c1530ec422fb168ed9ab，核心目录沿用maintenance/v0.54-glyph-compaction，内部版本0.65。默认main分支未改，完整资源与依赖在ZIP，无EXE。

差距：参考微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 的物理、航电、天气和职业体验，当前缺真实机型专属系统/FMS、精细机场/DEM、复杂天气、气动标定与完整航班实机验收。本轮只修复状态生命周期，不增加真实自动驾驶预位/自动着陆或完整航路规划。

下一步：带资源Windows启动，空中启用HDG/ALT/NAV后重开并立即保存，确认三模式关闭、航点归0；停稳换机后重复。再验收重新起飞启用NAV与完整任务结算，并采集目标Boot Camp固定场景60秒帧时/RAM/截图。当前无目标机连接，没有Windows游戏、鼠标、GPU或HD6000实测。尚非成品，不承诺该硬件达到MSFS2024同等画质功能；继续迭代。
