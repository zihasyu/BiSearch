cd bin
method=0
chunking=6

name=LKT
num=84
./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt
mv cp_values.txt cp_values_LKT.txt

name=ThunderbirdTar
num=240
./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt
mv cp_values.txt cp_values_ThunderbirdTar.txt

name=WEB
num=102
./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt
mv cp_values.txt cp_values_WEB.txt

name=chromium
num=107
./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt
mv cp_values.txt cp_values_chromium.txt


# name=cassandra
# num=97
# ./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt

