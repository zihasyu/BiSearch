# --- 配置 ---
# R 参数: 0, 0.1, 0.2, ..., 1.0
# R_VALUES=(0 0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0)
R_VALUES=(0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8 0.9 1.0)
# B 参数: 8k, 16k, 32k, 64k, 128k, 256k, 512k, 1m, 2m, 4m
B_VALUES=(8192 16384 32768 65536 131072 262144 524288 1048576 2097152 4194304)

# run_method /mnt/dataset2/FinTech/ethereum _ethereum 222
# run_method /mnt/dataset2/FinTech/opa _opa 183
# run_method /mnt/dataset2/FinTech/strata _strata 158
# run_method /mnt/dataset2/FinTech/fabric _fabric 89
# run_method /mnt/dataset2/FinTech/bitcoin _bitcoin 60
# run_method /mnt/dataset2/FinTech/graphNode _graphnode 67
# run_method /mnt/dataset2/FinTech/backtrader _backtrader 139
# run_method /mnt/dataset2/FinTech/lean _lean 500
DATASETS=(
    # "/mnt/dataset2/react _react 100"
    # "/mnt/dataset2/netty _netty 99"
    # "/mnt/dataset2/Cpython _Cpython 100"
    # "/mnt/dataset2/automake_tarballs _automake 100"
    # "/mnt/dataset2/coreutils_tarballs _coreutils 28"
    # "/mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117"
    # "/mnt/dataset2/linux _linux 270"
    # "/mnt/dataset2/WEB _WEB 102"
    "/mnt/dataset2/FinTech/ethereum _ethereum 222"
    "/mnt/dataset2/FinTech/opa _opa 183"
    "/mnt/dataset2/FinTech/strata _strata 158"
    "/mnt/dataset2/FinTech/fabric _fabric 89"
    "/mnt/dataset2/FinTech/bitcoin _bitcoin 60"
    "/mnt/dataset2/FinTech/graphNode _graphnode 67"
    "/mnt/dataset2/FinTech/backtrader _backtrader 139"
    "/mnt/dataset2/FinTech/lean _lean 500"
)
BASE_CMD="./BiSearch -c 4 -m 5"

# --- 核心执行函数 ---
run_all_datasets() {
    local param_flag=$1 # 参数名, e.g., "-r"
    local param_val=$2  # 参数值, e.g., "0.1"
    local param_name=$3 # 用于文件名的参数部分, e.g., "r0.1"

    echo -e "\n--- Testing with $param_flag = $param_val ---"

    for dataset_info in "${DATASETS[@]}"; do
        read -r path name num <<< "$dataset_info"
        output_file="SA_BiSearch_${param_name}${name}.txt"
        
        echo "Running on $name -> $output_file"
        $BASE_CMD -i "$path" -n "$num" "$param_flag" "$param_val" > "$output_file"

        # 清理
        # sudo rm -r mtarRestore restoreFile >/dev/null 2>&1
        # sudo mkdir mtarRestore restoreFile
        sudo rm Containers/* >/dev/null 2>&1
        sudo echo 3 > /proc/sys/vm/drop_caches
        sleep 1
    done
}

# --- 主程序 ---
# 如果 BiSearch 不在当前目录，尝试进入 bin 目录
if [ ! -f "./BiSearch" ] && [ -d "./bin" ] && [ -f "./bin/BiSearch" ]; then
    echo "Changing directory to ./bin"
    cd bin
fi

# # 实验 2: 测试 -B 的影响
# echo "--- STARTING EXPERIMENT 2: Impact of -B ---"
# for val in "${B_VALUES[@]}"; do
#     # 动态生成文件名 (e.g., 512K, 1M)
#     if [ "$val" -ge 1048576 ]; then
#         name="$((val / 1024 / 1024))M"
#     else
#         name="$((val / 1024))K"
#     fi
#     run_all_datasets "-B" "$val" "B$name"
# done


# 实验 1: 测试 -r 的影响
echo "--- STARTING EXPERIMENT 1: Impact of -r ---"
for val in "${R_VALUES[@]}"; do
    run_all_datasets "-r" "$val" "r$val"
done



echo -e "\n--- All experiments completed. ---"