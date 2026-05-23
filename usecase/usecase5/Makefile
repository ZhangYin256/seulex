cal: cal.l cal.y ../../build/SeuLex
	bison -d cal.y
	../../build/SeuLex cal.l -o cal.yy.c
	gcc -o cal cal.tab.c cal.yy.c -lfl