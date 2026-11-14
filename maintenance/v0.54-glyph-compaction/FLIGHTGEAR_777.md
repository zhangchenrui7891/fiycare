# FlightCareerAir v0.81

主菜单以飞行简报为首页。左侧集中开始、机库、任务、设置、存读档与退出操作，右侧显示当前机型、载客、任务和生涯积分。原来的网站入口更新为公开机库网站。

在 Visual Studio 中按原工程步骤构建并启动，停稳时打开「选择飞机」，选择「驾驶 777-300ER」。用追尾或俯视视角查看机身贴图。777 使用现有通用座舱与原型飞行参数。起落架采用显隐，FlightGear 脚本及其座舱模型未接入。

模型来自 FlightGear Aircraft-2024 的 777 ZIP。原始 AC3D、涂装、对应 XML、作者名单和 GPL v2 许可证保存在 assets/source/flightgear777，完整上游包可从 https://mirrors.ibiblio.org/flightgear/ftp/Aircraft-2024/777.zip 下载。派生模型 b777.fcmesh 和贴图 b777.fctex 保留该许可。

使用开启 Assimp 的构建生成 import_fg777，传入原始 777-300ER.ac 和输出 b777.fcmesh。贴图把 paint1.png 缩到 1024×1024 RGBA，并按现有 FCTEX005 格式保存。DX11 和 DX12 使用独立贴图层和 mipmap。离线转换剔除了退化面和灯光精灵。30,516 个三角面原需变换 91,548 个角点，缓存后每次绘制变换 23,447 个唯一顶点。提交给 GPU 的三角面数量不变。

验证记录见 validation_v081。Windows 游戏画面、启动以及 HD 6000 的帧率需要实机验证。源码包包含运行资源，无 EXE。
