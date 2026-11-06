# v0.64 检查命令与边界

- cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release（默认Assimp ON）
- cmake --build build -j4
- ctest --test-dir build --output-on-failure
- bash tools/check_portable.sh /tmp/fcair_portable64（退出0）
- g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined benchmark_tests.cpp -o /tmp/benchmark64；ASAN_OPTIONS=detect_leaks=0运行（通过）
- python3 tools/check_benchmark_context.py（通过）
- FCAIR_SANITIZE=1 ASAN_OPTIONS=detect_leaks=0 python3 tools/check_benchmark_context.py（通过）
- 同脚本传入原ZIP提取的v0.63 src/main.cpp：预期退出1，Changed production context still permits benchmark verdict。

环境准备：首次cmake命令不可用（退出127），安装cmake/ninja后重新执行完整构建。最初旧版对照使用了已不存在的上一轮本地路径；随后从已验证原ZIP提取旧main重新运行，最终old_integration.log才是缺陷复现证据。

CPU替身语句检查不验证真实Windows消息/GPU，Windows CI关闭Assimp和资源复制；不宣称目标机性能或游戏实机通过。
