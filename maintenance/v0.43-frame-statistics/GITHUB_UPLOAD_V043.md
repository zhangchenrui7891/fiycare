# v0.43 GitHub 上传范围

用户本轮已明确要求上传至 zhangchenrui7891/fiycare，独立分支为codex/v0.43-frame-statistics。旧main与v0.3工程保留。

当前环境可以读取Git仓库，但git push无法取得HTTPS用户名/凭据。已连接的GitHub接口可写，因此上传本轮变更集到maintenance/v0.43-frame-statistics，含实际performance.h、独立CPU回归、探针、与完整v0.42恢复包的集成补丁、说明及新验证日志。

此分支不是完整v0.43工程镜像：没有经接口重复上传约40MB的二进制资源和约23MB文本依赖，没有配置指向不完整工程的Windows CI。完整源码、资源、依赖、历史记录随FlightCareerAir_v0.43.zip交付。要在GitHub完整构建，需要以有凭据的Git环境提交这个完整包的内容；不能在v0.3上直接应用本次v0.42补丁。

没有宣称Windows云端构建通过，HD6000实机验收仍待完成。本次接口提交不绕过曾经main发布拒绝；使用本轮明确授权的新分支。
