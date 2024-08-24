cd bin
# path=/mnt/dataset2/LKT
# name=_LKT
# num=84

# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
# ./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
# ./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
# ./BiSearch -i $path -c 1 -m 6 -n $num  >Locality$name.txt

path=/mnt/dataset2/cassandra
name=_cassandra
num=97

./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt

./BiSearch -i $path -c 1 -m 6 -n $num  >Locality$name.txt
