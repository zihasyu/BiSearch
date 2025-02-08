cd bin
path=/mnt/dataset2/chromium 
name=_chromium
num=107
# 1 KiB
./BiSearch -i $path -c 4 -m 5 -n $num -r 1024 >BiSearchMultiTar1$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
# 2 KiB
./BiSearch -i $path -c 4 -m 5 -n $num -r 2048 >BiSearchMultiTar2$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
# 3 KiB
./BiSearch -i $path -c 4 -m 5 -n $num -r 3072 >BiSearchMultiTar3$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
# 4 KiB
./BiSearch -i $path -c 4 -m 5 -n $num -r 4096 >BiSearchMultiTar4$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
# 5 KiB
./BiSearch -i $path -c 4 -m 5 -n $num -r 5120 >BiSearchMultiTar5$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
# 6 KiB
./BiSearch -i $path -c 4 -m 5 -n $num -r 6144 >BiSearchMultiTar6$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
# 7 KiB
./BiSearch -i $path -c 4 -m 5 -n $num -r 7168 >BiSearchMultiTar7$name.txt
sudo rm Containers/*
sudo rm restoreFile/*