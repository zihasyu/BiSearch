#!/bin/bash

# ============================================================
#  tos_git_default_loop5.sh  (方法: chain=50)
#
#  运行 tos_git_default.sh 共 5 轮。
#  每轮遍历全部 8 个数据集（同一数据集不连续跑）。
#  每次运行的 CSV 数据追加到同名累积文件中。
#  最后对每个数据集的 Throughput_MB/s 和 RestoreThroughput_MB/s
#  计算 mean / stddev / 95%CI（z=1.96，标准正态）。
#
#  输出 (RESULT_DIR 下):
#    {ds}_detailed_log.csv       — 累积的写入日志
#    {ds}_restore_log.csv        — 累积的恢复日志
#    {ds}_stats.txt              — mean / stddev / CI95
#    summary_results.csv         — 全部轮次的汇总
# ============================================================

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR" || exit 1

NUM_RUNS=5
METHOD_NAME="chain=50"
RESULT_DIR="./TosGit_default_loop5"
SRC_DIR="./TosGit_chain50"   # 与 tos_git_default.sh 中 EXPERIMENT_BASE_DIR 一致

DATASETS=(linux WEB automake coreutils gcc react netty Cpython)

# ===========================
#  主流程
# ===========================
rm -rf "$RESULT_DIR"
mkdir -p "$RESULT_DIR"

for run in $(seq 1 $NUM_RUNS); do
    echo ""
    echo "============================================================"
    echo "  [${METHOD_NAME}] 第 ${run}/${NUM_RUNS} 轮"
    echo "============================================================"

    # --- 运行基础实验（内部会跑完全部 8 个数据集） ---
    bash tos_git_default.sh

    # --- 逐个数据集收集产物 ---
    for ds in "${DATASETS[@]}"; do
        src_det="${SRC_DIR}/${ds}_detailed_log.csv"
        src_rst="${SRC_DIR}/${ds}_restore_log.csv"
        dst_det="${RESULT_DIR}/${ds}_detailed_log.csv"
        dst_rst="${RESULT_DIR}/${ds}_restore_log.csv"

        # detailed_log: 第1轮写表头+数据，后续只追加数据行
        if [ -f "$src_det" ]; then
            if [ "$run" -eq 1 ]; then
                cp "$src_det" "$dst_det"
            else
                tail -n +2 "$src_det" >> "$dst_det"
            fi
        fi

        # restore_log: 同上
        if [ -f "$src_rst" ]; then
            if [ "$run" -eq 1 ]; then
                cp "$src_rst" "$dst_rst"
            else
                tail -n +2 "$src_rst" >> "$dst_rst"
            fi
        fi

        # summary_results: 追加到共享汇总
        src_sum="${SRC_DIR}/summary_results.csv"
        dst_sum="${RESULT_DIR}/summary_results.csv"
        if [ -f "$src_sum" ]; then
            if [ "$run" -eq 1 ]; then
                cp "$src_sum" "$dst_sum"
            else
                tail -n +2 "$src_sum" >> "$dst_sum"
            fi
        fi

        echo "  [收集] ${ds} (run ${run}) done"
    done
done

# ===========================
#  统计汇总
# ===========================
echo ""
echo "============================================================"
echo "  统计汇总  [${METHOD_NAME}]  (${NUM_RUNS} runs)"
echo "============================================================"

# ----------------------------------------------------------
# compute_stats <csv_file> <col_number>
#   跳过表头，对指定列计算 mean / stddev / 95%CI (z=1.96)
# ----------------------------------------------------------
compute_stats() {
    local file=$1
    local col=$2
    awk -F',' -v col="$col" '
    NR > 1 && $col != "" {
        val = $col + 0
        sum  += val
        sumsq += val * val
        n++
    }
    END {
        if (n == 0) { printf "N=0 Mean=0 StdDev=0 CI95=0\n"; exit }
        mean = sum / n
        var  = sumsq / n - mean * mean
        if (var < 0) var = 0          # 防浮点误差
        sd   = sqrt(var)
        ci   = 1.96 * sd / sqrt(n)
        printf "N=%d Mean=%.4f StdDev=%.4f CI95=%.4f\n", n, mean, sd, ci
    }
    ' "$file"
}

# 提取数值
extract_val() { echo "$1" | grep -oP "$2=\K[^ ]+"; }

for ds in "${DATASETS[@]}"; do
    stats_file="${RESULT_DIR}/${ds}_stats.txt"
    echo "" > "$stats_file"
    echo "=== ${ds}  (${METHOD_NAME}, ${NUM_RUNS} runs) ===" >> "$stats_file"
    echo "" >> "$stats_file"

    echo "--- ${ds} ---"

    det_csv="${RESULT_DIR}/${ds}_detailed_log.csv"
    rst_csv="${RESULT_DIR}/${ds}_restore_log.csv"

    # Throughput_MB/s = 第4列
    if [ -f "$det_csv" ]; then
        tp=$(compute_stats "$det_csv" 4)
        echo "  Throughput_MB/s:        $tp"
        echo "Throughput_MB/s (detailed, col 4):" >> "$stats_file"
        echo "  $tp" >> "$stats_file"
    fi

    # RestoreThroughput_MB/s = 第5列
    if [ -f "$rst_csv" ]; then
        rt=$(compute_stats "$rst_csv" 5)
        echo "  RestoreThroughput_MB/s: $rt"
        echo "" >> "$stats_file"
        echo "RestoreThroughput_MB/s (restore, col 5):" >> "$stats_file"
        echo "  $rt" >> "$stats_file"
    fi
done

echo ""
echo "============================================================"
echo "  全部完成！  输出目录: ${RESULT_DIR}/"
echo "    每个数据集: *_detailed_log.csv  *_restore_log.csv  *_stats.txt"
echo "    汇总: summary_results.csv"
echo "============================================================"
