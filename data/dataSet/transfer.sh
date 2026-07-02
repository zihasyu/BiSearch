#!/bin/bash

SRC_DIR="/home/cluster/WZY/BiSy/TOS/BiSearch/data/614/TosGit_restore_chain1"
DST_DIR="/home/cluster/WZY/BiSy/TOS/BiSearch/data/dataSet/depth1"

for src_file in "$SRC_DIR"/*.csv; do
    filename=$(basename "$src_file")
    dst_file="$DST_DIR/$filename"

    if [[ -f "$dst_file" ]]; then
        # 跳过第一行（表头），追加到目标文件末尾
        tail -n +2 "$src_file" >> "$dst_file"
        echo "已追加: $filename"
    else
        echo "跳过（目标不存在）: $filename"
    fi
done