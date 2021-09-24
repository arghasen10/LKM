#! /bin/bash

echo "Enter the Size of the DeQue"
read N
echo "$N" > /proc/partb_1_20CS92P05
while true
do
	echo -e "Enter your Choice: \n1.Insert\n2.Read Data\n3.Exit\n"
	read choice
	case $choice in
		1)
		echo "Insert a value in the DeQue"
		read val
		echo "$val" > /proc/partb_1_20CS92P05
		;;

		2)
		echo "Data at the left is : "
		cat /proc/partb_1_20CS92P05
		;;

		3)
		echo "Exiting..."
		break
		;;

		*)
		echo "Invalid Choice"
		continue
		;;
	esac

done

exit 0;
