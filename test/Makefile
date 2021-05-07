
strtest: strtest.c ../libtools-dbg.a
	gcc $^ -o $@

clean:
	rm -f strtest
	(cd .. ; make clean)

../libtools-dbg.a:
	(cd .. ; make debug)

