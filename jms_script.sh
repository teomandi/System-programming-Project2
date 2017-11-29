#!/bin/bash
# to run ./jms_script.sh -l /home/teomandi/Desktop/e2trash/ -c list
echo Number of input parameters = $#
n=0
if [ $1 = "-l" ]
	then  path=$2	
elif [ $3 = "-l" ]
	then path=$4
elif [ $4 = "-l" ]
	then path=$5
fi

if [ $1 = "-c" ]
	then comman=$2
elif [ $3 = "-c" ]
	then comman=$4
fi
if [ $# = 5 ]
	then
	echo nnnnn
	if [ $1 = "-c" ]
		then n=$3
	elif [ $3 = "-c" ]
		then n=$5
	fi
fi
echo PATH: $path COMMAND: $comman N: $n
cd $path
if [ $comman = "list" ]
	then
		ls 
elif [ $comman = "size" ]
	then
		if [ $n -ne "0" ]
		then
			du -hsx * | sort -rh | head -$n
		else
			du -hsx * | sort -rh 
		fi
elif [ $comman = "purge" ]
	then
		rm -R -- */
fi


