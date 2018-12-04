# -Wunused = (Warn Unused) Da aviso de las variables que no se estan usando
# -Wall    = (Warn All) Da aviso de todos los posibles errores de compilación
main: server.c
	gcc -Wunused -Wall server.c -o server
	@echo "done compiling server.c"
	gcc -Wunused -Wall client.c -o client
	@echo "done compiling client.c"

.PHONY: clean
clean:
	rm -rf *.o client server
