test2: test2-flex test2-seulex test2.txt
	./test2-flex < test2.txt
	./test2-seulex < test2.txt
	
test2-flex:test2.l test2.txt
	flex ./test2.l
	gcc ./lex.yy.c -o test2-flex -lfl

test2-seulex:test2.l test2.txt
	../../build/SeuLex ./test2.l
	gcc ./test2.yy.c -o test2-seulex -lfl

