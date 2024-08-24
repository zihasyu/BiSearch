#!/bin/bash

# 使用awk处理文件
result=$(awk -F, '
{
  if ($5 != 0 && $5 != 1 && $3 ~ /^[0-9]+(\.[0-9]+)?$/ && $4 ~ /^[0-9]+(\.[0-9]+)?$/ && $4 != 0) {
    sum += $3 / $4
    count++
  }
}
END {
  print sum, count
}' C1_M6_chunkIndex.txt)

# 分割结果
sum=$(echo $result | cut -d' ' -f1)
count=$(echo $result | cut -d' ' -f2)

# 确保 sum 和 count 是有效的数字
sum=$(echo $sum | awk '{printf "%.2f", $0}')
count=$(echo $count | awk '{printf "%d", $0}')

# 计算平均值
if [ "$count" -ne 0 ]; then
  avg=$(echo "scale=2; $sum / $count" | bc)
else
  avg=0
fi

# 输出累加结果和次数
echo "sum: $sum"
echo "count: $count"
echo "avg delta ratio: $avg"