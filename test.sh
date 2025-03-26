cd bin
run_method(){
  local path=$1
  local name=$2
  local num=$3
./BiSearch -i $path -c 1 -m 0 -n $num  >Dedup$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 1 -m 3 -n $num  >Odess$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 1 -m 4 -n $num  >Palantir$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 5 -m 3 -n $num  >OdessMTar$name.txt
sudo rm -r mTarFile
sudo mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 5 -m 4 -n $num  >PalantirMTar$name.txt
sudo rm -r mTarFile
sudo mkdir mTarFile
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 4 -m 3 -n $num  >SA_Odess$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches

./BiSearch -i $path -c 4 -m 5 -n $num  >SA_BiSearch$name.txt
sudo rm Containers/*
sudo echo 3 > /proc/sys/vm/drop_caches
}

run_RAW(){
  local path=$1
  local name=$2
  local num=$3
./BiSearch -i $path -c 8 -m 0 -n $num  >Dedup_Skip$name.txt
rm -r mTarFile
mkdir mTarFile
rm Containers/*
echo 3 > /proc/sys/vm/drop_caches
}
datasets=(

  "/mnt/dataset2/automake_tarballs _automake 100"
  "/mnt/dataset2/bash_tarballs _bash 44"
  "/mnt/dataset2/coreutils_tarballs _coreutils 28"
  "/mnt/dataset2/fdisk_tarballs _fdisk 22"
  "/mnt/dataset2/glibc_tarballs _glibc 100"
  "/mnt/dataset2/smalltalk_tarballs _smalltalk 40"
  "/mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117"
  "/mnt/dataset2/linux _linux 270"
  "/mnt/dataset2/WEB _WEB 102"
  "/mnt/dataset2/chromium _chromium 107"
  "/mnt/dataset2/ThunderbirdTar _Thunderbird 240"
  "/mnt/dataset2/Android _Android 36"
)

for dataset in "${datasets[@]}"; do
  # run_method $dataset
  run_RAW $dataset
done

# run_method /mnt/dataset2/linux _linux 270
# run_method /mnt/dataset2/WEB _WEB 102
# run_method /mnt/dataset2/chromium _chromium 107
# run_method /mnt/dataset2/ThunderbirdTar _Thunderbird 240
# run_method /mnt/dataset2/Android _Android 36
# run_method /mnt/dataset2/automake_tarballs _automake 100
# run_method /mnt/dataset2/bash_tarballs _bash 44
# run_method /mnt/dataset2/coreutils_tarballs _coreutils 28
# run_method /mnt/dataset2/fdisk_tarballs _fdisk 22
# run_method /mnt/dataset2/glibc_tarballs _glibc 100
# run_method /mnt/dataset2/smalltalk_tarballs _smalltalk 40
# run_method /mnt/dataset2/GNU_GCC/gcc-packed/tar _gcc 117





