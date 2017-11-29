all:
	gcc pool.c -o pool
	gcc coord.c -o jms_coord
	gcc console.c -o jms_console
	gcc slave.c -o slave
	gcc slave3.c -o slave3
	./jms_coord -w ./jms_out -r ./jms_in -n 2 -l /home/teomandi/Desktop/e2trash/