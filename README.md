# TarReduce

## Introduction

TarReduce is a high-performance deduplication system optimized for TAR archive workloads. It employs content-defined chunking, fingerprinting. The system supports various chunking algorithms (FastCDC, mtar, semantics-aware Chunking(ours)) and deduplication methods (N-trans, Finesse, Odess, TarReduce (ours), etc.).

## Building

### Prerequisites

**System Requirements:**
- Ubuntu 20.04
- Clang 
- CMake 3.10+

**Install Dependencies:**

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    clang \
    libboost-all-dev \
    libssl-dev \
    zlib1g-dev
```


### Compilation
```bash
cd ./Script
bash setup.sh
```

## Usage

### Basic Command
```bash
./TarReduce -i <input_directory> -c 4 -m 5 -n <backup_count>
```
### Required Parameters

| Parameter | Description |
|-----------|-------------|
| `-i` | Input directory containing backup files |
| `-c` | Chunking type (see table below) |
| `-m` | Deduplication method (see table below) |
| `-n` | Number of backup versions to process |


#### Chunking Types (`-c`)

| Value | Name | Description |
|-------|------|-------------|
| 0 | Fixed-size | Fixed-size chunking |
| 1 | FastCDC | FastCDC chunking |
| 2 | Gear-CDC | Gear-based CDC chunking |
| 4 | SA (ours) | semantics-aware Chunking |
| 5 | mtar | mtar chunking |

#### Deduplication Method (`-m`)
| Value | Name | Description |
|-------|------|-------------|
| 0 | Dedup | Basic deduplication |
| 1 | N-Trans | N-Transform algorithm |
| 2 | Finesse | Finesse algorithm |
| 3 | Odess | Odess algorithm |
| 5 | TarReduce (ours)| Hybrid approach + Adaptive mechanism. It can only be used with -c 4 |

### Optional Parameters:

| Parameter | Description | Default |
|-----------|-------------|---------|
| `-r` | Adaptive mechanism's alpha | 0.1 |
| `-a` | Accept threshold for Adaptive mechanism's alpha | - |
| `-b` | False filter mode: 0=fixed threshold, 1=Adaptive mechanism's alpha | 1 |
| `-t` | Name hash: 0=disabled, 1=enabled | 1 |
| `-H` | Multi-header chunk count | 16 |
| `-B` | Big chunk size in bytes (for TarReduce) | 4\*1024\*1024 |
| `-R` | Should a restore experiment be conducted? 0=no, 1=yes | 0 |


