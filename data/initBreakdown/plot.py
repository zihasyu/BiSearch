# -*- coding: utf-8 -*-
import os
import glob
import re
import matplotlib
import codecs  # 添加 codecs 模块
matplotlib.use('Agg')  # 在导入pyplot之前设置后端
import matplotlib.pyplot as plt

# 寻找所有匹配的文件
files = glob.glob("Dedup_Skip_*.txt")
if not files:
    print("未找到符合名称模式的文件")
    exit()

plt.figure(figsize=(10, 6))
for f in files:
    with codecs.open(f, "r", encoding="utf-8") as fp:  # 使用 codecs.open 替代 open
        content = fp.read()
    # 提取形如 "casecount is 数字" 的行
    matches = re.findall(r'casecount\s+is\s+(\d+)', content)
    if not matches:
        continue
    y_values = [int(v) for v in matches]
    x_values = list(range(1, len(y_values) + 1))
    
    # 使用文件名的一部分作为图例
    basename = os.path.basename(f)
    if basename.startswith("Dedup_Skip_") and basename.endswith(".txt"):
        label_name = basename[len("Dedup_Skip_"):-4]
    else:
        label_name = basename

    plt.plot(x_values, y_values, marker='o', label=label_name)

plt.xlabel("Backup")
plt.ylabel("Case")
plt.title("casecount")
plt.legend()
plt.grid(True)
# 保存图像到本地，而不展示
plt.savefig("casecount_plot.png")
print("图像已保存到 'casecount_plot.png'")