test1: test1-flex test1-seulex test1.txt
	./test1-flex < test1.txt
	./test1-seulex < test1.txt
	
test1-flex:test1.l test1.txt
	flex ./test1.l
	gcc ./lex.yy.c -o test1-flex -lfl

test1-seulex:test1.l test1.txt
	../../build/SeuLex ./test1.l
	gcc ./test1.yy.c -o test1-seulex -lfl

