最终验证（2026-11-02）：Linux Assimp ON Release构建及43/43 CTest通过；严格C++17便携脚本退出0，新增ASan/UBSan通过（detect_leaks=0）。Windows Server2022/MSVC主程序编译成功，16/16 CPU检查通过（含GDI/HLSL），Assimp OFF、资源复制OFF，8条既有警告。没有游戏、GPU、HD6000或完整航班实机验证。Windows证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/41473357011 。

## v0.59 · 2026-11-02（HDG接管NAV）
基线：读取最新v0.58（文件版本39）、PROGRESS/TODAY_REVIEW和座舱/自动驾驶代码。原ZIP CRC及2222项哈希通过，1661项assets/third_party逐字节一致。上轮任务重置修复已有Linux42/42及Windows15/15记录，本轮独立验证。

当前功能：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维驾驶舱、PFD/NAV/ENGINE三屏、训练导航/几何进近、双机场生涯、浦东离线影像、示意地景、FXAA及网站入口。网站账号未绑定游戏存档，本轮未改变网站公开设置或验证地区可达性。

问题与修复：NAV内部每步启用headingHold，旧HDG按钮只翻转此标志，NAV下一步又重新接管。旧版运行新测试确实失败。现在NAV中按HDG会清除NAV、启用HDG并捕获当前航向；再按HDG退出横向保持。ALT、原有NAV关闭后的航向保持、手动干预和失电退出规则保留。无新资源或每帧分配，不宣称FPS收益。未新增真实FCU/FMS、模式预位或自动着陆。

新增检查：10机型×2物理模式20组切换链路、2000次后续导航更新，覆盖航向选择不被覆盖、ALT独立、重新选择NAV、手动接管、断电和不一致NAV状态；严格C++17及新增ASan/UBSan（detect_leaks=0）。这是构造状态的CPU检查，不是物理试飞或Windows鼠标/游戏验收。最终构建结果见本页首段。

GitHub分支codex/v0.59-lateral-mode，受检产品提交8d04efa73bec4d6a5db51e6dbeab810cdd9ccbc5；核心路径沿用maintenance/v0.54-glyph-compaction，内部版本0.59。完整资源与依赖在ZIP，main未改。

差距：对照本轮检索的MSFS2024官方新手指南所述物理、航电、天气与职业体验，本项目仍缺真实机型系统/FMS、精细机场/DEM、复杂天气、气动标定和完整航班验收。参考 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 。没有接近成品的画面对比或实机性能证据，不承诺5350U/HD6000/8GB达到相同画质功能。

下一步：目标Windows带资源构建启动，按LATERAL_MODE.md验收NAV→HDG→调整航向→退出→重选NAV及失电/手动接管；同时复验上轮返程重开、保存载入和结算。固定场景记录60秒帧时/RAM和座舱截图。包内无EXE，未运行Windows游戏或HD6000，继续迭代。
