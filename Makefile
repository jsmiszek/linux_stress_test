all: massivereader multiwriter

massivereader:
	gcc -Wall -o massivereader massivereader.c helper.c -lrt
	
multiwriter:
	gcc -Wall -o multiwriter multiwriter.c helper.c -lrt
	
clean: 
	rm  massivereader
	rm  multiwriter
