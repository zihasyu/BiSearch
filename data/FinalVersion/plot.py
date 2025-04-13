# -*- coding: utf-8 -*-
import os
import glob
import numpy as np
import matplotlib
matplotlib.use('Agg')  # 设置为非交互式后端
import matplotlib.pyplot as plt

# 寻找所有匹配的文件
files = glob.glob("FinalVersion_*.txt")
if not files:
    print("未找到符合名称模式的文件")
    exit()

plt.figure(figsize=(12, 8))

# 为每个文件创建一条CDF曲线
for f in files:
    # 从文件读取数据
    values = []
    with open(f, 'r') as fp:
        for line in fp:
            if line.strip():  # 忽略空行
                try:
                    value, _ = line.strip().split()
                    values.append(float(value))
                except ValueError:
                    print("警告: 忽略无效行: '{0}'".format(line.strip()))

    if not values:
        print("文件 {0} 中没有有效数据".format(f))
        continue
    
    # 排序数据以准备CDF计算
    values.sort()
    
    # 计算CDF
    y_values = np.arange(1, len(values) + 1) / float(len(values))  # 确保Python 2中的除法是浮点除法
    
    # 从文件名提取图例标签
    label_name = os.path.basename(f)[len("FinalVersion_"):-4]
    
    # 绘制CDF曲线
    plt.plot(values, y_values, marker='.', label=label_name, linewidth=2, markersize=4)

# 设置图表属性
plt.xlabel("Chunk Overlap Proportion")
plt.ylabel("CDF For Unique Chunk Num")
plt.title("CDF of Chunk Overlap Proportion")
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')

# 设置坐标轴范围（可选，如果数据范围合适的话）
plt.xlim(0, 1.05)
plt.ylim(0, 1.05)

# 调整布局以防止图例被裁剪
plt.tight_layout()

# 保存图像
plt.savefig("cdf_values_plot.png", bbox_inches='tight', dpi=300)
print("图像已保存到 'cdf_values_plot.png'")