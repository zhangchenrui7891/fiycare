# 777 性能基准

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

两种 777 参数同时出现时优先使用追尾。省略 `--benchmark` 时这两个修饰参数不起作用。每次在所选目录写出 `startup.log`、`performance_summary.txt` 和基准截图。重跑会替换所选预设和 FXAA 模式的输出。对比 FXAA 时可为同一指令追加 `--no-aa`。只对 `result_status=VALID` 的结果比较平均 FPS 和 P95 帧间隔。数据来自 CPU 墙钟，报告中的场景为初始设定，运行期间的全部场景状态并未锁定。

## v0.87 的内存字段

基准在写摘要和捕获最终截图之前调用 Windows 进程内存接口一次。`working_set_bytes` 为此时驻留在物理内存中的进程工作集，`peak_working_set_bytes` 为进程从启动至查询时的工作集峰值，包含启动和模型加载。字段单位为字节。采集失败时两项均为 `NA`，帧间隔判定独立保留。工作集包含映射的共享页，不能用来推算 GPU 显存或系统总内存使用量。


## v0.88 的 FXAA 对照

用 `build_vs.bat` 编译后，运行 `run_benchmark_777_fxaa_pair.bat`，依次测量 DX11 的 777 驾驶舱 FXAA 开启和关闭。每轮保持窗口在前台，不移动飞机或调设置。脚本要求上一轮正常退出且摘要为 `VALID` 才继续。

原有 FXAA 开启的路径不变。关闭 FXAA 的结果位于 `diagnostics/benchmark/no-fxaa/`，再按驾驶舱、追尾或 A350 默认预设分目录。两边分别保存摘要、BMP、CSV 和启动日志。摘要的 `fxaa` 表示请求的模式，实际帧配置一旦不符即取消该轮判定。可用摘要中的 `p95_ms`、`peak_working_set_bytes` 和截图做比较。

## v0.96 座舱环视检查

驾驶舱基准从首次绘制起记录偏航和俯仰环视角。随后任意一个角度变化，或遇到无效角度，都会令该轮配置失效。即使重新转回原视线，结果也保持 `NOT_EVALUATED`。摘要会记录 `cockpit_view_observed`，并在 `configuration_guard` 中标明新增检查范围。

本检查适用于 A350 和 777 驾驶舱预设。追尾预设继续使用机型、相机模式等原有配置检查。采样时仍需保持同一场景，不操作飞机。日照、飞机位置及全部场景状态尚未纳入锁定。报告的 CPU 墙钟帧间隔与目标机画面需要一起验收。
