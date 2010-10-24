all: to_json json_to_tar

%: %.c
	gcc -o $@ -ansi -I/usr/local/include $< -L/usr/local/lib -lyajl -g


clean:
	rm -f to_json json_to_tar

distclean: clean
