all:
	gcc leader.c -o leader
	gcc follower.c -o follower


leader:
	ipcrm -a
	./leader 15


f1:
	./follower

f2:
	./follower 10

f3:
	./follower 8

cshm:
	ipcrm -a 

clean:
	rm -f leader follower