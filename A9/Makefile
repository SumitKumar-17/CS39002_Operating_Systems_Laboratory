run: demandpaging.c
	gcc -Wall -o runsearch demandpaging.c
	./runsearch > output.txt

vrun: demandpaging.c
	gcc -Wall -DVERBOSE -o runsearch demandpaging.c
	./runsearch > verboseoutput.txt

db: gensearch.c
	gcc -Wall -o gensearch gensearch.c
	./gensearch

clean:
	-rm -f runsearch gensearch output.txt verboseoutput.txt