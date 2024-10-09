#!/bin/bash
#plot 8 & 10
cd bin
# 筛选符合条件的行
input_file="chunkInfoLog.txt"
grep '^./BiSearch -i' chunkInfoLog.txt | awk '$5 == 4 && $7 == 5' | while read -r line; do
    # 获取当前行的行号
    lineno=$(grep -n "$line" chunkInfoLog.txt | cut -d: -f1)
    
    # 提取下一次遇到的dedup reduct size、local reduct size、Feature reduct size、Locality reduct size
    dedup=$(sed -n "$((lineno+1)),\$p" chunkInfoLog.txt | grep -m 1 'dedup reduct size' | awk '{print $5}' | sed 's/MiB//')
    local=$(sed -n "$((lineno+1)),\$p" chunkInfoLog.txt | grep -m 1 'local reduct size' | awk '{print $5}' | sed 's/MiB//')
    feature=$(sed -n "$((lineno+1)),\$p" chunkInfoLog.txt | grep -m 1 'Feature reduct size:' | awk '{print $4}' | sed 's/MiB//')
    locality=$(sed -n "$((lineno+1)),\$p" chunkInfoLog.txt | grep -m 1 'Locality reduct size:' | awk '{print $4}' | sed 's/MiB//')
    
    # 输出结果
        echo -n "$dedup,$local,$feature,$locality," >> ./ExData/8-impact.txt
done

grep '^./BiSearch -i' chunkInfoLog.txt | awk '$5 == 4 && $7 == 5' | while read -r line; do
    # 获取当前行的行号
    lineno=$(grep -n "$line" chunkInfoLog.txt | cut -d: -f1)
    
# 提取并格式化数据
# 提取并格式化数据
    dedup_time=$(sed -n "$((lineno+1)),\$p" $input_file | grep -m 1 'Dedup Time:' | awk '{print $3}' | sed 's/s//')
    locality_match_time=$(sed -n "$((lineno+1)),\$p" $input_file | grep -m 1 'Locality Match Time:' | awk '{print $4}' | sed 's/s//')
    locality_delta_time=$(sed -n "$((lineno+1)),\$p" $input_file | grep -m 1 'Locality Delta Time:' | awk '{print $4}' | sed 's/s//')
    feature_match_time=$(sed -n "$((lineno+1)),\$p" $input_file | grep -m 1 'Feature Match Time:' | awk '{print $4}' | sed 's/s//')
    feature_delta_time=$(sed -n "$((lineno+1)),\$p" $input_file | grep -m 1 'Feature Delta Time:' | awk '{print $4}' | sed 's/s//')
    lz4_compression_time=$(sed -n "$((lineno+1)),\$p" $input_file | grep -m 1 'Lz4 Compression Time:' | awk '{print $4}' | sed 's/s//')

# 输出结果到文件
echo -n "$dedup_time,$lz4_compression_time,$locality_match_time,$locality_delta_time,$feature_match_time,$feature_delta_time," >> ./ExData/10-breakdown.txt
done