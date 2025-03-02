cd bin
run_BiPal(){
  local path=$1
  local name=$2
  local num=$3
./BiSearch -i $path -c 1 -m 0 -n 2  > FastCDC_$name.txt
mv SkipCp.txt SkipCp_$name.txt
mv AcceptCp.txt AcceptCp_$name.txt
mv ForceCp.txt ForceCp_$name.txt
}
# run_raw /mnt/dataset2/ThunderbirdTar _Thunderbird 240
# run_raw /mnt/dataset2/automake_tarballs _automake 100
# run_raw /mnt/dataset2/bash_tarballs _bash 44
# run_raw /mnt/dataset2/coreutils_tarballs _coreutils 28
# run_raw /mnt/dataset2/fdisk_tarballs _fdisk 22
# run_raw /mnt/dataset2/glibc_tarballs _glibc 100
# run_raw /mnt/dataset2/smalltalk_tarballs _smalltalk 40
# run_method /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117

run_BiPal /mnt/dataset2/automake_tarballs _automake 100
run_BiPal /mnt/dataset2/bash_tarballs _bash 44
run_BiPal /mnt/dataset2/coreutils_tarballs _coreutils 28
run_BiPal /mnt/dataset2/fdisk_tarballs _fdisk 22
run_BiPal /mnt/dataset2/glibc_tarballs _glibc 100
run_BiPal /mnt/dataset2/smalltalk_tarballs _smalltalk 40
run_BiPal /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117
run_BiPal /mnt/dataset2/ThunderbirdTar _Thunderbird 240
run_BiPal /mnt/dataset2/chromium _chromium 107
run_BiPal /mnt/dataset2/linux _linux 270
run_BiPal /mnt/dataset2/WEB _WEB 102