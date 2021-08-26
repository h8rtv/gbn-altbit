FLAGS =

all: gbn alt-bit

.PHONY: gbn
gbn:
	gcc prog2-gbn.c -o gbn $(FLAGS)


.PHONY: alt-bit
alt-bit:
	gcc prog2-alternating-bit.c -o alt-bit $(FLAGS)

clean:
	rm gbn alt-bit