# v0.66 验证索引

- baseline.log：原ZIP CRC/SHA256与assets/third_party逐字节比较。
- storage.log：python3 tools/check_rebuilt_storage.py，实际生产lambda，文件系统API使用CPU替身。
- storage_sanitized.log：同脚本 --sanitize，ASan/UBSan，detect_leaks=0。
- old_storage.log：同脚本传入原ZIP提取的v0.65 main，旧版在痕迹替换失败断言退出1。
- configure.log/build.log/ctest.log：cmake Release，默认Assimp ON，完整构建与CTest。
- portable.log：bash tools/check_portable.sh /tmp/portable66，严格C++17全套。
- windows_excerpt.log：Windows主程序编译及CPU检查关键行；Assimp OFF、资源复制OFF，非游戏实机。

未进行Windows真实文件系统失败、磁盘写满、断电、窗口交互、GPU、HD6000或FPS测量。保存错误仍沿用顶层报错退出；不宣称不中断重试或跨文件原子事务。
