# Flowrix Beta

Flowrix Beta 是一个以 [florr.io](https://florr.io) 为灵感的多人花瓣战斗游戏。探索不同生态区，击败怪物收集花瓣，通过合成、配装和天赋强化角色。

## 在线游玩

- [进入 Flowrix Beta](http://154.219.105.161/)
- [留言板与修复进度](https://ashflor.com/feedback.html?v=prog5#progress-title)

## 本地运行

仓库包含已构建的 Web 客户端和服务端。安装 Node.js 后，在仓库根目录启动服务。

```sh
node dist/server.js --port 3000 --db dist/inventory.json --web-root dist
```

启动后打开 [http://localhost:3000](http://localhost:3000)。本地账号与存档保存在 `dist/inventory.json`。

## 从源码构建

Web 客户端与服务端都使用 C++，通过 Emscripten 编译为 WebAssembly。安装 Node.js、CMake 和 Emscripten SDK，确保 `emcc` 与 `emcmake` 可用，然后在仓库根目录构建。

```sh
npm run build
```

构建产物写入 `dist/`。修改花瓣、怪物、掉落或地图数据后，客户端和服务端需要一起重新构建和发布。

- [引擎与网络架构](cpp/docs/ARCHITECTURE.md)
- [地图编辑指南](maps/README.md)
- [上游仓库](https://github.com/flowrix-io/florr_clone)

## 许可证

Copyright (c) 2026 sussybite8888

本项目按 [GNU Affero General Public License v3.0 or later](LICENSE) 发布，包含改编自 [trigonal-bacon/gardn](https://github.com/trigonal-bacon/gardn) 的代码。
