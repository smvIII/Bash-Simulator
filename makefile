# Names: Stanley Vossler, Carlos Pantoja-Malaga, Matthew Echenique
# Professor: Andy Wang, PhD
# Class: COP 4610
# Project: 1
# Description: This is the makefile for Project 1: "Implementing a Shell in C".

BINS = shell
C = gcc
CFLAGS = -std=c99 

all: $(BINS)

shell: shell.c
	$(C) $(CFLAGS) -o shell shell.c

clean:
	rm $(BINS)
