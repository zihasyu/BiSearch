cd bin

path=/mnt/dataset2/LKT
name=_LKT
num=84
# # ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 1 -m 6 -n $num  >Locality$name.txt
# ./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 3 -m 5 -n $num  >BiSearchTarfast$name.txt
./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches


path=/mnt/dataset2/chromium 
name=_chromium
num=107
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 6 -n $num  >Locality$name.txt
# ./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 3 -m 5 -n $num  >BiSearchTarfast$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches


path=/mnt/dataset2/WEB
name=_WEB
num=102
# # ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 1 -m 6 -n $num  >Locality$name.txt
# ./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 3 -m 5 -n $num  >BiSearchTarfast$name.txt
./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

path=/mnt/dataset2/ThunderbirdTar
name=_Thunderbird
num=240
# # ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 1 -m 6 -n $num  >Locality$name.txt
# ./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 3 -m 5 -n $num  >BiSearchTarfast$name.txt

./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches