# thready Makefile
#
# TODO Add nicer comments; build on cstructs Makefile as an example.
#

cstructs_obj = out/array.o out/map.o out/list.o
ifeq ($(shell uname -s), Darwin)
	cflags = $(includes) -std=c99
else
	cflags = $(includes) -std=c99 -D _GNU_SOURCE
endif
lflags = -lm
cc = gcc $(cflags)


all: out/thready.o out/json.o out/jsonutil.o

out/thready.o: thready/thready.c thready/thready.h | out
	$(cc) -o $@ -c $<

out/json.o: json/json.c json/json.h | out
	$(cc) -o $@ -c $<

out/jsonutil.o: json/jsonutil.c json/jsonutil.h | out
	$(cc) -o $@ -c $<

$(cstructs_obj) : out/%.o: cstructs/%.c cstructs/%.h | out
	$(cc) -o $@ -c $<

out:
	mkdir out

clean:
	rm -rf out/

.PHONY: test
