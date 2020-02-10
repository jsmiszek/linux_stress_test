all: massivereader multiwriter

massivereader:
	gcc -Wall -o massivereader massivereader.c helper.c
	
multiwriter:
	gcc -Wall -o multiwriter multiwriter.c helper.c
	
clean: 
	rm  massivereader
	rm  multiwriter
