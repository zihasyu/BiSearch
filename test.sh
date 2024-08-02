cd bin
path=/mnt/dataset2/cassandra
name=_cassandra
num=97
./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
./BiSearch -i $path -c 3 -m 5 -n $num  >BiSearchTarfast$name.txt
./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt



# path=/mnt/dataset2/LKT
# name=_LKT
# num=84
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
# ./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
# ./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
# ./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
# ./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
# ./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
# ./BiSearch -i $path -c 3 -m 5 -n $num  >BiSearchTarfast$name.txt
# ./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt

# path=/mnt/dataset2/WEB
# name=_WEB
# num=102
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
# ./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
# ./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
# ./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
# ./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
# ./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
# ./BiSearch -i $path -c 3 -m 5 -n $num  >BiSearchTarfast$name.txt
# ./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt