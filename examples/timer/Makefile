# copy this into the codegen/exe/user folder on the target machine
# then run it, it will build the binary in this folder
vpath %c ../../../stl
OBJ = thread1.o  thread2.o  thread3.o  user.o  user_initialize.o  user_terminate.o main.o stl.o
CFLAGS += -I . -I ../../../stl
LIBS = -ldl -lpthread -lrt

user: $(OBJ)
	$(CC) -o main -fPIC $(OBJ) $(LIBS)
