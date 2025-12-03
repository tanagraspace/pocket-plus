
CC := gcc
CFLAGS := -g -Wall

RM := rm -f

MD5 := md5sum

.PHONY: all clean example

all: pocket_compress pocket_decompress

pocket_compress:
	${CC} ${CFLAGS} -o ${@} ${@}.c

pocket_decompress:
	${CC} ${CFLAGS} -o ${@} ${@}.c

clean:
	${RM} pocket_compress pocket_decompress *.pkt *.depkt

example: pocket_compress pocket_decompress
	${RM} 1028packets.ccsds.pkt 1028packets.ccsds.pkt.depkt
	./pocket_compress 1028packets.ccsds 90 20 50 100 2
	./pocket_decompress 1028packets.ccsds.pkt
	${MD5} 1028packets.ccsds 1028packets.ccsds.pkt.depkt

