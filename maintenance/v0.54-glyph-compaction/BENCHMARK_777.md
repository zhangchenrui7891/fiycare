# v0.84 的 777 性能基准

从 Visual Studio 构建含运行资源的 Release 版本后，在 PowerShell 中运行以下任一命令。每次保持窗口在前台约 60 秒，采样期间不要操作飞机或切换视角。

```powershell
.\out\build\release\FlightCareerAir.exe --dx11 --benchmark --benchmark-777-cockpit
.\out\build\release\FlightCareerAir.exe --dx11 --benchmark --benchmark-777-chase
```

两个预设均从浦东、正午、640×360 低负载设置开始。驾驶舱使用当前通用座舱。追尾绘制 777 外模，缺少可加载的 `assets/b777.fcmesh` 时提示错误。若涂装不可用而使用中性材质，`startup.log` 会记录原因，比较画面或帧时间前请检查该日志。

| 参数 | 起始场景 | 输出目录（相对于 `%LOCALAPPDATA%\FlightCareerAir\diagnostics\benchmark`） |
|---|---|---|
| `--benchmark` | A350 驾驶舱 | 根目录 |
| `--benchmark --benchmark-777-cockpit` | 777-300ER 驾驶舱 | `777-cockpit` |
| `--benchmark --benchmark-777-chase` | 777-300ER 追尾 | `777-chase` |

两种 777 参数同时出现时优先使用追尾。省略 `--benchmark` 时这两个修饰参数不起作用。每次在所选目录写出 `startup.log`、`performance_summary.txt` 和基准截图。重跑前会清除该目录中上一次基准摘要和截图，先复制需要保留的对照数据。对比 FXAA 时可为同一指令追加 `--no-aa`。只对 `result_status=VALID` 的结果比较平均 FPS 和 P95 帧间隔。数据来自 CPU 墙钟，报告中的场景为初始设定，运行期间的全部场景状态并未锁定。
