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

// Fonction utilitaire pour ignorer les espaces
std::size_t ft_skipSpaces(const std::string &s, const std::size_t &start)
{
	std::size_t pos = s.find_first_not_of(" \t\n\v\f\r", start);
	return (pos == std::string::npos) ? s.length() : pos;
}

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

/* **************************** |EVENTS/COMMANDS| **************************** */
/* ********* char	(Server::*)(Client *const, const std::string &); ********* */
/* *************************************************************************** */
char	Server::pass(Client *const cl, const std::string &cmd)
{
	if (cl->getRegisterLevel() > 0)
	{
		cl->sendMessage(formatError(ERR_ALREADYREGISTERED, cl->getNick().empty() ? "*" : cl->getNick(), "You may not reregister"));
		return (1);
	}
	std::size_t	idx = ft_skipSpaces(cmd);
	if (idx >= cmd.size())
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, "*", "PASS :Not enough parameters"));
		return (1);
	}

	// Ignorer espaces et extraire uniquement le mot de passe
	std::string input_pw = cmd.substr(idx);
	std::size_t end_pw = input_pw.find_first_of(" \t\n\v\f\r");
	if (end_pw != std::string::npos)
		input_pw = input_pw.substr(0, end_pw);

	if (input_pw == _pw)
	{
		cl->sendMessage(formatMessage("NOTICE", cl->getNick().empty() ? "*" : cl->getNick(), "Password accepted"));
		cl->upRegisterLevel();
	}
	else
		cl->sendMessage(formatError(ERR_PASSWDMISMATCH, "*", "Password incorrect"));
	return (1);
}

char	Server::nick(Client *const cl, const std::string &cmd)
{
	if (!cl->getRegisterLevel())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, "*", "You have not registered (use PASS first)"));
		return (1);
	}

	std::size_t	idx = ft_skipSpaces(cmd);
	if (idx >= cmd.size())
	{
		cl->sendMessage(formatError(ERR_NONICKNAMEGIVEN, cl->getNick().empty() ? "*" : cl->getNick(), "No nickname given"));
		return (1);
	}

	// Extraire uniquement nickname sans espaces après
	std::string nick = cmd.substr(idx);
	std::size_t end_nick = nick.find_first_of(" \t\n\v\f\r");
	if (end_nick != std::string::npos)
		nick = nick.substr(0, end_nick);

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

char	Server::user(Client *const cl, const std::string &cmd)
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

	std::vector<std::string> params = parseIrcMessage(cmd, "");
	if (params.size() < 4) {
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "USER :Not enough parameters"));
		return (1);
	}

	// USER <username> <hostname> <servername> :<realname>
	cl->setUsername(params[0]);
	cl->setRealname(params[3]);

	// Si niveau 2 alors NICK traité
	if (cl->getRegisterLevel() == 2)
	{
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
			cl->sendMessage(formatMessage("NOTICE", target, "Please complete registration with NICK command."));
		return (1);
	}

	// PASS validé mais NICK pas encore
	cl->upRegisterLevel();
	cl->sendMessage(formatMessage("NOTICE", target, "USER command received. Please set NICK to complete registration."));
	return (1);
}

char	Server::ping(Client *const cl, const std::string &cmd)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		cl->sendMessage(formatMessage("NOTICE", target, "Current registration level: " + levelToString(cl->getRegisterLevel()) + " (needs to be 3)"));
		return (1);
	}

	std::size_t	idx = ft_skipSpaces(cmd);
	if (idx >= cmd.size())
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "PING :Not enough parameters"));
		return (1);
	}

	// Extraire token sans espaces après
	std::string token = cmd.substr(idx);
	std::size_t end_token = token.find_first_of(" \t\n\v\f\r");
	if (end_token != std::string::npos)
		token = token.substr(0, end_token);

	// PING -> PONG
	cl->sendMessage(formatMessage("PONG", "irc.server.com", token));
	return (1);
}

char	Server::quit(Client *const cl, const std::string &cmd)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}

	std::vector<std::string> params = parseIrcMessage(cmd, "");
	std::string quitMessage = params.empty() ? "Client Quit" : params[0];

	cl->sendMessage(formatMessage("QUIT", cl->getNick(), "Quit: " + quitMessage));
	disconnectClient(cl, quitMessage);

	return (1);
}

// Fonction débogage pour afficher l'état d'enregistrement du client
char	Server::debug(Client *const cl, const std::string &cmd)
{
	(void)cmd;

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