.PHONY: gbn
gbn:
	gcc prog2-gbn.c -o gbn -lm

.PHONY: alt-bit
alt-bit:
	gcc prog2-alternating-bit.c -o alt-bit

clean:
	rm gbn alt-bit