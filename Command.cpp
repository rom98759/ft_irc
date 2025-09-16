/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/11 17:06:22 by rcaillie          #+#    #+#             */
/*   Updated: 2025/09/11 17:06:22 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"

/*
 * - Le pseudonyme ne doit pas être vide.
 * - La longueur maximale du pseudonyme est de 9 caractères.
 * - Le premier caractère doit être une lettre ou un caractère spécial autorisé.
 * - Les caractères restants doivent être alphanumériques ou des caractères spéciaux autorisés.
*/
static char	ft_isValidNick(const std::string &nick)
{
	static const char *const	spec = {"[]{}\\|"};

	std::size_t	nsize = nick.size();
	for (std::size_t i = 0; i < nsize; ++i)
		if (!std::isalnum(nick.at(i))
			&& ((std::string)spec).find(nick.at(i)) == std::string::npos)
			return (0);
	return (1);
}

static std::string levelToString(unsigned char level)
{
	if (level == 0b00)
		return "0";
	else if (level == 0b01)
		return "1";
	else if (level == 0b10)
		return "2";
	else if (level == 0b11)
		return "3";

	std::stringstream ss;
	ss << static_cast<int>(level);
	return ss.str();
}

static const std::vector<std::string>	ft_split(const std::string &str, const char &sep)
{
	std::vector<std::string>	recipients;
	std::size_t					start = 0;
	std::size_t					end = str.find(sep);

	while (end != std::string::npos)
	{
		recipients.push_back(str.substr(start, end - start));
		start = end + 1;
		end = str.find(sep, start);
	}
	recipients.push_back(str.substr(start));
	return (recipients);
}

/* **************************** |EVENTS/COMMANDS| **************************** */
/* ********* char	(Server::*)(Client *const, const std::string &); ********* */
/* *************************************************************************** */
char	Server::pass(Client *const cl, const std::vector<std::string> &tokens)
{
	if (cl->getRegisterLevel() > 0)
	{
		cl->sendMessage(formatError(ERR_ALREADYREGISTERED, cl->getNick().empty() ? "*" : cl->getNick(), "You may not reregister"));
		return (1);
	}

	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, "*", "PASS :Not enough parameters"));
		return (1);
	}
	if (tokens.size() != 2)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, "*", "PASS :Too many parameters"));
		return (1);
	}

	if (tokens.at(1) == _pw)
	{
		cl->sendMessage(formatMessage("NOTICE", "*", "Password accepted"));
		cl->upRegisterLevel();
	}
	else
		cl->sendMessage(formatError(ERR_PASSWDMISMATCH, "*", "Password incorrect"));
	return (1);
}

char	Server::nick(Client *const cl, const std::vector<std::string> &tokens)
{
	if (!cl->getRegisterLevel())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, "*", "You have not registered (use PASS first)"));
		return (1);
	}

	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NONICKNAMEGIVEN, cl->getNick().empty() ? "*" : cl->getNick(), "No nickname given"));
		return (1);
	}
	if (tokens.size() != 2)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, "*", "NICK :Too many parameters"));
		return (1);
	}

	std::string nick = tokens.at(1);

	// Vérifier le pseudo est valide
	if (!ft_isValidNick(nick))
	{
		cl->sendMessage(formatError(ERR_ERRONEUSNICKNAME, cl->getNick().empty() ? "*" : cl->getNick(), nick + " :Erroneous nickname. Only alphanumeric ASCII characters and \"[]{}\\|\" are considered valid."));
		return (1);
	}

	// Vérifier pseudo déjà utilisé par autre client
	std::size_t clientsCount = _clients.size();
	for (std::size_t i = 0; i < clientsCount; i++)
	{
		if (_clients[i] != cl && _clients[i]->getNick() == nick)
		{
			cl->sendMessage(formatError(ERR_NICKNAMEINUSE, cl->getNick().empty() ? "*" : cl->getNick(), nick + " :Nickname is already in use"));
			return (1);
		}
	}

	// Si client pas pseudo alors première définition
	bool isNewNick = cl->getNick().empty();

	// Stocker last pseudo notification (si changement)
	std::string oldNick = cl->getNick();
	std::string targetNick = isNewNick ? "*" : oldNick;

	cl->setNick(nick);

	// Si nouveau pseudo et client pas enregistré
	if (isNewNick && !cl->isRegistered())
	{
		cl->upRegisterLevel();
		cl->sendMessage(formatMessage("NICK", nick, nick));

		if (cl->isRegistered())
		{
			// messages bienvenue IRC
			cl->sendMessage(formatMessage(RPL_WELCOME, nick, "Welcome to the IRC Network, " + nick));
			cl->sendMessage(formatMessage(RPL_YOURHOST, nick, "Your host is unicorn.42.network, running version 1.0"));
			cl->sendMessage(formatMessage(RPL_MYINFO, nick, "unicorn.42.network 1.0 o o"));
		}
		else
			cl->sendMessage(formatMessage("NOTICE", nick, "Please complete registration with USER command."));
	}
	// Si changement de pseudo
	else if (!isNewNick)
	{
		std::string nickChangeMsg = ":" + oldNick + " NICK " + nick + "\r\n";
		mall(nickChangeMsg);
	}

	return (1);
}

