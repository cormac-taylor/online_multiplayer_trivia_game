# ******************************************************************************
# Name        : Makefile
# Author      : Cormac Taylor
# Pledge      : I pledge my honor that I have abided by the Stevens Honor System.
# ******************************************************************************

all:
	gcc server.c -o server
	gcc client.c -o client

clean:
	rm server
	rm client
	