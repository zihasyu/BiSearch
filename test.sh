cd bin

path=/mnt/dataset2/linux
name=_LKT
num=270
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
# sudo rm Containers/*
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 2 -m 3 -n $num  >Odess$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMTar$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 6 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 7 -m 4 -n $num  >PalantirMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 4 -m 5 -n $num  >BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
sudo echo 3 > /proc/sys/vm/drop_caches


path=/mnt/dataset2/chromium 
name=_chromium
num=107
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # # ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
# # # sudo rm Containers/*
# # # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 2 -m 3 -n $num  >Odess$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # # ./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
# # # sudo rm Containers/*
# # # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMTar$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 6 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 7 -m 4 -n $num  >PalantirMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 4 -m 5 -n $num -r 8192 > BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
sudo echo 3 > /proc/sys/vm/drop_caches


path=/mnt/dataset2/WEB
name=_WEB
num=102
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # # ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
# # # sudo rm Containers/*
# # # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 2 -m 3 -n $num  >Odess$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # # ./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
# # # sudo rm Containers/*
# # # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMTar$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 6 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 7 -m 4 -n $num  >PalantirMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 4 -m 5 -n $num -r 8192 > BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
sudo echo 3 > /proc/sys/vm/drop_caches

path=/mnt/dataset2/ThunderbirdTar
name=_Thunderbird
num=240
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # # ./BiSearch -i $path -c 1 -m 1 -n $num  >Ntransform$name.txt
# # # sudo rm Containers/*
# # # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 1 -m 2 -n $num  >Finesse$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 2 -m 3 -n $num  >Odess$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# # # ./BiSearch -i $path -c 1 -m 5 -n $num  >BiSearch$name.txt
# # # sudo rm Containers/*
# # # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# # ./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMTar$name.txt
# # sudo rm Containers/*
# # sudo rm Containers/*
# # sudo rm restoreFile/*
# # sudo rm mtarRestore/*
# # sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 6 -m 3 -n $num  >OdessMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 7 -m 4 -n $num  >PalantirMTar$name.txt
# sudo rm Containers/*
# sudo rm restoreFile/*
# sudo rm mtarRestore/*
# sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 4 -m 5 -n $num -r 8192 > BiSearchMultiTar$name.txt
sudo rm Containers/*
sudo rm restoreFile/*
sudo echo 3 > /proc/sys/vm/drop_caches