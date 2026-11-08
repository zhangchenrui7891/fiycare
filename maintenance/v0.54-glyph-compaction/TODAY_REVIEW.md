## v0.73 · 2026-11-08 UTC（驾驶舱油门与环视输入互斥）

基线：最新可访问完整工程为v0.72（文件版本53），已读取TODAY_REVIEW、PROGRESS、上轮验证记录与主程序/驾驶舱输入代码。2392项SHA256及ZIP CRC通过。提示中的v0.8不是本次新确认的工程版本；沿用连续交付编号。上轮修复新航班告警确认残留，Linux46/46、Windows19/19是历史结果。

当前特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维驾驶舱、PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像和FXAA保留。机型系统/气动仍是原型，周边地景为示意；网站账号未绑定本地生涯。

本轮实际改动：油门左键拖动与右键环视原本可同时开启，右键移动抢走油门事件，释放右键又释放鼠标捕获并结束油门。现在先开始的拖动保持控制权，后按的另一按钮被忽略；环视时左键不再操作开关/油门/菜单。失去捕获结束两种拖动并保留已排队最终值，失焦/菜单等cancel路径则清空输入。main改用同一输入对象的looking状态，新增一个布尔状态，无新资源或每帧分配。

检查：扩展cockpit_input_tests，覆盖两种松键顺序、开关抑制、捕获丢失、取消和恢复，并保留100001事件合并与120000混合命令回归。新增check_pointer_integration.py，编译main实际鼠标消息语句、模拟Win32捕获回调；v0.72语句在“油门拖动时右键不能转动视角”断言失败，新版通过。该检查属于CPU模拟消息，不是Windows实机鼠标验证。初次检查脚手架遇到原有单行for/return的严格缩进警告；抽取时仅规范换行后编译通过，未改变语句逻辑。

Linux Assimp ON Release完整282步构建完成，后续增量重建通过；最终47/47 CTest通过（18.48秒）。首次配置因当前环境未安装cmake退出127，安装cmake/ninja后成功。严格C++17便携检查与ASan/UBSan检查退出0（detect_leaks=0）。Windows2022/MSVC x64 Release主程序编译成功，20/20 CPU/GDI/HLSL检查通过（1.15秒），8条既有警告；Assimp OFF、资源复制OFF，未运行游戏/GPU。CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42497946848 。日志见validation_v073。

GitHub分支codex/v0.73-pointer-ownership，受检代码c662bfa746c10642f661ec02a96654ecc4277165，目录maintenance/v0.54-glyph-compaction；默认main未改。最终记录提交不改变产品代码。完整源码资源ZIP不含EXE，1661项资源/第三方文件与v0.72字节相同。

差距与边界：未运行Windows游戏、实际键鼠或HD6000，没有FPS/RAM/画面对比验收证据。微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 描述其全球世界、航电、天气、飞行建模与生涯体验；本项目仍缺真实机型系统/FMS、气动标定、精细机场/DEM、复杂天气、完整航班实机验收。仍明显未达成品，不承诺i5-5350U/HD6000/8GB实现MSFS2024同等画质或功能。

下一步：优先在带资源的Windows构建验收左右键交叉操作、两种松键顺序、切走焦点/菜单/全屏后的恢复；再收集Boot Camp启动日志、60秒帧时/RAM、真实截图和完整航班结果。验收清单见POINTER_OWNERSHIP.md。当前无目标机连接，实机步骤尚未执行；继续迭代，不触发成品确认或停止条件。
