## v0.43 · 2026-10-31（滚动帧时统计）

基线：下载最新v0.42恢复版（文件版本23），ZIP CRC及1925项清单SHA256全部一致。读取PROGRESS、RECOVERY_V042和上轮验证记录。当前七机型、27控件共用座舱、航电、训练导航/几何进近、双机场任务、近似气动、生涯及原画质设置保留。

实际修改：FrameWindow每帧统计从完整排序改为nth_element选择nearest-rank P95，在原求和遍历中取最大值；保持固定300样本、FPS求和顺序、P95和最长卡顿语义、环形过期与重置。main版本文字/CMake更新v0.43。新增独立CPU回归及CTest/严格检查注册。产品/资源/依赖对比仅src/performance.h与main.cpp改变，其余逐字节保留。60秒BenchmarkSamples未改。

本轮验证：Assimp开启的Linux完整Release构建252步通过；24/24 CTest通过；严格C++17全套通过；新增回归ASan/UBSan（detect_leaks=0）通过；增量ninja无工作。初轮CTest为23通过、1项因本地生成的cockpit_solids_check缺执行权限未启动；修正该文件执行权限后全套复验通过，初轮日志保留。新回归85002次与冻结v0.42完整排序算法精确对照，涵盖部分窗口、环形覆盖、重复查询、重置、无效输入和卡顿过期。

合成CPU探针五轮各200000次add+summary：旧1460–1492ms，新282–311ms，约每次节省6微秒。仅为本环境函数级结果，不是整帧/GPU/Windows/HD6000/RAM证据，不宣称游戏FPS增长或卡顿消失。详见PERFORMANCE_WINDOW.md和validation_v043。

GitHub：用户本轮明确授权目标仓库。命令行完整推送因无法取得HTTPS登录凭据失败；已连接接口可写，改为独立分支codex/v0.43-frame-statistics上传本轮变更集、独立测试、集成补丁和验证日志。分支不是完整工程镜像；未上传全部资源/依赖，不触发虚假的Windows CI。实际远程结果以GITHUB_UPLOAD_V043.md和交付链接为准。旧main保留。

未验证：Windows/MSVC整程序、真实字体/键鼠/GPU画面、HD6000默认画质60秒帧时/RAM；包内无EXE。共用座舱/近似气动/训练NAV仍非真实机型系统、完整FMS/EFB和校准飞行模型。精细机场、复杂天气及完整航班验收仍有明显缺口，没有可核查的成品画面比较，未接近MSFS2024目标，不承诺目标硬件同等画质或功能。

下一步：先check_release.bat、build_vs.bat，优先处理真实Windows启动/编译错误，再验收27控件/中文及默认画质同场景60秒帧时、RAM和截图。完整GitHub镜像需可认证的Git推送环境。旧缺失日志范围继续保留，未补造。继续迭代，不宣告成品。

