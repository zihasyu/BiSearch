#!/bin/bash

# --- 配置 ---
EXPERIMENT_BASE_DIR="git_final_experiment"

# --- 函数定义 ---

# 参数1: 数据集名称
# 参数2: 数据集源路径
# 参数3: 版本数量
process_dataset() {
    local name=$1
    local source_path=$2
    local num_versions=$3
    local repo_path="$EXPERIMENT_BASE_DIR/$name"
    local log_file="$EXPERIMENT_BASE_DIR/${name}_throughput_log.csv"

    echo "--- 开始处理数据集: $name ---"
    
    # 1. 初始化仓库和日志
    mkdir -p "$repo_path"
    cd "$repo_path" || { echo "错误: 无法进入目录 $repo_path"; exit 1; }
    git init

    # 2. Git 配置
    git config --local gc.auto 0
    git config --local gc.pruneExpire never
    git config --local pack.useReverseIndex false
    git config --local repack.writeBitmaps false
    
    # 3. 初始化日志文件和统计变量
    echo "Version,LogicalSize_Bytes,Time_Seconds,Throughput_MB/s" > "$log_file"
    local total_logical_size=0

    # 4. 查找并排序 tar 包
    local tar_files
    mapfile -t tar_files < <(find "$source_path" -maxdepth 1 -type f \( -name "*.tar" -o -name "*.tar.gz" -o -name "*.tgz" -o -name "*.tar.bz2" \) | sort -V | head -n "$num_versions")

    if [ ${#tar_files[@]} -eq 0 ]; then
        echo "警告: 在 $source_path 中没有找到 tar 包，跳过。"
        cd ../..
        return
    fi

    # 5. 循环处理每个 tar 包
    for (( i=0; i<${#tar_files[@]}; i++ )); do
        local tar_file="${tar_files[$i]}"
        local version_num=$((i + 1))
        
        local start_time
        start_time=$(date +%s.%N)

        echo "  [版本 $version_num/${#tar_files[@]}] 正在处理: $(basename "$tar_file")"

        local extract_dir="content"
        mkdir -p "$extract_dir"

        tar -xf "$tar_file" -C "$extract_dir" --strip-components=1 2>/dev/null || tar -xf "$tar_file" -C "$extract_dir" 2>/dev/null

        local current_logical_size
        current_logical_size=$(du -sb "$extract_dir" | awk '{print $1}')
        total_logical_size=$((total_logical_size + current_logical_size))

        git add .
        git commit -m "feat: Add version $version_num" --no-gpg-sign > /dev/null

        rm -rf "$extract_dir"/*
        rm -rf "$extract_dir"/.[!.]* "$extract_dir"/..?*
        
        git add .
        git commit -m "chore: Remove content of version $version_num" --no-gpg-sign > /dev/null

        local end_time
        end_time=$(date +%s.%N)
        
        local time_taken
        time_taken=$(echo "$end_time - $start_time" | bc)
        local throughput=0
        if (( $(echo "$time_taken > 0" | bc -l) )); then
            throughput=$(echo "scale=2; ($current_logical_size / 1024 / 1024) / $time_taken" | bc)
        fi
        
        echo "$version_num,$current_logical_size,$time_taken,$throughput" >> "$log_file"
    done

    # 6. 实验结束，计算最终大小和比率
    local git_objects_size_bytes
    git_objects_size_bytes=$(du -sb .git/objects | awk '{print $1}')
    
    local total_physical_size_bytes
    total_physical_size_bytes=$(du -sb .git | awk '{print $1}')

    local git_metadata_size_bytes=$((total_physical_size_bytes - git_objects_size_bytes))

    local ocr=0 # OCR: TotalLogicalSize / GitObjectsSize
    if (( git_objects_size_bytes > 0 )); then
        ocr=$(echo "scale=4; $total_logical_size / $git_objects_size_bytes" | bc)
    fi

    local err=0 # ERR: TotalLogicalSize / TotalPhysicalSize
    if (( total_physical_size_bytes > 0 )); then
        err=$(echo "scale=4; $total_logical_size / $total_physical_size_bytes" | bc)
    fi

    echo "--- 数据集 '$name' 处理完成 ---"
    echo "总逻辑大小: $(numfmt --to=iec-i --suffix=B --format="%.2f" $total_logical_size)"
    echo "物理大小 (.git/objects): $(numfmt --to=iec-i --suffix=B --format="%.2f" $git_objects_size_bytes)"
    echo "元数据大小 (refs, index, etc.): $(numfmt --to=iec-i --suffix=B --format="%.2f" $git_metadata_size_bytes)"
    echo "总物理大小 (.git): $(numfmt --to=iec-i --suffix=B --format="%.2f" $total_physical_size_bytes)"
    echo "OCR (Logical / Objects): $ocr"
    echo "ERR (Logical / Total .git): $err"
    
    # 记录总结结果
    echo "$name,$total_logical_size,$git_objects_size_bytes,$git_metadata_size_bytes,$total_physical_size_bytes,$ocr,$err" >> ../summary_results.csv

    cd ../..
}

# --- 主程序 ---
echo "正在设置实验环境..."
rm -rf "$EXPERIMENT_BASE_DIR"
mkdir -p "$EXPERIMENT_BASE_DIR"
# 更新 CSV 头部以匹配您的要求
echo "Dataset,TotalLogicalSize_Bytes,GitObjectsSize_Bytes,GitMetadataSize_Bytes,TotalPhysicalSize_Bytes,OCR,ERR" > "$EXPERIMENT_BASE_DIR/summary_results.csv"

# --- 运行实验 ---
# 帮助函数，用于调用 process_dataset
run_method() {
    local path=$1
    local name=${2#_} # 移除名称前的下划线
    local num=$3
    process_dataset "$name" "$path" "$num"
}

# 从您的 test.sh 文件中提取的实验列表
run_method /mnt/dataset2/react _react 100
run_method /mnt/dataset2/netty  _netty 99
run_method /mnt/dataset2/Cpython _Cpython 100
run_method /mnt/dataset2/automake_tarballs _automake 100
run_method /mnt/dataset2/coreutils_tarballs _coreutils 28
run_method /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117
run_method /mnt/dataset2/linux _linux 270
run_method /mnt/dataset2/WEB _WEB 102

echo "--- 所有实验已完成 ---"
echo "总结报告:"
cat "$EXPERIMENT_BASE_DIR/summary_results.csv"
echo "详细吞吐量日志保存在 $EXPERIMENT_BASE_DIR/ 中，每个数据集一个文件。"