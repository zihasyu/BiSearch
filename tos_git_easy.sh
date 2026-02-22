#!/bin/bash

# --- 配置 ---
EXPERIMENT_BASE_DIR="./TosGit"

# --- 函数定义 ---

# 参数1: 数据集名称
# 参数2: 数据集源路径
# 参数3: 版本数量
process_dataset() {
    local name=$1
    local source_path=$2
    local num_versions=$3
    local repo_path="./$name"
    
    echo "--- 开始处理数据集: $name (模式: 自动GC) ---"
    
    # 1. 初始化仓库
    mkdir -p "$repo_path"
    cd "$repo_path" || { echo "错误: 无法进入目录 $repo_path"; exit 1; }
    git init
    git config pack.depth 1
    git config pack.windowMemory 8m
    git config pack.window 2

    # 2. Git 配置 (使用默认GC配置，不禁用)
    # git config --local gc.auto 0  <-- 移除此行以启用自动GC

    # 3. 初始化日志
    local log_file="../${name}_detailed_log.csv"
    echo "Version,LogicalSize_Bytes,CommitTime_Seconds,Throughput_MB/s,PhysicalSize_Bytes" > "$log_file"
    local total_logical_size=0
    local total_commit_time=0

    # 4. 查找并排序 tar 包
    local tar_files
    mapfile -t tar_files < <(find "$source_path" -maxdepth 1 -type f \( -name "*.tar" -o -name "*.tar.gz" -o -name "*.tgz" -o -name "*.tar.bz2" \) | sort -V | head -n "$num_versions")

    if [ ${#tar_files[@]} -eq 0 ]; then
        echo "警告: 在 $source_path 中没有找到 tar 包，跳过。"
        cd ..
        return
    fi

    # 5. 循环提交每个版本 (不手动 repack)
    for (( i=0; i<${#tar_files[@]}; i++ )); do
        local tar_file="${tar_files[$i]}"
        local version_num=$((i + 1))
        
        echo "  [版本 $version_num/${#tar_files[@]}] 正在提交: $(basename "$tar_file")"

        local start_time
        start_time=$(date +%s.%N)

        # 清理工作目录
        find . -maxdepth 1 ! -name '.git' ! -name '.' -exec rm -rf {} +
        # 解压
        tar -xf "$tar_file" --strip-components=1 2>/dev/null || tar -xf "$tar_file" 2>/dev/null

        local current_logical_size
        current_logical_size=$(du -sb --exclude=.git . | awk '{print $1}')
        total_logical_size=$((total_logical_size + current_logical_size))

        # 添加并提交
        git add -A
        git commit -m "v$version_num" --no-gpg-sign > /dev/null

        # --- 不执行手动 repack ---
        # git repack ...

        local end_time
        end_time=$(date +%s.%N)
        
        local time_taken
        time_taken=$(echo "$end_time - $start_time" | bc)
        total_commit_time=$(echo "$total_commit_time + $time_taken" | bc)

        # 记录大小
        local physical_size_bytes
        physical_size_bytes=$(du -sb .git | awk '{print $1}')

        local throughput=0
        if (( $(echo "$time_taken > 0" | bc -l) )); then
            throughput=$(echo "scale=2; ($current_logical_size / 1024 / 1024) / $time_taken" | bc)
        fi
        
        echo "$version_num,$current_logical_size,$time_taken,$throughput,$physical_size_bytes" >> "$log_file"
    done



    # --- 6. 记录最终状态 ---
    local final_physical_size
    final_physical_size=$(du -sb .git | awk '{print $1}')
    
    local final_err=0
    if (( final_physical_size > 0 )); then final_err=$(echo "scale=4; $total_logical_size / $final_physical_size" | bc); fi

    # 写入总结
    echo "$name-auto-gc,$total_logical_size,$final_physical_size,$final_err,$total_commit_time" >> ../summary_results.csv

    echo "  [结果] 总耗时: ${total_commit_time}s | 最终物理大小: $final_physical_size bytes | ERR: $final_err"
    echo "--- 数据集 '$name' 处理完成 ---"
    cd ..
}

# --- 主程序 ---
echo "正在设置实验环境于 $(pwd)/$EXPERIMENT_BASE_DIR..."

mkdir -p "$EXPERIMENT_BASE_DIR"
cd "$EXPERIMENT_BASE_DIR" || exit
echo "Dataset,TotalLogicalSize_Bytes,TotalPhysicalSize_Bytes,ERR,TotalTime_Seconds" > "summary_results.csv"

# --- 运行实验 ---
run_git_experiment() {
    local path=$1
    local name=${2#_}
    local num=$3
    process_dataset "$name" "$path" "$num"
}

run_git_experiment /mnt/dataset2/react _react 100
run_git_experiment /mnt/dataset2/netty _netty 99
run_git_experiment /mnt/dataset2/Cpython _Cpython 100
run_git_experiment /mnt/dataset2/automake_tarballs _automake 100
run_git_experiment /mnt/dataset2/coreutils_tarballs _coreutils 28
run_git_experiment /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117
run_git_experiment /mnt/dataset2/linux _linux 270
run_git_experiment /mnt/dataset2/WEB _WEB 102
run_git_experiment /mnt/dataset2/cross_gcc _cross_gcc 212

cd ..

echo "--- 所有实验已完成 ---"
echo "总结报告:"
cat "$EXPERIMENT_BASE_DIR/summary_results.csv"
echo "详细逐版本日志保存在 $EXPERIMENT_BASE_DIR/ 中。"