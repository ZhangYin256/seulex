test3: test3-flex test3-seulex test3.txt
	./test3-flex < test3.txt
	./test3-seulex < test3.txt
	
test3-flex:test3.l test3.txt
	flex ./test3.l
	gcc ./lex.yy.c -o test3-flex -lfl

test3-seulex:test3.l test3.txt
	../../build/SeuLex ./test3.l
	gcc ./test3.yy.c -o test3-seulex -lfl

