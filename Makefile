obj-m += cryptoapi-demo.o
#gcc testebbchar.c -o teste
#obj-m += testebbchar.o
 
all:
	rmmod cryptoapi-demo	
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	insmod cryptoapi-demo.ko key="0123456789ABCDE" iv="0123456789ABCDE"
	
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	
	
