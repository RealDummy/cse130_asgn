ASGN :=asgn1
NAME :=httpserver
OBJ := bind.o server.o strview.o helpers.o log.o queue.o
W := -Wall -Werror -Wextra -pedantic -Wenum-conversion
O := 
L := -lpthread -fsanitize=address
all: $(NAME).o $(OBJ)
	clang $(W) $<  -o $(NAME) $(OBJ) $(O) $(L) 

%.o: %.c $(OBJ:%.o %.h)
	clang $(W) -c $< -o $@ $(O)

clean:
	rm -f $(OBJ) $(NAME)
