## v0.51 · 2026-11-01（增量构建运行资源同步）

基线：读取最新 v0.50（文件版本31）、完整连续记录、地景/构建/资源代码；ZIP CRC及2087项SHA256核验通过。七机型、27控件共用座舱、航电、训练NAV/几何进近、双机场任务、生涯、浦东离线影像、机场西侧示意三维地景和FXAA保留。

实际修复：CMake原来仅在FlightCareerAir重链接后POST_BUILD复制assets。资源本身变更或输出assets丢失时，再构建程序不重链接便不复制，导致过期/缺失资源。改为主程序显式依赖每次执行的资源复制目标，复制失败传播为构建失败、可以修复后重试；OFF模式不复制。使用目标文件目录支持单/多配置路径；每次构建增加目录遍历，不改变游戏运行开销，未测Windows构建耗时。不清理输出端额外文件，不是精确目录镜像。CMake/Main版本更新v0.51，其余运行逻辑未改；原28份运行资源及1630份依赖文件逐字节相同。

验证：旧POST_BUILD冻结夹具在仅改资源用例确实失败；生产函数在Ninja和Ninja Multi-Config共14组场景通过，实际构建小程序，覆盖空格/中文路径、资源更新且EXE哈希/mtime保持、删除输出修复、缺源失败与恢复重试、OFF/ON。Linux Assimp开启Release全部目标264步构建完成；初轮两个二进制因本地执行权限被拒绝未启动，删除这两个输出并由链接器重新生成后最终34/34 CTest通过，JUnit0失败0跳过；严格C++17便携全套通过。初始/最终日志保留。无新增C++算法，本轮不重复新增ASan/UBSan，不引用旧结果冒充本轮。

本轮追加的真实Windows证据发现v0.50的地景头文件存在near标识符与Windows SDK旧宏冲突，主程序MSVC编译失败（run41271924954）。实际将src/scenery.h中的near参数/局部变量统一改名highDetail，不改变几何/LOD；地景回归在生产头文件包含前定义near/far空宏，包含后清理。最初夹具也误影响测试自身near/far变量，已限定宏作用范围，保留失败记录。修复后Linux再次全量34/34回归与地景独立严格C++17通过；这是本轮必须修复的Windows编译阻碍。Windows复验结果另记于summary。

Windows第二次复验主程序编译成功，9/9 CPU检查通过（run41272122345，Assimp OFF/运行资源复制OFF）。新增资源小程序测试初次读取CMake生成的UTF-8路径时误用Windows默认编码，导致中文路径乱码而找不到输出；已在测试脚本显式UTF-8读取路径与日志，不修改产品逻辑，后续第三次复验以最终记录为准。

最终Windows结果：run41272267314成功，提交1125820b049320dad6aa6336a7eb730a6b94dca8；Windows Server2022/MSVC x64 Release主程序编译链接成功，9/9 CPU检查通过，Visual Studio 17 2022实际资源复制夹具7组场景通过（含中文/空格路径）。主程序Assimp OFF/真实资源复制OFF，资源复制使用合成小文件，不是带真实机模的游戏启动。既有MSVC警告保留。结果链接：https://github.com/zhangchenrui7891/fiycare/actions/runs/41272267314 。

GitHub：产品代码提交0c7cdbd845a141af0af2c621ff73722f85c20e08，独立分支codex/v0.51-runtime-assets，maintenance/v0.51-runtime-assets含继承的v0.49核心源码、本轮修复及v0.50地景。运行资源/完整第三方仍随ZIP交付，不是完整发行镜像；main不改。Windows检查见validation_v051/summary.json及本记录后续结果。

边界：没有运行游戏，不验证Windows场景、真实键鼠、资源视觉效果或HD6000 FPS/RAM；包内无游戏EXE，不宣称改善帧率。上轮地景是示意建筑/路网，没有碰撞。与MSFS2024相比真实机型系统/FMS、精细机场高程与地景、复杂天气、气动标定及完整航班仍有明显差距。2026-11-01读取官方指南 https://www.flightsimulator.com/guide/new-flight-simmer-guide/ （文中日期2026-01-05），参考物理/航电/天气/生涯方向；未具备可核查的成品实机对比，不承诺5350U/HD6000/8GB同等画质功能。

下一步：目标Windows运行check_release.bat、build_vs.bat，验收带资源启动和仅改资源后增量复制；卫星/三维地景、中文仪表和27控件操作检查后，采同场景同画质60秒帧时/RAM/截图。根据瓶颈决定地景预算或渲染优化，继续迭代，不宣告成品。

