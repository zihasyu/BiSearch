#!/bin/bash
cd bin

# 创建输出文件夹，如果文件夹已存在则不会报错
mkdir -p "ExData"

#palantirOCR
grep "Overall Compression Ratio:" "Palantir_LKT.txt" | awk -F 'Overall Compression Ratio: ' '{print $2}' | paste -sd "," - > "ExData/palantirOCR.txt"
#OdessOCR
grep "Overall Compression Ratio:" "Odess_LKT.txt" | awk -F 'Overall Compression Ratio: ' '{print $2}' | paste -sd "," - > "ExData/OdessOCR.txt"

#DCC
grep "DCC:" "Palantir_LKT.txt" | awk -F 'DCC: ' '{print $2}' | paste -sd "," - > "ExData/palantirDCC.txt"
#DCC
grep "DCC:" "Odess_LKT.txt" | awk -F 'DCC: ' '{print $2}' | paste -sd "," - > "ExData/OdessDCC.txt"