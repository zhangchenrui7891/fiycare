最终结果（2026-11-03）：Linux Assimp ON完整Release构建与44/44 CTest通过；全套严格C++17便携检查及渲染选择ASan/UBSan通过（detect_leaks=0）。Windows2022/MSVC x64 Release主程序编译链接成功，17/17 CPU检查通过；Assimp OFF、资源复制OFF，8条既有警告。没有游戏运行/GPU/HD6000实测或FPS提升数据。Windows证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/41556622167 。受检代码207bfc85aa848937d7ae6cfca4554121488ac541；之后仅补资料。

验证环境记录：Linux首轮asset_pipeline_check缺少可执行权限（BAD_COMMAND），已恢复权限并重跑；保留ctest_initial.log，不将无法执行算作通过。

## v0.61 · 2026-11-03（默认渲染负载）
基线：实际读取v0.60（文件版本41）、PROGRESS/TODAY_REVIEW及启动/渲染配置。原ZIP CRC及2252项哈希通过，1661项资源和依赖逐字节一致。上轮模型恢复失败保护及Linux44/44、Windows17/17均为历史结果，本轮独立验证。

功能特点：C++17、DX12/DX11、中文菜单、十机型含A330/A350、共用27控件三维座舱、三屏航电、训练导航/进近、双机场生涯、浦东离线底图、示意地景、FXAA和网站入口。网站账号未绑定游戏存档，本轮未改变站点访问权限。

问题与改动：默认renderScale=2导致宽高各2倍，即四倍像素，对HD6000默认负载过高。现在默认原生比例1；增加精确--supersample开关，--native优先且顺序无关。高画质脚本显式开启超采样，菜单切换仍保留。low/benchmark仍固定640×360。FXAA及已有存档设置不变，原生可能比超采样更显细小锯齿，不能声称画面完全相同。

性能边界：1600×900客户区的渲染目标由3200×1800降为1600×900，像素数从5,760,000降到1,440,000。只是像素预算计算，不是帧率、显存或HD6000实测；模型、CPU模拟及其他负载不随之等比例减少。未修改资产、气动、地景或生涯。

检查：生产createRenderer以CPU API/渲染器替身执行，覆盖默认、显式2×、切回原生、DX11回退、low/benchmark重建。启动参数覆盖19标志、133误导字符串及native优先级。旧版默认尺寸在新增断言失败。严格便携与新增ASan/UBSan结果和完整构建见页首；不等于Windows真实窗口/GPU验收。

GitHub：codex/v0.61-native-default，受检代码207bfc85aa848937d7ae6cfca4554121488ac541；继承核心目录maintenance/v0.54-glyph-compaction，内部版本0.61。main未改；完整资产/第三方依赖在ZIP。

差距：2026-11-03复核官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ ，其物理、航电、天气和职业体验仍为长期参考；项目缺真实机型专属系统/FMS、精细机场/DEM、复杂天气、气动标定与完整航班实机验收。当前没有接近成品的功能/画面与实机性能证据，不承诺5350U/HD6000/8GB达到同等画质功能。

下一步：目标Windows带资源启动，固定机场、机型、视角与FXAA，比较原生和2×超采样各60秒帧时/RAM与截图，尤其检验座舱小字和机翼边缘。既有640×360 benchmark不代表此次原生/超采样对比。包内无EXE，未运行Windows游戏或HD6000；继续迭代。
