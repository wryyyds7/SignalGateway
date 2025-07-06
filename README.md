# SignalGateway — 车载信号网关中间件

C++17 车载信号中间件：CAN 信号解析 → 事件总线路由 → SOME/IP 服务发布。配套 GTest 单元测试、AddressSanitizer、GitHub Actions CI（Ubuntu + openEuler）。

## 架构

```
CAN Layer → Signal Router (Event Bus) → SOME/IP Publisher
                    ↓
              IPC Publisher (UDS) → DigitalCluster 仪表
              Signal Recorder (CSV 录制/回放)
              Web Control Panel (浏览器手动控制)
```

## 构建

### Ubuntu

```bash
sudo apt install build-essential cmake
mkdir build && cd build && cmake .. && make -j$(nproc)
```

### openEuler 22.03 LTS

```bash
sudo dnf install gcc gcc-c++ cmake make
mkdir build && cd build && cmake .. && make -j$(nproc)
```

## 运行

```bash
# 启动网关（带 IPC + Web 控制面板）
./build/signal_gateway --ipc /tmp/sg.sock --web 8080 --duration 600

# 浏览器打开 http://localhost:8080 手动控制信号

# 启动 DigitalCluster 仪表
./DigitalCluster --ipc /tmp/sg.sock
```

## 测试

```bash
cd build && ctest --verbose
```

## CI

GitHub Actions 在 Ubuntu 22.04 和 openEuler 22.03 LTS 容器上并行编译 + 测试 + AddressSanitizer。

## 开源参考

- [COVESA/vsomeip](https://github.com/COVESA/vsomeip)
- [automotive-grade-linux/agl-signal-composer](https://gerrit.automotivelinux.org/)
