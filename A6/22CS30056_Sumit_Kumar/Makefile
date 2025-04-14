all:
	gcc -Wall -o cook cook.c
	gcc -Wall -o waiter waiter.c
	gcc -Wall -o customer customer.c

db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt

output:
	./cook > transcript_cook.txt 2>&1 & ./waiter >> transcript_waiter.txt 2>&1 & ./customer >> transcript_customer.txt 2>&1

final:
	./cook > transcript_combined.txt 2>&1 & ./waiter >> transcript_combined.txt 2>&1 & ./customer >> transcript_combined.txt 2>&1

clean:
	rm -f cook waiter customer gencustomers

transcript_clean:
	rm -rf transcript_combined.txt transcript_cook.txt transcript_waiter.txt transcript_customer.txt 