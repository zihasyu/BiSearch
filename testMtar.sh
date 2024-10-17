cd bin

path=/mnt/dataset2/LKT
name=_LKT
num=84

rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 1 -n $num  >NtransformMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 4 -n $num  >PalantirMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 5 -n $num  >BiSearchMtar$name.txt
rm -r mTarFile
mkdir mTarFile

path=/mnt/dataset2/chromium
name=_chromium
num=107

rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 1 -n $num  >NtransformMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 4 -n $num  >PalantirMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 5 -n $num  >BiSearchMtar$name.txt
rm -r mTarFile
mkdir mTarFile


path=/mnt/dataset2/WEB
name=_WEB
num=102
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 1 -n $num  >NtransformMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 4 -n $num  >PalantirMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 5 -n $num  >BiSearchMtar$name.txt
rm -r mTarFile
mkdir mTarFile

path=/mnt/dataset2/ThunderbirdTar
name=_Thunderbird
num=240
rm -r mTarFile
mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 1 -n $num  >NtransformMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 4 -n $num  >PalantirMtar$name.txt
rm -r mTarFile
mkdir mTarFile

sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 5 -m 5 -n $num  >BiSearchMtar$name.txt
rm -r mTarFile
mkdir mTarFile