CC=g++

all: MyBot

cleantemp:
	rm -rf *.o MyBot ./Benzenine ./funky ./sohamBot ./tcp log* *.sh v1 playnview playgame *.txt stats* ideas errs typescript ./Untitled\ Folder ./zvold *.py ./example_bots

clean:
	rm -rf *.o MyBot *.txt stats* ideas errs typescript ./Untitled\ Folder log* ./Benzenine ./funky ./Untitled\ Folder



MyBot: MyBot.o PlanetWars.o Commons.o
