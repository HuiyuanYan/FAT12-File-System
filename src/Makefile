objects = FAT12.o shell.o main.o 
FAT : $(objects)
	g++ -o FAT $(objects)
	./FAT

main.o : FAT12.h
FAT12.o : FAT12.h
shell.o : shell.h
.PHONY : clean
clean:
	rm edit $(objects)