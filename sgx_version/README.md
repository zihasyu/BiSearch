# SGX Version — BiSearch Secure Backup

## 目录结构
```
sgx_version/
├── Makefile
├── include/
│   ├── sgx_common.h       — 共享 C 结构体 (ChunkMeta_t, MethodStats_t)
│   └── secure_queue.h     — 模拟 C/S 网络的线程安全队列
├── App/                   — Untrusted 宿主程序
│   ├── main.cc            — 程序入口，Client/Server 线程
│   ├── App.h / App.cpp    — Enclave 初始化 + OCALL 实现
│   ├── client_chunker.h / .cc — Client 端 FASTCDC + AES-GCM 加密
└── Enclave/               — Trusted 数据缩减核心
    ├── Enclave.edl        — ECALL/OCALL 接口定义
    ├── Enclave.config.xml — 内存配置 (256MiB heap)
    ├── Enclave.cpp        — ECALL 入口实现
    ├── enclave_crypto.h   — SGX tcrypto SHA-256 + AES-GCM
    ├── enclave_bisearch.h / .cc — 事件驱动 BiSearch
    └── enclave_datawrite.h / .cc — Container 拼装 + OCALL 落盘
```

## 编译步骤 (在支持 SGX 的远程服务器上)

```bash
cd sgx_version

# 1. 生成测试签名密钥 (仅首次)
make gen_key

# 2. 编译 (默认硬件模式 + 调试)
make -j$(nproc)

# 3. 仿真模式 (无 SGX 硬件的机器上验证逻辑)
make SGX_MODE=SIM -j$(nproc)

# 4. 运行
mkdir -p sgx_Containers
./bisearch_sgx -i /path/to/input/dir -n 3
```

## 依赖
- Intel SGX SDK (`/opt/intel/sgxsdk`)，Ubuntu 20.04/22.04 上安装:
  ```bash
  # 参考 https://github.com/intel/linux-sgx
  sudo apt install libsgx-enclave-common-dev sgx-aesm-service
  ```
- `libssl-dev` (用于 App 侧 AES-GCM 加密)
- `g++` with C++17

## xdelta3 集成
xdelta3 的 `.c` 文件需要以 Trusted 模式编译进 Enclave。在 Makefile 中
`$(LZ4_SRC)` 已复用原项目的 `src/util/lz4.c`。
xdelta3 采用头文件方式，在 `Enclave_C_Flags` 中加入 `-DXDELTA3_MAIN=0` 并
将 `include/xdelta3.h` 作为包含文件即可；若你的 xdelta3 有独立 `.c` 文件，
在 `Enclave_C_Files` 中追加路径。

## 安全说明
- **Client 密钥**：`main.cc` 中 `CLIENT_KEY` 为全 `0xAB` 占位。生产环境请替换为
  通过 SGX Remote Attestation 协商的会话密钥。
- **存储密钥**：Enclave 内部通过 `client_key XOR 0xA5` 推导，演示用途。生产环境
  应使用 `sgx_get_key()` 从 SGX 密封密钥派生。
- **EPC 大小**：`Enclave.config.xml` 配置了 256MiB Heap。处理大型数据集时
  (uniqueChunkNum > 500万) 可能触发 EPC 分页，需适当扩大或分段处理。
