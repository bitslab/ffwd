awk '{print $1, $1*2000/$3}' ccQueue.run.txt >> ccQueue.run.txt.plot
awk '{print $1/1000}' LFStack.run.txt  >> LFStack.run.txt.mid
for i in 2 4 8 16 32 48 64; do echo $i >> rows; done
paste rows LFStack.run.txt.mid  >> LFStack.run.txt.plot
awk '{print $1, $2/1000}' hQueue.run.txt >> hQueue.run.txt.plot
awk '{print $1, $2/1000}' dsmQueue.run.txt >>  dsmQueue.run.txt.plot
paste rows MSQueue.run.txt >> MSQueue.run.txt.mid
awk '{print $1, $1*2000/$3}' MSQueue.run.txt.mid >> MSQueue.run.txt.plot
paste rows SimQueue.run.txt >> SimQueue.run.txt.mid
awk '{print $1, $2/1000}'  SimQueue.run.txt.mid >>  SimQueue.run.txt.plot
paste rows SimStack.run.txt >> SimStack.run.txt.mid
awk '{print $1, $2/1000}'  SimStack.run.txt.mid >>  SimStack.run.txt.plot
