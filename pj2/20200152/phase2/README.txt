[system programming lecture]

-project 2 baseline

csapp.{c,h}
        CS:APP3e functions

shellex.c
        Simple shell example

컴파일
	make

실행  
	./myshell

종료
	exit

특이사항
	| (pipe)
	ls | grep filename
	cat filename | less
	cat filename | grep -i "abc" | sort -r