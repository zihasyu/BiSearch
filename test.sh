cd bin
run_method(){
  local path=$1
  local name=$2
  local num=$3
# ./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches

# ./BiSearch -i $path -c 1 -m 2 -n $num  >FinesseFastCDC$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches

# ./BiSearch -i $path -c 1 -m 3 -n $num  >OdessFastCDC$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches

# ./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches



# ./BiSearch -i $path -c 5 -m 2 -n $num  >FinesseMTar$name.txt
# sudo rm -r mTarFile
# sudo mkdir mTarFile
# sudo rm -r mtarRestore
# sudo mkdir mtarRestore
# sudo rm -r restoreFile
# sudo mkdir restoreFile
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 8 -m 3 -n $num  -R 1 >OdessMTar$name.txt
sudo rm -r mTarFile
sudo mkdir mTarFile
sudo rm -r mtarRestore
sudo mkdir mtarRestore
sudo rm -r restoreFile
sudo mkdir restoreFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 8 -m 0 -n $num  -R 1 >LZ4MTar$name.txt
sudo rm -r mTarFile
sudo mkdir mTarFile
sudo rm -r mtarRestore
sudo mkdir mtarRestore
sudo rm -r restoreFile
sudo mkdir restoreFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 5 -m 4 -n $num  >PalantirMTar$name.txt
# sudo rm -r mTarFile
# sudo mkdir mTarFile
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches

# ./BiSearch -i $path -c 4 -m 3 -n $num  >SA_Odess$name.txt
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
./BiSearch -i $path -c 4 -m 5 -n $num -R 1 >SA_BiSearch_Restore$name.txt
sudo rm -r mtarRestore
sudo mkdir mtarRestore
sudo rm -r restoreFile
sudo mkdir restoreFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
# ./BiSearch -i $path -c 4 -m 5 -n $num  -R 1 -L 0 >SA_BiSearch_deflate_Restore$name.txt
./BiSearch -i $path -c 4 -m 3 -n $num  >SA_Ode$name.txt
sudo rm -r mtarRestore
sudo mkdir mtarRestore
sudo rm -r restoreFile
sudo mkdir restoreFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
}

run_method /mnt/dataset2/cross_c++_tar _cross_gcc_tar 317
# run_method /mnt/dataset2/react _react 100
# run_method /mnt/dataset2/netty  _netty 99
# run_method /mnt/dataset2/Cpython _Cpython 100
# run_method /mnt/dataset2/automake_tarballs _automake 100
# run_method /mnt/dataset2/coreutils_tarballs _coreutils 28
# run_method /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117
# run_method /mnt/dataset2/linux _linux 270
# run_method /mnt/dataset2/WEB _WEB 102




# run_method /mnt/dataset2/cross_gcc _cross_gcc 212
# run_method /mnt/dataset2/bash_tarballs _bash 44
# run_method /mnt/dataset2/fdisk_tarballs _fdisk 22
# run_method /mnt/dataset2/glibc_tarballs _glibc 100
# run_method /mnt/dataset2/smalltalk_tarballs _smalltalk 40

# run_method /mnt/dataset2/chromium _chromium 107



# run_method /mnt/dataset2/Windows Windows 738
# run_method /mnt/dataset2/Android _Android 36
# run_method /mnt/dataset2/ThunderbirdTar _Thunderbird 240