NAME = ircserv

NAME_BONUS = unibot^_^

OBJDIR = obj/

SRC = main.cpp Server.cpp Client.cpp Command.cpp Channel.cpp
OBJ = $(patsubst %.cpp,$(OBJDIR)%.o,$(notdir $(SRC)))
DEP = $(patsubst %.cpp,$(OBJDIR)%.d,$(notdir $(SRC)))

SRC_BONUS = Unibot.cpp
OBJ_BONUS = $(patsubst %.cpp,$(OBJDIR)%.o,$(notdir $(SRC_BONUS)))
DEP_BONUS = $(patsubst %.cpp,$(OBJDIR)%.d,$(notdir $(SRC_BONUS)))

CCA = c++ -g3 -std=c++98 -Werror -Wall -Wextra -MP -MMD
INCLUDES = -I.

MAKEFLAGS += --no-print-directory

all: $(NAME)

$(NAME): $(OBJ)
	@$(CCA) $(OBJ) -o $(NAME)
	@printf "ircserv: Build Complete !\n"

bonus: $(NAME_BONUS)

$(NAME_BONUS): $(OBJ_BONUS)
	@$(CCA) $(OBJ_BONUS) -o $(NAME_BONUS)
	@printf "unibot^_^: Build Complete !\n"

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
	@if [ -f $(NAME_BONUS) ]; then \
		rm -f $(NAME_BONUS) \
		&& printf "unibot^_^: Cleaned !\n"; \
	fi

re: fclean all

.PHONY: all clean fclean re bonus

-include $(DEP)
