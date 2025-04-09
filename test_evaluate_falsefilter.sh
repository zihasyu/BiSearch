cd bin
run_loop_once(){
  local path=$1
  local name=$2
  local num=$3
  local loop=$4
  ./BiSearch -i $path -c 4 -m 5 -n $num  -a $loop -b 0 >SA_BiSearch${name}_${loop}.txt
    rm Containers/*
# sudo echo 3 > /proc/sys/vm/drop_caches
}
run_method(){
    local path=$1
    local name=$2
    local num=$3
    for loop in $(seq 0 1 20); do
        run_loop_once $path $name $num $loop
    done
}

run_method /mnt/dataset2/Android _Android 36
run_method /mnt/dataset2/automake_tarballs _automake 100
run_method /mnt/dataset2/bash_tarballs _bash 44
run_method /mnt/dataset2/coreutils_tarballs _coreutils 28
run_method /mnt/dataset2/fdisk_tarballs _fdisk 22
run_method /mnt/dataset2/glibc_tarballs _glibc 100
run_method /mnt/dataset2/smalltalk_tarballs _smalltalk 40
run_method /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117
run_method /mnt/dataset2/ThunderbirdTar _Thunderbird 240
run_method /mnt/dataset2/chromium _chromium 107
run_method /mnt/dataset2/linux _linux 270
run_method /mnt/dataset2/WEB _WEB 102

