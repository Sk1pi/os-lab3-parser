CC     = gcc
CFLAGS = -Wall -g

calc: parser.tab.c lex.yy.c ast.c main.c ast.h
	$(CC) $(CFLAGS) -o calc parser.tab.c lex.yy.c ast.c main.c -lm

# -d: заголовок з токенами, -v: parser.output з таблицею станів LALR
parser.tab.c parser.tab.h: parser.y
	bison -d -v parser.y

lex.yy.c: lexer.l parser.tab.h
	flex lexer.l

# Розібрати приклад, зібрати згенерований C-код і порівняти результати
run: calc
	./calc examples/input.txt > result_interp.txt
	$(CC) out.c -o out -lm
	./out > result_compiled.txt
	diff result_interp.txt result_compiled.txt && echo "Інтерпретатор і згенерований код дали однаковий результат"

# Демонстрація конфліктів: та сама граматика без пріоритетів
conflicts:
	sed -e '/^%left/d' -e '/^%right/d' -e '/^%precedence/d' -e 's/%prec NEG//' parser.y > /tmp/noprec.y
	-bison -Wcounterexamples -o /tmp/noprec.c /tmp/noprec.y

clean:
	rm -f calc out parser.tab.c parser.tab.h parser.output lex.yy.c out.c result_*.txt
