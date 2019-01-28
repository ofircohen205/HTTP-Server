#!/bin/bash
mkdir a
chmod 111 a
gcc -Wall *.c -o t -lpthread

echo "---Validation test---"

gnome-terminal -e "sudo valgrind ./t $1 80 80"
sleep 2
echo "-------------------------------------------"
echo "Overflow test"
gnome-terminal -e "wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF&& wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF"

echo "------------------------------------------------"
sleep 5
echo "Parallel test"

gnome-terminal -e "wget -O GOOD1.PDF http://localhost:$1/work.PDF"
gnome-terminal -e "wget -O GOOD2.PDF http://localhost:$1/work.PDF.1"
gnome-terminal -e "wget -O GOOD3.PDF http://localhost:$1/work.PDF.2"
gnome-terminal -e "wget -O GOOD4.PDF http://localhost:$1/work.PDF.3"
gnome-terminal -e "wget -O GOOD5.PDF http://localhost:$1/work.PDF.4"
gnome-terminal -e "wget -O GOOD6.PDF http://localhost:$1/work.PDF.5"
echo "-------------------------------------------"

echo "Overflow test"
gnome-terminal -e "wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF&& wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF && wget http://localhost:$1/work.PDF"

sleep 10

#pkill gnome-terminal






