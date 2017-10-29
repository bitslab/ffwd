rm tmp; 
for ((i=0;i<=10;i++)); do 
	$1 >> tmp; 
done; 
awk '{ sum += $2; n++ } END { if (n > 0) print sum / n; }' tmp;
# awk '{sum+=$2; sumsq+=$2*$2} END {print sqrt(sumsq/NR - (sum/NR)**2)}' tmp; \
