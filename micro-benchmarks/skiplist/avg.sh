rm tmp;
for ((i=0;i<=10;i++)); do
	$1 >> tmp;
done;
awk '{ sum += $2; n++ } END { if (n > 0) print sum/n; }' tmp;
rm tmp;