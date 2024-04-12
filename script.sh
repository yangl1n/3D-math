#~ /bin/bash
set -e
end=$(($1-1))
for (( i=0; i<=$end; i++ ))
do
	qsub run.sh $1 $i;
	sleep 1.5s
done
while [ `ls | grep "point" | wc -l` != "$1" ]
do
	echo "Waiting all the jobs finish...";
	sleep 20s
done
mv point* run.sh* ./output
cd ./output
cat point* > points.txt
mv points.txt ../
