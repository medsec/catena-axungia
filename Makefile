CC=clang

#assumes the Catena-reference implementation to be in ../catena
ifndef CATENADIR
	CATENADIR = ../catena
endif
CATENAPATH = $(CATENADIR)/src

TARGETDIR=./
SRCDIR=./src

CFLAGS=-fomit-frame-pointer -O3 -std=c99 -fgnu89-inline -march=native -W -Wall

LDFLAGS = $(CFLAGS) -lm -s

#Silence the warnings from blake2b
CFLAGS+= -Wno-unused-function -Wno-unused-const-variable

#expecting the catena-reference implementation in ../Catena-reference
#you can change this by adding CATENADIR=../somepath to the invocation of make
ifndef CATENADIR
	CATENADIR = ../Catena-reference
endif
CATENAPATH = $(CATENADIR)/src

CFLAGS+= -I$(CATENAPATH)

#Determine if SSE2 or greater is available
SSE_TAGS = $(shell /bin/grep -m 1 flags /proc/cpuinfo | /bin/grep -o \
	'sse2\|sse3\|ssse3\|sse4a\|sse4.1\|sse4.2' | sed  's/\_/./g')

ifneq (${SSE_TAGS},) 
    #Choose optimized hash function
    HDIR=$(CATENAPATH)/blake2-sse
    LDFLAGS += -L$(HDIR) 
    CFLAGS += -I$(HDIR)
	HASH=$(HDIR)/blake2b.c
	HASHIMPL = $(CATENAPATH)/catena-blake2b-sse.c
else
	#use reference implementation
    HDIR=$(CATENAPATH)/blake2-ref
    LDFLAGS += -L$(HDIR) 
    CFLAGS += -I$(HDIR)
	HASH=$(HDIR)/blake2b-ref.c
	HASHIMPL = $(CATENAPATH)/catena-blake2b-ref.c
endif

FAST = -DFAST

.PHONY: all clean

all: BRG-wrapper.o BRGFH-wrapper.o DBGFH-wrapper.o DBG-wrapper.o \
	catena-blake2b.o catena-blake2bFH.o catena-axungia 

#Preprocessor magic: to be able to include catena-DBG and catena-BRG at the same
#time, we have to redefine the following: Flap, GARLIC, LAMBDA, MIN_GARLIC, 
#VERSION_ID, wrapper
#For DBG, we additionally have to define DBG for the right includes to be used

BRG-wrapper.o: $(SRCDIR)/wrapper.c
	$(CC) $(CFLAGS) $(FAST) -DFlap=FlapBRG -DGARLIC=GARLICBRG \
	-DLAMBDA=LAMBDABRG -DMIN_GARLIC=MIN_GARLICBRG -DVERSION_ID=VERSION_IDBRG \
	-Dwrapper=catena_BRG -c \
	$(SRCDIR)/wrapper.c -o $@

DBG-wrapper.o: $(SRCDIR)/wrapper.c
	$(CC) $(CFLAGS) $(FAST) -DFlap=FlapDBG -DGARLIC=GARLICDBG \
	-DLAMBDA=LAMBDADBG -DMIN_GARLIC=MIN_GARLICDBG -DVERSION_ID=VERSION_IDDBG \
	-Dwrapper=catena_DBG -DDBG -c \
	$(SRCDIR)/wrapper.c -o $@

#build blake2b / blake2b Fullhash
catena-blake2b.o: $(HASHIMPL)
	$(CC) $(CFLAGS) $(FAST) -c $(HASHIMPL) -o $@

#redefine __Hash1 to 5, __HashFast, __ResetState
catena-blake2bFH.o: $(HASHIMPL)
	$(CC) $(CFLAGS) -D__Hash1=__HashFH1 -D__Hash2=__HashFH2 -D__Hash3=__HashFH3 \
	-D__Hash4=__HashFH4 -D__Hash5=__HashFH5 -D__HashFast=__HashFastFH \
	-D__ResetState=__ResetStateFH -c $(HASHIMPL) -o $@

#here we also have to redefine __HashFast and __ResetState to support FullHash
#There no change needed to __Hash as it is the same for Fast or FullHash
#For BRG we have to redefine reverse to prevent collisions
BRGFH-wrapper.o: $(SRCDIR)/wrapper.c
	$(CC) $(CFLAGS) -DFlap=FlapBRGFH -DGARLIC=GARLICBRGFH -DLAMBDA=LAMBDABRGFH \
	-DMIN_GARLIC=MIN_GARLICBRGFH -DVERSION_ID=VERSION_IDBRGFH \
	-Dreverse=reverseFH -Dwrapper=catena_BRGFH -D__HashFast=__HashFastFH \
	-D__ResetState=__ResetStateFH -c \
	$(SRCDIR)/wrapper.c -o $@

#For DBG we have to redefine idx and sigma to prevent collisions
DBGFH-wrapper.o: $(SRCDIR)/wrapper.c
	$(CC) $(CFLAGS) -DFlap=FlapDBGFH -DGARLIC=GARLICDBGFH -DLAMBDA=LAMBDADBGFH \
	-DMIN_GARLIC=MIN_GARLICDBGFH -DVERSION_ID=VERSION_IDDBGFH \
	-Didx=idxFH -Dsigma=sigmaFH -Dwrapper=catena_DBGFH -DDBG \
	-D__HashFast=__HashFastFH -D__ResetState=__ResetStateFH -c \
	$(SRCDIR)/wrapper.c -o $@

catena-axungia: $(SRCDIR)/catena-axungia.c DBG-wrapper.o BRG-wrapper.o\
	DBGFH-wrapper.o BRGFH-wrapper.o catena-blake2b.o catena-blake2bFH.o
	$(CC) $(LDFLAGS) $(FAST) -o $(TARGETDIR)/$@ $(SRCDIR)/catena-axungia.c \
	$(CATENAPATH)/catena-helpers.c $(HASH) catena-blake2b.o catena-blake2bFH.o \
	DBG-wrapper.o BRG-wrapper.o DBGFH-wrapper.o BRGFH-wrapper.o -o $@

clean:
	rm -f *.o
	rm -f catena-axungia
