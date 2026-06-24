#!/bin/bash

# --- 配置 ---
EXPERIMENT_BASE_DIR="./TosGit_chain50"

# --- 函数定义 ---
# 参数1: 数据集名称
# 参数2: 数据集源路径
# 参数3: 版本数量
process_dataset() {
    local name=$1
    local source_path=$2
    local num_versions=$3
    local repo_path="./$name"
    
    echo "--- 开始处理数据集: $name (模式: 默认 Git 配置 + 恢复测试) ---"
    sync

    # 1. 初始化仓库
    mkdir -p "$repo_path"
    cd "$repo_path" || { echo "错误: 无法进入目录 $repo_path"; exit 1; }
    git init
    git config --local pack.threads 1
    # 【关键修改】不再设置任何 git config
    # Git 将使用系统默认配置 (默认会启用 autoGC, 默认压缩级别等)
    # 例如: gc.auto, gc.autoPackLimit 等均保持默认行为

    # 2. 初始化日志
    local log_file="../${name}_detailed_log.csv"
    echo "Version,LogicalSize_Bytes,CommitTime_Seconds,Throughput_MB/s,LooseObjectsSize_Bytes,LooseMetadataSize_Bytes,LooseTotalSize_Bytes" > "$log_file"
    
    local total_logical_size=0
    local total_commit_time=0

    # 3. 查找并排序 tar 包
    local tar_files
    mapfile -t tar_files < <(find "$source_path" -type f \( -name "*.tar" -o -name "*.tar.gz" -o -name "*.tgz" -o -name "*.tar.bz2" \) | sort -V | head -n "$num_versions")
    
    if [ ${#tar_files[@]} -eq 0 ]; then
        echo "警告: 在 $source_path 中没有找到 tar 包，跳过。"
        cd ..
        return
    fi

    # 4. 循环提交每个版本
    # 【关键修改】因为使用默认配置，Git 可能会在中途自动触发 GC/repack
    for (( i=0; i<${#tar_files[@]}; i++ )); do
        local tar_file="${tar_files[$i]}"
        local version_num=$((i + 1))
        echo " [版本 $version_num/${#tar_files[@]}] 正在提交: $(basename "$tar_file")"
        
        local start_time
        start_time=$(date +%s.%N)
        
        # 清理工作目录并解压
        find . -maxdepth 1 ! -name '.git' ! -name '.' -exec rm -rf {} +
        tar -xf "$tar_file" --strip-components=1 2>/dev/null || tar -xf "$tar_file" 2>/dev/null
        
        local current_logical_size
        current_logical_size=$(du -sb --exclude=.git . | awk '{print $1}')
        total_logical_size=$((total_logical_size + current_logical_size))
        
        # 添加并提交
        git add -A
        # 使用固定的时间戳和作者信息，确保可复现性
        GIT_AUTHOR_DATE="2000-01-01T00:00:00+00:00" \
        GIT_COMMITTER_DATE="2000-01-01T00:00:00+00:00" \
        GIT_AUTHOR_NAME="x" GIT_AUTHOR_EMAIL="x@x" \
        GIT_COMMITTER_NAME="x" GIT_COMMITTER_EMAIL="x@x" \
        git commit -m "v$version_num" --no-gpg-sign > /dev/null
        git repack  -d -l 
        local end_time
        end_time=$(date +%s.%N)
        local time_taken
        time_taken=$(echo "$end_time - $start_time" | bc)
        total_commit_time=$(echo "$total_commit_time + $time_taken" | bc)
        
        # 记录 loose 状态下的大小
        local git_objects_size_bytes
        git_objects_size_bytes=$(du -sb .git/objects | awk '{print $1}')
        local total_physical_size_bytes
        total_physical_size_bytes=$(du -sb .git | awk '{print $1}')
        local git_metadata_size_bytes=$((total_physical_size_bytes - git_objects_size_bytes))
        
        local throughput=0
        if (( $(echo "$time_taken > 0" | bc -l) )); then
            throughput=$(echo "scale=2; ($current_logical_size / 1024 / 1024) / $time_taken" | bc)
        fi
        
        echo "$version_num,$current_logical_size,$time_taken,$throughput,$git_objects_size_bytes,$git_metadata_size_bytes,$total_physical_size_bytes" >> "$log_file"
    done

    # --- 5. 所有版本提交完毕 ---
    # 注意：因为是默认配置，这里不再手动执行 repack
    # Git 可能已经根据 autoGC 规则进行了优化，或者仍然保持松散对象状态
    # 我们直接记录当前状态
    
    local final_git_objects_size
    final_git_objects_size=$(du -sb .git/objects | awk '{print $1}')
    local final_total_physical_size
    final_total_physical_size=$(du -sb .git | awk '{print $1}')
    local final_git_metadata_size=$((final_total_physical_size - final_git_objects_size))
    
    local final_ocr=0
    if (( final_git_objects_size > 0 )); then
        final_ocr=$(echo "scale=4; $total_logical_size / $final_git_objects_size" | bc);
    fi
    
    local final_err=0
    if (( final_total_physical_size > 0 )); then
        final_err=$(echo "scale=4; $total_logical_size / $final_total_physical_size" | bc);
    fi

    # 写入总结
    # 【关键修改】列名标识为 default
    echo "$name-default,$total_logical_size,$final_git_objects_size,$final_git_metadata_size,$final_total_physical_size,$final_ocr,$final_err,$total_commit_time,0,$total_commit_time" >> ../summary_results.csv
    
    echo " [结果] 提交总耗时: ${total_commit_time}s"
    echo " [结果] 最终物理大小: $final_total_physical_size bytes | OCR: $final_ocr | ERR: $final_err"

    # --- 6. 逐个版本恢复测试 ---
    # 清理缓存，模拟独立的恢复操作
    sync
    sudo echo 3 > /proc/sys/vm/drop_caches
    echo " [Restore Test] 开始逐个版本恢复测试..."
    
    local restore_log_file="../${name}_restore_log.csv"
    echo "Version,CommitHash,RestoredLogicalSize_Bytes,RestoreTime_Seconds,RestoreThroughput_MB/s" > "$restore_log_file"
    
    # 获取所有 commit 的 hash，按时间正序排列
    local commit_hashes
    mapfile -t commit_hashes < <(git log --reverse --pretty=format:"%H"; echo)
    
    for (( i=0; i<${#commit_hashes[@]}; i++ )); do
        local commit_hash="${commit_hashes[$i]}"
        local version_num=$((i + 1))
        local restore_start
        restore_start=$(date +%s.%N)
        
        # 清理工作区并检出指定版本
        git reset --hard "$commit_hash"
        
        local restore_end
        restore_end=$(date +%s.%N)
        local restore_time
        restore_time=$(echo "$restore_end - $restore_start" | bc)
        
        local restored_size
        restored_size=$(du -sb --exclude=.git . | awk '{print $1}')
        
        local restore_throughput=0
        if (( $(echo "$restore_time > 0" | bc -l) )); then
            restore_throughput=$(echo "scale=2; ($restored_size / 1024 / 1024) / $restore_time" | bc)
        fi
        
        echo " [Restore $version_num/${#commit_hashes[@]}] 恢复耗时: ${restore_time}s, 吞吐量: ${restore_throughput} MB/s"
        echo "$version_num,$commit_hash,$restored_size,$restore_time,$restore_throughput" >> "$restore_log_file"
        
        # 每次恢复后清理缓存
        sudo echo 3 > /proc/sys/vm/drop_caches
    done
    
    echo "--- 数据集 '$name' 处理完成 ---"
    cd ..
    echo " [清理] 正在删除仓库目录: $repo_path"
    rm -rf "$repo_path"
}

# --- 主程序 ---
echo "正在设置实验环境于 $(pwd)/$EXPERIMENT_BASE_DIR..."
rm -rf "$EXPERIMENT_BASE_DIR"
mkdir -p "$EXPERIMENT_BASE_DIR"
cd "$EXPERIMENT_BASE_DIR" || exit

# 【关键修改】更新 CSV 表头以反映新的模式
echo "Dataset,TotalLogicalSize_Bytes,GitObjectsSize_Bytes,GitMetadataSize_Bytes,TotalPhysicalSize_Bytes,OCR,ERR,CommitTime_Seconds,RepackTime_Seconds,TotalTime_Seconds" > "summary_results.csv"

# --- 运行实验 ---
run_git_experiment() {
    local path=$1
    local name=${2#_}
    local num=$3
    process_dataset "$name" "$path" "$num"
}

# 这里保持了与 tos_git_restore.sh 一致的测试列表
run_git_experiment /mnt/dataset2/linux _linux 270
run_git_experiment /mnt/dataset2/WEB _WEB 102
run_git_experiment /mnt/dataset2/automake_tarballs _automake 100
run_git_experiment /mnt/dataset2/coreutils_tarballs _coreutils 28
run_git_experiment /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117
run_git_experiment /mnt/dataset2/react _react 100
run_git_experiment /mnt/dataset2/netty _netty 99
run_git_experiment /mnt/dataset2/Cpython _Cpython 100

cd ..
echo "--- 所有实验已完成 ---"
echo "总结报告:"
cat "$EXPERIMENT_BASE_DIR/summary_results.csv"
echo "详细逐版本日志保存在 $EXPERIMENT_BASE_DIR/ 中 (例如: react_detailed_log.csv)。"