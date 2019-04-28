all:
	get
	build
	run

get:
	git pull origin master

build:
	mpicc main.c -o main

run:
	mpirun -n 4 main

publish:
	git add main.c mpi_hosts Makefile
	git commit -m "update"
	git push origin master