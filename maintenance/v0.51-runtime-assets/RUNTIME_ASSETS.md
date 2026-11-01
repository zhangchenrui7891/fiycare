# v0.51 运行资源同步

旧版使用 POST_BUILD：仅更新 assets 而没有 C++ 重链接，或构建后删除输出 assets，重新构建不会复制，可能仍显示旧贴图/机模或缺资源启动失败。

新增 tools/runtime_assets.cmake：构建 FlightCareerAir 前总是运行资源复制目标，复制失败使构建失败，可修复源目录后直接重试。普通默认构建和显式 FlightCareerAir 目标都包含复制；OFF 模式仍不复制。不删除输出目录中源端已删除的额外文件，以保留用户自定义内容；不是发布目录镜像工具。每次构建会遍历资源目录，尚未测量Windows构建耗时；不改变游戏每帧工作。

该函数要求 CMake >=3.22 的 CMP0112 NEW（工程已有最低版本），TARGET_FILE_DIR 不产生反向目标依赖；适配单配置与多配置输出路径。独立测试实际构建小型程序，包含空格/中文路径、资源变更无需重新链接、删除输出修复、失败/恢复、OFF/ON；非游戏运行测试。冻结旧 POST_BUILD 实现会在资源变更用例失败。

Windows验收：运行 check_release.bat、build_vs.bat；仅更换一个合法原生贴图或机模，再 build_vs.bat，核对 EXE旁 assets 的哈希；删除输出的一项资源再构建应补回。不要删源码 assets 或用户自定义机模。随后验收卫星/三维地景、27控件和一次完整航班，并用 run_benchmark_dx11.bat 采集60秒帧时、内存和截图。没有目标机证据前不宣称FPS改善。

附带修复v0.50地景near与Windows SDK宏的编译冲突，改名highDetail并加入宏环境回归，网格生成逻辑不变。
