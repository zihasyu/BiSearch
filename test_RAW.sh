cd bin
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches


path=/mnt/dataset2/linux
name=_linux
num=270
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 6 -m 0 -n $num  >Mo_Dedup$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 7 -m 3 -n $num  >Mo_Odess$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 6 -m 4 -n $num  >Mo_Palantir$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

path=/mnt/dataset2/chromium
name=_chromium
num=107
./BiSearch -i $path -c 6 -m 0 -n $num  >Mo_Dedup$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 7 -m 3 -n $num  >Mo_Odess$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 6 -m 4 -n $num  >Mo_Palantir$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

path=/mnt/dataset2/WEB
name=_WEB
num=102

./BiSearch -i $path -c 6 -m 0 -n $num  >Mo_Dedup$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 7 -m 3 -n $num  >Mo_Odess$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 6 -m 4 -n $num  >Mo_Palantir$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

path=/mnt/dataset2/ThunderbirdTar
name=_Thunderbird
num=240

./BiSearch -i $path -c 6 -m 0 -n $num  >Mo_Dedup$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 7 -m 3 -n $num  >Mo_Odess$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 6 -m 4 -n $num  >Mo_Palantir$name.txt
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches