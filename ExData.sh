#!/bin/bash
cd bin

# 创建输出文件夹，如果文件夹已存在则不会报错
mkdir -p "ExData"

#1-DCC
grep "DCC:" chunkInfoLog.txt | awk '{print $2}' | paste -sd "," - > "ExData/1-DCC.txt"
#2-DCR
grep "DCR:" chunkInfoLog.txt | awk '{print $2}' | paste -sd "," - > "ExData/2-DCR.txt"
#3-DCE
grep "DCE:" chunkInfoLog.txt | awk '{print $2}' | paste -sd "," - > "ExData/3-DCE.txt"
#4-OCR
grep "Overall Compression Ratio:" chunkInfoLog.txt | awk '{print $4}' | paste -sd "," - > "ExData/4-OCR.txt"
#5-1-Index
grep "Index Overhead:" chunkInfoLog.txt| awk '{gsub("MiB","",$3); print $3}' |paste -sd "," - > "ExData/5-1-Index.txt"
#5-2-Recipe
grep "Recipe Overhead:" chunkInfoLog.txt| awk '{gsub("MiB","",$3); print $3}' |paste -sd "," - > "ExData/5-2-Recipe.txt"
#7-ST
grep "Throughput:" chunkInfoLog.txt | awk '{gsub("MiB/s","",$2); print $2}' | paste -sd "," - > "ExData/7-ST.txt"