char	Server::user(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (cl->getRegisterLevel() < 1)
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered (use PASS first)"));
		return (1);
	}

	if (cl->isRegistered() || cl->getUsername().length() || cl->getRealname().length())
	{
		cl->sendMessage(formatError(ERR_ALREADYREGISTERED, target, "You may not reregister"));
		return (1);
	}

	if (tokens.size() < 5) {
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "USER :Not enough parameters"));
		return (1);
	}
	if (tokens.size() != 5)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, target, "USER :Too many parameters"));
		return (1);
	}

	// USER <username> <hostname> <servername> :<realname>
	cl->setUsername(tokens.at(1));
	cl->setRealname(tokens.at(4));

	cl->upRegisterLevel();
	if (cl->isRegistered())
	{
		// Envoyer les messages de bienvenue standard IRC
		std::string nick = cl->getNick();
		cl->sendMessage(formatMessage(RPL_WELCOME, nick, "Welcome to the IRC Network, " + nick));
		cl->sendMessage(formatMessage(RPL_YOURHOST, nick, "Your host is unicorn.42.network, running version 1.0"));
		cl->sendMessage(formatMessage(RPL_MYINFO, nick, "unicorn.42.network 1.0 o o"));
	}
	else
		cl->sendMessage(formatMessage("NOTICE", target, "USER command received. Please complete registration with NICK command."));

	return (1);
}

char	Server::ping(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		cl->sendMessage(formatMessage("NOTICE", target, "Current registration level: " + levelToString(cl->getRegisterLevel()) + " (needs to be 3)"));
		return (1);
	}

	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "PING :Not enough parameters"));
		return (1);
	}
	if (tokens.size() != 2)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, target, "PING :Too many parameters"));
		return (1);
	}

	// PING -> PONG
	cl->sendMessage(formatMessage("PONG", "irc.server.com", tokens.at(1)));
	return (1);
}

char	Server::quit(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}
	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "PING :Not enough parameters"));
		return (1);
	}
	if (tokens.size() != 2)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, target, "PING :Too many parameters"));
		return (1);
	}

	std::string quitMessage = tokens.empty() ? cl->getNick() : tokens.at(1);

	cl->sendMessage(formatMessage("QUIT", cl->getNick(), "Quit: " + quitMessage));
	disconnectClient(cl, quitMessage);

	return (1);
}

