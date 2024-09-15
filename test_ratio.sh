cd bin


path=/mnt/dataset2/cassandra
name=_cassandra
num=97
for ratio in {1..5}; do
  ./BiSearch -i "$path" -c 4 -m 5 -n "$num" -r "$ratio">${name}_${ratio}.txt
done

path=/mnt/dataset2/LKT
name=_LKT
num=84
for ratio in {1..5}; do
  ./BiSearch -i "$path" -c 4 -m 5 -n "$num" -r "$ratio">${name}_${ratio}.txt
done

path=/mnt/dataset2/ThunderbirdTar
name=_Thunderbird
num=240

for ratio in {1..5}; do
  ./BiSearch -i "$path" -c 4 -m 5 -n "$num" -r "$ratio">${name}_${ratio}.txt
done

# path=/mnt/dataset2/WEB
# name=_WEB
# num=102
# for ratio in {1..5}; do
#   ./BiSearch -i "$path" -c 4 -m 5 -n "$num" -r "$ratio">${name}_${ratio}.txt
# done

