LIBS=libs/serversocket \
	 libs/file_handeling \
	 libs/handshake \
	 libs/serial \
	 libs/message \
	 libs/client
	 
INCLUDE_PATH=libs/

boardcomputer_exec: libraries
	gcc libs/boardcomputer.c $(wildcard libraries/*) -I$(INCLUDE_PATH) -o boardcomputer

libraries:
	mkdir libraries/
	for dir in $(LIBS); do \
		cd $$dir; \
		gcc -c *.c -I../; \
		mv *.o ../../libraries; \
		cd -; \
	done

clean:
	rm -rf libraries/

remove:
	rm boardcomputer
