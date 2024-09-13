cd bin
method=5
chunking=4

# name=LKT
# num=84
# ./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt

name=WEB
num=102
./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt

# name=cassandra
# num=97
# ./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt

# name=ThunderbirdTar
# num=240
# ./BiSearch -i /mnt/dataset2/$name -c $chunking -m $method -n $num  >C"${chunking}M${method}_${name}".txt