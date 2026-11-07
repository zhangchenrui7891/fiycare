# v0.67 验证命令与边界

cmake Release默认Assimp ON，完整构建；ctest --test-dir build --output-on-failure。

bash tools/check_portable.sh /tmp/portable67：严格C++17全套。
python3 tools/check_rebuilt_storage.py：实际保存lambda和恢复包装；--sanitize运行ASan/UBSan（detect_leaks=0）。

状态/痕迹提交故障通过Linux文件系统API替身注入，验证暂停、持久错误状态、内存保留与重试后的文件读取；底层提交抛出异常的既有检查继续保留。check_clean_overlay.py执行实际HUD语句，验证保存错误持续显示与清除。

没有Windows游戏、真实F5键、弹窗、文件系统故障注入、磁盘写满或HD6000验证。退出失败弹窗目前不能取消退出。
