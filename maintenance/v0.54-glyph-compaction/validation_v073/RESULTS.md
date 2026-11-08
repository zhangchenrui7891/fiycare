# v0.73 检查结果

- 基线v0.72：ZIP CRC、2392条文件校验通过；1661条资源及第三方文件保持一致。
- 新增实际main鼠标语句CPU回归：旧版本预期退出1（视角误动），新版退出0。
- Linux Assimp ON Release：完整构建282步；最终47/47 CTest，18.48秒。
- check_portable.sh：退出0，严格C++17编译检查及全部脚本通过。
- cockpit_input_tests ASan/UBSan：退出0，detect_leaks=0；泄漏检查未启用。
- Windows2022 MSVC x64 Release：主程序编译成功；20/20 CTest，1.15秒，8条既有警告。
- 受检代码：c662bfa746c10642f661ec02a96654ecc4277165。
- CI：https://github.com/zhangchenrui7891/fiycare/actions/runs/42497946848
- Job：128784241378，success。
- Win32 API模拟消息检查属于CPU逻辑验证；Windows CI的CPU/GDI/HLSL也不代表游戏启动、渲染和实机交互通过。
- 未测试：Windows资源加载运行、真实鼠标消息、DX11/DX12游戏画面、HD6000帧时/RAM/温度、完整航班。

初始配置失败原因是环境缺cmake，安装后成功；初始新脚手架严格编译遇到既有for/return单行缩进警告，提取时规范换行后通过。旧版回归失败是有意保留的故障复现，详见legacy_pointer_failure.log。
