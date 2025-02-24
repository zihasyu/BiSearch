cd bin
# sudo rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches

run_raw() {
  local path=$1
  local name=$2
  local num=$3
  ./BiSearch -i $path -c 9 -m 3 -n $num >FileLevel_Odess$name.txt
  # rm -r mTarFile
  # mkdir mTarFile
  # sudo rm Containers/*
  # sudo echo 3 > /proc/sys/vm/drop_caches
  # ./BiSearch -i $path -c 6 -m 3 -n $num  >Mo_Odess$name.txt

  # rm -r mTarFile
  # mkdir mTarFile
  # sudo rm Containers/*
  # sudo echo 3 > /proc/sys/vm/drop_caches
  # ./BiSearch -i $path -c 1 -m 3 -n $num  >FastCDC_Odess$name.txt
  # sudo echo 3 > /proc/sys/vm/drop_caches
}

run_raw /mnt/dataset2/ThunderbirdTar _Thunderbird 240
run_raw /mnt/dataset2/automake_tarballs _automake 100
run_raw /mnt/dataset2/bash_tarballs _bash 44
run_raw /mnt/dataset2/coreutils_tarballs _coreutils 28
run_raw /mnt/dataset2/fdisk_tarballs _fdisk 22
run_raw /mnt/dataset2/glibc_tarballs _glibc 100
run_raw /mnt/dataset2/smalltalk_tarballs _smalltalk 40
run_raw /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117
run_raw /mnt/dataset2/linux _linux 270
run_raw /mnt/dataset2/chromium _chromium 107
run_raw /mnt/dataset2/WEB _WEB 102

