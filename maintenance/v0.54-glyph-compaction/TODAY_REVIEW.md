## v0.74 · 2026-11-09（窗口缩放取消旧鼠标拖动）

基线：读取最新v0.73完整工程、TODAY_REVIEW与连续记录，ZIP CRC及2407项SHA256通过。沿用连续版本编号；提示中的v0.8不是本次确认的工程版本。上轮已实现油门/环视互斥；历史Linux47/47、Windows CI20/20通过，实机验收仍空缺。

当前特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维驾驶舱及PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、FXAA。气动/机型系统仍属原型，周边建筑道路为示意，网站账号尚未绑定本地生涯。

本轮代码：WM_ENTERSIZEMOVE此前只暂停键盘，未取消油门/环视拖动；缩放后归一化鼠标坐标变化可能让旧拖动延续并跳变。现在同时cancel驾驶舱输入、清空待处理命令和左键标记，并只释放本窗口持有的捕获。已应用的飞行状态保持不变，恢复后必须重新按下鼠标。本轮只改输入事件路径，无新增贴图/地景或每帧分配。

回归：扩展实际main鼠标语句抽取检查，模拟Win32 API及捕获回调；覆盖油门/环视×本窗口/其他窗口捕获四组合、960×540到1920×1080坐标变化、重启输入后的新按键恢复。v0.73语句在取消断言处失败；新版通过。既有两种松键顺序等检查保留。更新键盘源码布线检查以匹配新取消路径。CPU模拟并非Windows消息/硬件验收。

本轮验证：Linux Assimp ON Release完整282步构建成功，47/47 CTest通过（17.86秒）；严格C++17便携检查退出0。Windows2022/MSVC x64 Release主程序编译成功，20/20 CPU/GDI/HLSL检查通过（1.37秒），8条既有警告。Assimp OFF、资源复制OFF，未运行游戏/GPU。CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42523521225 。详见validation_v074。

GitHub分支codex/v0.74-resize-input；受检代码b05d2de2ff4da5b64e754cb747aaeb8e46bfa3bd，目录maintenance/v0.54-glyph-compaction，默认main未改。最终提交仅补记录。源码资源ZIP不含EXE，1661项资源/第三方文件未改。

差距：微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 所述全球世界、航电、气动、天气、生涯体验仍是长期目标。本项目缺真实机型系统/FMS、气动标定、精细机场/DEM、复杂天气和完整航班实机证据。未运行Windows游戏或HD6000，没有FPS/RAM/截图验收；不承诺5350U/HD6000/8GB达到MSFS2024同等画质功能。尚非成品，继续迭代。

下一步：见RESIZE_INPUT.md，优先带资源Windows启动及窗口缩放前后油门/环视恢复验收，再收集Boot Camp 60秒帧时/RAM和完整航班记录。当前无目标机连接，未执行这些人工步骤。
