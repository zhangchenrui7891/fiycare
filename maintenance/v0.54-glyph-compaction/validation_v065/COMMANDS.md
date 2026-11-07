# v0.65 验证命令与范围

- cmake -S source -B build -G Ninja -DCMAKE_BUILD_TYPE=Release；cmake --build build -j4；ctest --test-dir build --output-on-failure。
- bash tools/check_portable.sh /tmp/portable65：严格C++17 -Wall -Wextra -Werror -pedantic。
- g++ -std=c++17 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined career_reset_tests.cpp -o /tmp/guidance65；ASAN_OPTIONS=detect_leaks=0 /tmp/guidance65。
- 从原v0.64 ZIP提取src/ui_state.h，运行新增career_reset_tests；旧代码预期退出1，见old_guidance.log。

测试调用实际ui::restart/switchAircraft，覆盖10机型×2机场×5任务的成功重开、事故重开、换机与任务选择。构造的任务状态和生命周期不是物理试飞，更不是Windows鼠标操作或HD6000实测。
