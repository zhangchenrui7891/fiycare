## v0.75 · 2026-11-09（后台预览限速）

基线：读取最新v0.74完整工程及TODAY_REVIEW、连续记录；ZIP CRC和2417项SHA256通过。上轮窗口缩放取消旧鼠标拖动已完成，历史Linux47/47与Windows20/20通过；提示的v0.8不是本次确认的工程版本，沿用连续版本编号。

当前特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、27控件共用三维驾驶舱及PFD/NAV/ENGINE、训练导航/进近、双机场生涯、浦东离线影像、FXAA。机型系统及气动仍是原型，周边地景为示意，网站账号未绑定本地生涯。

本轮修复：失去前台焦点但未最小化时，旧版暂停物理却仍可能不限速绘制。现在按100毫秒预算补足后台等待（最多约10帧/秒），已超预算不补等、不积累欠帧；前台普通模式不限帧、节能20帧/秒保留，最小化仍走既有跳过渲染路径。等待前重新检查焦点；若将执行后台策略，清除previousMeasured，避免等待期间焦点返回后将后台间隔混入前台统计。同步修正benchmark版本字符串v0.72为v0.75。没有新增资源或每帧分配，未声称实测功耗/FPS改善。

验证：2000组模拟帧预算覆盖两种节能设置、短帧/超预算帧；检查前台恢复和负数/NaN/无穷耗时。实际main末尾限速语句抽取后编译，验证后台清除采样连续标记、恢复不错误标记后台间隔、前台节能等待保留。保留85002次滚动统计对照。上述都是CPU逻辑测试，不是操作系统Sleep精度、GPU或HD6000验证。

本轮验证：Linux Assimp ON Release完整282步构建成功，47/47 CTest通过（25.36秒），严格C++17便携检查退出0；实际限速语句ASan/UBSan检查退出0（detect_leaks=0）。Windows2022/MSVC x64 Release主程序编译成功，21/21 CPU/GDI/HLSL检查通过（1.35秒），8条既有警告。Assimp OFF、资源复制OFF，未运行游戏/GPU。最终受检CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42549024574 。

GitHub分支codex/v0.75-background-pacing，受检代码f87863fec7fdff86e294278f3be3a805212b1b44；目录maintenance/v0.54-glyph-compaction，默认main未改。源码资源ZIP无EXE，1661项资源/第三方文件未改。日志和抽取检查代码见validation_v075。

差距：对照微软官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ 描述的世界、航电、飞行建模、天气与生涯，本项目仍缺真实机型系统/FMS、气动标定、精细机场/DEM、复杂天气及完整航班实机证据。未运行Windows游戏或HD6000，尚无FPS/RAM/真实截图验收。目标5350U/HD6000/8GB/256GB不承诺同等画质和功能；尚非成品，继续迭代。

下一步：带资源Windows构建分别采集前台普通/节能、后台可见、最小化四种状态60秒CPU/GPU/帧时/RAM，检查切回后的输入与飞行恢复及benchmark样本。清单见BACKGROUND_PACING.md。无目标机连接，这些实机步骤未执行。
