## v0.76 · 2026-11-09（可选机模路径异常回退）

基线：已读取最新v0.75完整工程、TODAY_REVIEW和连续记录，ZIP CRC及2429项SHA256通过。沿用连续交付版本；提示的v0.8不是本轮确认的工程版本。上轮后台10帧预算及采样保护保留，历史Linux47/47、Windows21/21通过，目标机验证仍缺失。

当前特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维驾驶舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像及FXAA。机型系统/气动仍为原型，周边地景为示意，网站账号未绑定本地生涯。

本轮代码：把可选自定义机模exists检查移入既有加载异常恢复范围。此前路径检查异常绕过回退，默认模型可用也会终止启动；现在保存诊断并回退至默认模型或程序生成几何。必需默认资源也不可用仍报错。未删除自定义文件、未修改当前模型；不捕获内存耗尽异常。同步更新项目及benchmark版本为v0.76。

回归：Linux以4096字符路径分量触发真实filesystem_error，验证默认/程序生成两种回退、诊断保留、必需默认失败不被吞掉、当前模型及持久自定义数据不变。v0.75头文件配新测试退出1，新版退出0。Windows测试允许平台不抛该路径异常时跳过该组并提示，故不能用Windows测试总数证明Windows异常复现。没有测试真实Windows游戏启动。

本轮验证：Linux Assimp ON Release完整282步构建成功，47/47 CTest通过（26.03秒），严格C++17便携检查退出0；相关ASan/UBSan检查退出0（detect_leaks=0）。Windows2022/MSVC x64 Release主程序编译成功，21/21 CPU/GDI/HLSL检查通过（1.65秒），8条既有警告。Assimp OFF、资源复制OFF，未运行游戏/GPU。CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42576542907 。详见validation_v076。

GitHub分支codex/v0.76-model-probe；受检代码f2d8739618772cf6e2a8e736ce27e761db7c6520，目录maintenance/v0.54-glyph-compaction，默认main未改。源码资源ZIP无EXE，1661项资源/第三方文件未改。最终记录提交不改变产品代码。

差距：对照微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 的世界、航电、天气和生涯体验，本项目仍缺真实机型系统/FMS、气动标定、精细机场/DEM、复杂天气和完整航班实机证据。未运行Windows游戏或HD6000，没有FPS/RAM/真实画面对比验收。不承诺5350U/HD6000/8GB/256GB达到MSFS2024同等画质功能，尚非成品，继续迭代。

下一步：优先执行MODEL_PROBE_RECOVERY.md中的带资源Windows启动、异常路径回退与日志验收，再采集目标机启动时间、内存、帧时和完整航班证据。当前无目标机连接，这些实机步骤尚未执行。
