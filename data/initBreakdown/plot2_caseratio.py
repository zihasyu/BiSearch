# -*- coding: utf-8 -*-
import os
import glob
import re
import matplotlib
import codecs  # 添加 codecs 模块
matplotlib.use('Agg')  # 在导入pyplot之前设置后端
import matplotlib.pyplot as plt
import numpy as np  # 添加NumPy导入
# 寻找所有匹配的文件
files = glob.glob("Dedup_Skip_*.txt")
if not files:
    print("未找到符合名称模式的文件")
    exit()

plt.figure(figsize=(12, 8))

for f in files:
    with codecs.open(f, 'r', encoding='utf-8') as fp:  # 使用 codecs.open 替代 open
        content = fp.read()
        
    # 使用正则表达式提取每个version小节的数据
    sections = re.findall(r'dedup done.*?-{17}END-{31}', content, re.DOTALL)
    
    ratios = []
    for section in sections:
        # 提取casecount和unique chunk num
        casecount_match = re.search(r'casecount is (\d+)', section)
        unique_chunk_match = re.search(r'unique chunk num: (\d+)', section)
        
        if casecount_match and unique_chunk_match:
            casecount = float(casecount_match.group(1))
            unique_chunk = float(unique_chunk_match.group(1))
            ratio = casecount / unique_chunk
            ratios.append(ratio)
    
    # 使用version序号作为x轴
    x_values = list(range(0, len(ratios) ))
    x_values = [i*100/ len(ratios) for i in x_values]
    # version_count = len(ratios)
    # x_values = [i / version_count for i in range(1, version_count + 1)]
    
    # 从文件名提取图例标签
    label_name = os.path.basename(f)[len("Dedup_Skip_"):-4]
    # 绘制折线图
    plt.plot(x_values, ratios, marker='o', label=label_name, linewidth=2, markersize=4)

plt.xlabel("CDF For Version Number (%)")
plt.ylabel("Casecount / Unique Chunk Num")
plt.title("Casecount to Unique Chunk Number Ratio Over Versions")
plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
plt.grid(True)

# 调整布局以防止图例被裁剪
plt.tight_layout()

# 保存图像
plt.savefig("casecount_ratio_plot.png", bbox_inches='tight', dpi=300)
print("图像已保存到 'casecount_ratio_plot.png'")