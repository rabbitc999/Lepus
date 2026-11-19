# Lepus MCP Server

这是一个标准 MCP stdio 服务，为 AI 编写 Lepus 提供 `lepus_write`、`lepus_run`、`lepus_man` 和 `lepus_new` 工具，并暴露内嵌的 `lepus-programming` Skill。

## 构建与打包

在仓库根目录执行：

```powershell
cd plugins/mcp-lepus
node scripts/package.mjs
```

脚本会先把仓库中的 `skills/lepus-programming/SKILL.md` 复制到 `skill-source/SKILL.md`（原文件副本），再生成 `generated/skill-content.mjs`，最后把真实 Lepus 编译器复制到 `bin/lepus.exe`。编译器来源可用 `LEPUS_COMPILER` 或 `LEPUS_BIN` 指定，也会尝试 `vscode/bin/` 和 PATH。

## MCP 配置

```json
{
  "mcpServers": {
    "lepus": {
      "command": "node",
      "args": ["D:/path/to/plugins/mcp-lepus/server.mjs"]
    }
  }
}
```

服务使用 JSON-RPC 2.0 over stdio，日志写入 stderr，不污染协议输出。相对路径以 `LEPUS_WORKSPACE`（未设置时为当前目录）为基准，写入限制在工作区内。
