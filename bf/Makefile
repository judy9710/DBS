# Makefile for BF layer
LIB	= bf
INCDIR	= ${MINIREL_HOME}/h
INCS	= 
SRCS	= 
TESTS	= bftest.c
OBJS	= ${SRCS:.c=.o}
LIBS	= lib${LIB}.a

#############################################################################
# This macro definition can be overwritten by command-line definitions.
CC	= gcc
CFLAGS	= -g -ansi -pedantic
#CFLAGS	= -O -ansi -pedantic
#############################################################################

all: lib${LIB}.a ${LIB}test # dependency 2개

${LIB}test: ${LIB}test.o lib${LIB}.a		
	$(CC) $(CFLAGS) -o $@ $< ${LIBS} 
# 위의 코드는 .o와 .a를 링킹해줌

lib${LIB}.a:$(OBJS)
	ar cr lib${LIB}.a $(OBJS)
	ranlib lib${LIB}.a
# 위의 코드가지고 19번째 줄 실행, 아카이브 생성(ar)과 무작위 라이브러리로 변환(ranlib)

$(OBJS): ${INCS}

clean:
	rm -f lib${LIB}.a *.o ${LIB}test *.bak *~

.c.o:; $(CC) $(CFLAGS) -c $< -I. -I$(INCDIR)


