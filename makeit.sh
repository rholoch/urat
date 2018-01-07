if [ $1x != "x" ]; then
	cc $1.c -g -Wno-format -o $1
fi