char	Server::join(Client *const cl, const std::vector<std::string> &tokens)
{
	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, "*", "You have not registered"));
		return (1);
	}
	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, cl->getNick(), "JOIN :Not enough parameters"));
		return (1);
	}
	if (tokens.size() > 3)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, cl->getNick(), "JOIN :Too many parameters"));
		return (1);
	}
	std::vector<std::string>	chans = ft_split(tokens[1], ',');
	std::size_t					csize = chans.size();
	// TODO: Determine wether we handle (and how) empty strings or not
	if (cl->cannotJoinNChannels(csize))
	{
		cl->sendMessage(formatError(ERR_TOOMANYCHANNELS, cl->getNick(), "JOIN :You plan to join too many channels"));
		return (1);
	}

	std::vector<std::string>	keys;
	std::size_t					ksize;
	if (tokens.size() == 3)
		keys = ft_split(tokens[2], ',');
	ksize = keys.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		// TODO: Verify if it is a channel (# as first character) (We maybe won't handle local channels = no &).
		// TODO: Verify valid name overall (No ' ', no '^G', no ',').
		if (chans[i].empty())
		{
			cl->sendMessage(formatError(ERR_BADCHANMASK, chans[i], "JOIN :Bad channel mask"));
			continue ;
		}
		Channel	*targetChan = getChannel(chans[i]);
		if (targetChan == NULL)
		{
			targetChan = new Channel(chans[i], i < ksize ? keys[i] : "");
			*this += targetChan;
		}
		else if (targetChan->isFull())
		{
			cl->sendMessage(formatError(ERR_CHANNELISFULL, targetChan->getName(), "JOIN :Channel full"));
			continue ;
		}
		std::string	key = targetChan->getKey();
		if (!key.empty() && key != (i < ksize ? keys[i] : ""))
		{
			cl->sendMessage(formatError(ERR_BADCHANNELKEY, targetChan->getName(), "JOIN :Bad channel key"));
			continue ;
		}
		if (!(*cl += targetChan))
		{
			cl->sendMessage(formatMessage("Can't join", targetChan->getName(), "JOIN :Channel already joined"));
			continue ;
		}
		*targetChan += cl;
		// TODO: Send other steps of Welcome : TOPIC (if it has) | The list of every user bound to the channel.
		cl->sendMessage(formatMessage("Joined", targetChan->getName(), "JOIN :Channel successfully joined"));
		// TODO: Tell everyone in the channel cl joined.
	}
	return (1);
}

char	Server::part(Client *const cl, const std::vector<std::string> &tokens)
{
	if (!(cl->isRegistered()))
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, "*", "You have not registered"));
		return (1);
	}
	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, cl->getNick(), "PART :Not enough parameters"));
		return (1);
	}
	if (tokens.size() > 3)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, cl->getNick(), "PART :Too many parameters"));
		return (1);
	}

	std::vector<std::string>	chans = ft_split(tokens[1], ',');
	std::size_t					csize = chans.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		Channel	*targetChan = getChannel(chans[i]);
		if (targetChan == NULL)
		{
			cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, chans[i], "PART :No such channel"));
			continue ;
		}
		if (*cl -= targetChan)
		{
			*targetChan -= cl;
			cl->sendMessage(formatMessage(cl->getNick(), chans[i], "PART :Left"));
			// TODO: Send to all cl has quit the channel with reason if it has.
		}
		else
		{
			cl->sendMessage(formatError(ERR_NOTONCHANNEL, chans[i], "PART :Not on channel"));
			continue ;
		}
	}
	return (1);
}

// Fonction débogage pour afficher l'état d'enregistrement du client
char	Server::debug(Client *const cl, const std::vector<std::string> &tokens)
{
	(void)tokens;

	std::string status = "--- DEBUG CLIENT STATUS ---\r\n";
	status += "- Register Level: " + levelToString(cl->getRegisterLevel()) + "/3\r\n";
	status += "- Is Registered: " + std::string(cl->isRegistered() ? "Yes" : "No") + "\r\n";
	status += "- Nickname: " + (cl->getNick().empty() ? "[Not Set]" : cl->getNick()) + "\r\n";
	status += "- Username: " + (cl->getUsername().empty() ? "[Not Set]" : cl->getUsername()) + "\r\n";
	status += "- Realname: " + (cl->getRealname().empty() ? "[Not Set]" : cl->getRealname()) + "\r\n";
	status += "- Binary RegisterLevel: ";

	unsigned char level = cl->getRegisterLevel();
	for (int i = 1; i >= 0; i--)
		status += ((level >> i) & 1) ? "1" : "0";

	status += "\r\n";
	status += "-------------------------\r\n";

	cl->sendMessage(status);
	return (1);
}
