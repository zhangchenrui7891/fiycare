# v0.63 验证索引（2026-11-06）

受检产品代码：09a332cb9499cd6602fda769974e5982f81f58cb。之后仅更新文档/证据。

- baseline.log：CRC、2279项哈希，1661资源/依赖文件未改。
- configure.log/build.log/ctest.log：Linux Release，Assimp ON，282步，45/45通过，进程退出0。
- portable.log：bash tools/check_portable.sh /tmp/fcair_portable63，退出0；C++17 -Wall -Wextra -Werror -pedantic。
- segment.log：python3 tools/check_performance_segment.py，实际main采样/重置语句8种切换及恢复。
- segment_sanitized.log：FCAIR_SANITIZE=1 ASAN_OPTIONS=detect_leaks=0运行上项，通过。
- old_segment.log：同脚本读取v0.62 main，退出1为预期旧缺陷复现。
- render_metadata.log：16组实际CSV语句兼容检查，通过。
- windows_excerpt.log：Windows CI日志关键行，主程序链接及18/18检查通过，8条既有警告。

Windows2022/MSVC x64 Release主程序编译链接成功，18/18 CPU检查通过（含新增配置边界、GDI及HLSL），8条既有警告。Assimp OFF、资源复制OFF；未运行Windows游戏、GPU或HD6000。证据：https://github.com/zhangchenrui7891/fiycare/actions/runs/42230227288 ，日志摘录validation_v063/windows_excerpt.log。

没有实机截图、游戏运行、GPU帧时、目标机RAM或性能提升数据。源码ZIP不含EXE。
