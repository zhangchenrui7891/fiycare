# v0.59 HDG接管NAV

问题：NAV每个物理步更新目标航向并将headingHold设为true。旧版HDG按钮只翻转headingHold，不清除navigationHold，因此无法通过HDG退出NAV，下一物理步又被NAV接管。

新行为：NAV工作时按HDG，退出NAV、启用HDG并捕获当前机头航向；不在NAV时保留原来的HDG开关行为。ALT模式独立保留，航向增减按钮调整的新目标不再被NAV覆盖。NAV按钮原有关闭后保留航向保持的行为未变。失电与手动横向输入仍退出相应模式。此为项目简化自动驾驶逻辑，不代表A330/A350真实FCU/FMS。

代码位于src/cockpit_controls.h，通过原有座舱鼠标事件入口调用；不改27控件位置，不增加每帧分配或资源，不宣称FPS提高。存档格式不变。

autopilot_mode_tests.cpp覆盖10机型×2物理模式的20组模式切换链路、2000次后续控制更新；检查HDG接管、再次关闭、重新选择NAV、ALT独立、失电、手动干预和不一致NAV状态恢复。状态由测试构造，并非物理航迹或实机鼠标测试。旧版运行新增测试在接管断言失败，日志在validation_v059/old_failure.log。

待Windows人工验收：带资源构建启动，起飞并以安全速度进入NAV；点击HDG，观察NAV熄灭、HDG保留且目标为当前航向；调节目标航向，确认不会被航点导航覆盖；再次点HDG退出横向控制，ALT保持。再检查NAV重选、断电和手动输入。记录真实画面和目标Boot Camp机器同场景60秒帧时/RAM，不能以本轮CPU测试替代。
