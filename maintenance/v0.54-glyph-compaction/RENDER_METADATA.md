# v0.62 性能日志配置证据

v0.61的performance.csv中low_mode实际输出powerSaver（20FPS限速开关）。例如benchmark固定640×360但不限帧，会写low_mode=0，造成误判；原生与2×超采样也没有尺寸字段可区分。

现在low_mode输出实际low启动配置，追加render_width、render_height、software_frame_cap三列。旧有前12列顺序保留，但low_mode含义已修正；解析器需按表头读取15列，不应固定旧列数。software_frame_cap为20或0（0表示无本程序的软件限帧，不保证无驱动/VSync限制）。

渲染器创建和日志共用launch::renderExtent尺寸计算。日志读取appliedScale，避免本帧菜单请求尚未重建时错误标成新尺寸。窗口/比例重建后沿用原有统计窗口重置逻辑。记录的是成功配置的渲染尺寸，不是独立GPU查询或GPU时间戳。

新增tools/check_render_metadata.py执行生产main的CSV表头/输出语句，检查16组low、限帧、比例及窗口尺寸组合。launch_options_tests补充共享尺寸计算，既有生产createRenderer替身检查继续覆盖DX11/DX12选择和回退。均为CPU检查，未运行游戏。

没有更改默认画质、FXAA、帧率控制、存档、气动或模型。CSV仍是300帧滚动窗口的CPU墙钟统计，切换机型/FXAA等配置可能混合窗口内样本；比较时先稳定设置并等待窗口更新。未新增GPU帧时、内存采样或原生分辨率自动benchmark。

实机验收：固定机场/机型/视角/FXAA，在原生与2×分别运行60秒，保留performance.csv和截图。检查原生1600×900对应1600/900，2×对应3200/1800；--low或benchmark对应640/360。单独核对限帧开关。不要把这些元数据或本轮CPU检查当成HD6000性能证据。
