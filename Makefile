NAME = ircserv

OBJDIR = obj/

SRC = main.cpp Server.cpp Client.cpp srv_utils.cpp
OBJ = $(patsubst %.cpp,$(OBJDIR)%.o,$(notdir $(SRC)))
DEP = $(patsubst %.cpp,$(OBJDIR)%.d,$(notdir $(SRC)))

CCA = c++ -g3 -std=c++98 -Werror -Wall -Wextra -MP -MMD
INCLUDES = -I.

MAKEFLAGS += --no-print-directory

all: $(NAME)

$(NAME): $(OBJ)
	@$(CCA) $(OBJ) -o $(NAME)
	@printf "ircserv: Build Complete !\n"

$(OBJDIR):
	@mkdir -p $(OBJDIR)
	@printf "ircserv: $(OBJDIR) Genereated !\n"

vpath %.cpp .

$(OBJDIR)%.o: %.cpp | $(OBJDIR)
	@$(CCA) $(INCLUDES) -c $< -o $@

clean:
	@if [ -d $(OBJDIR) ]; then \
		rm -rf $(OBJDIR) \
		&& printf "ircserv: $(OBJDIR) Removed !\n"; \
	fi

fclean: clean
	@if [ -f $(NAME) ]; then \
		rm -f $(NAME) \
		&& printf "ircserv: Cleaned !\n"; \
	fi

re: fclean all

.PHONY: all clean fclean re

-include $(DEP)
