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
 * - La taille du pseudonyme ne doit pas excéder 42 caractères.
 * - Le pseudonyme ne doit pas être vide.
 * - Les caractères doivent être alphanumériques ou des caractères spéciaux autorisés.
*/
static char	ft_isValidNick(const std::string &nick)
{
	static const char *const	spec = "[]{}\\|-_";

	std::size_t	nsize = nick.size();
	for (std::size_t i = 0; i < nsize; ++i)
		if (!std::isalnum(nick.at(i))
			&& ((std::string)spec).find(nick.at(i)) == std::string::npos)
			return (0);
	return (!!nsize && nsize <= 42);
}

/*
 * - La taille du nom ne doit pas excéder 747 caractères
 * - Le nom ne doit pas être vide.
 * - Les caractères peuvent être alphabétiques, espaces, et certains caractères spéciaux.
*/
static char	ft_isValidRealname(const std::string &realname)
{
	std::size_t	rsize = realname.size();
	for (std::size_t i = 0; i < rsize; ++i)
	{
		char c = realname.at(i);
		if (!std::isprint(c) && c != ' ')
			return (0);
	}
	return (!!rsize && rsize <= 747);
}

/*
 * - La taille du nom de canal ne doit pas excéder 42 caractères.
 * - Un nom de canal doit commencer par '#'.
 * - Les caractères doivent être alphanumériques ou '-'.
*/
static char	ft_isValidChannelMask(const std::string &channelMask)
{
	std::size_t	csize = channelMask.size();

	if (!csize || channelMask.at(0) != '#')
		return (0);
	for (std::size_t i = 1; i < csize; ++i)
		if (!std::isalnum(channelMask.at(i)) && channelMask.at(i) != '-')
			return (0);
	return (csize > 1 && csize <= 42);
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

static inline void	welcome(Client *const cl)
{
	const std::string	&nick = cl->getNick();

	cl->sendMessage(formatMessage(RPL_WELCOME, nick, "Welcome to the IRC Network, " + nick));
	cl->sendMessage(formatMessage(RPL_YOURHOST, nick, "Your host is 127.0.0.1, running version 1.0"));
	cl->sendMessage(formatMessage(RPL_MYINFO, nick, "127.0.0.1 1.0 o o"));
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
		cl->sendMessage(formatError(ERR_ERRONEUSNICKNAME, cl->getNick().empty() ? "*" : cl->getNick(), nick + " :Erroneous nickname. Only alphanumeric ASCII characters and \"[]{}\\|-_\" are considered valid. Nicknames can't be empty or exceed 42 characters"));
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
			welcome(cl);
		else
			cl->sendMessage(formatMessage("NOTICE", nick, "Please complete registration with USER command."));
	}
	// Si changement de pseudo
	else if (!isNewNick)
	{
		std::string nickChangeMsg = ":" + oldNick + "!" + cl->getUsername() + "@127.0.0.1 NICK :" + nick + "\r\n";
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

	if (!ft_isValidNick(tokens[1]))
	{
		cl->sendMessage(formatError("INVALID USERNAME", target, "Erroneous username. Only alphanumeric ASCII characters and \"[]{}\\|-_\" are considered valid. Nicknames can't be empty or exceed 42o"));
		cl->sendMessage(formatMessage("INVALID USERNAME", target, "USER :Default username \"guest\" used instead"));
		cl->setUsername("guest");
	}
	else
		cl->setUsername(tokens[1]);

	if (!ft_isValidRealname(tokens[4]))
	{
		cl->sendMessage(formatError("INVALID REALNAME", target, "Erroneous realname. Only alphabetic ASCII characters and spaces are considered valid. Real names can't be empty or exceed 747o"));
		cl->sendMessage(formatMessage("INVALID REALNAME", target, "USER :Default realname \"Guest Person\" used instead"));
		cl->setRealname("Guest Person");
	}
	else
		cl->setRealname(tokens[4]);

	cl->upRegisterLevel();
	if (cl->isRegistered())
		welcome(cl);
	else
		cl->sendMessage(formatMessage("NOTICE", target, "USER command received. Please complete registration with NICK command."));

	return (1);
}

char	Server::whois(Client *const cl, const std::vector<std::string> &tokens)
{
	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, "*", "You have not registered"));
		return (1);
	}
	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, cl->getNick(), "WHOIS :Not enough parameters"));
		return (1);
	}
	if (tokens.size() != 2)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, cl->getNick(), "WHOIS :Too many parameters"));
		return (1);
	}

	if (tokens[1].empty())
	{
		cl->sendMessage(formatError(ERR_NONICKNAMEGIVEN, cl->getNick(), "WHOIS :No nickname given"));
		return (1);
	}

	Client *const	target = getClient(tokens[1]);

	if (!target)
	{
		cl->sendMessage(formatError(ERR_NOSUCHNICK, tokens[1], "WHOIS :No such nickname"));
		return (1);
	}
	cl->sendMessage(formatMessage("USER FOUND", cl->getNick(), "\n" + target->getSheet()));
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
	cl->sendMessage(formatMessage("PONG", "127.0.0.1", tokens.at(1)));
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

	std::string quitMessage = "Client Quit";
	if (tokens.size() >= 2)
		quitMessage = tokens.at(1);

	disconnectClient(cl, quitMessage);

	return (2);
}

char	Server::join(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}
	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "JOIN :Not enough parameters"));
		return (1);
	}
	if (tokens.size() > 3)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, target, "JOIN :Too many parameters"));
		return (1);
	}
	std::vector<std::string>	chans = ft_split(tokens[1], ',');
	std::size_t					csize = chans.size();
	// TODO: Determine wether we handle (and how) empty strings or not
	if (cl->cannotJoinNChannels(csize))
	{
		cl->sendMessage(formatError(ERR_TOOMANYCHANNELS, target, "JOIN :You plan to join too many channels"));
		return (1);
	}

	std::vector<std::string>	keys;
	std::size_t					ksize;
	if (tokens.size() == 3)
		keys = ft_split(tokens[2], ',');
	ksize = keys.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		if (chans[i].empty() || !ft_isValidChannelMask(chans[i]))
		{
			cl->sendMessage(formatError(ERR_BADCHANMASK, chans[i], "JOIN :Bad channel mask. Channel masks need to start with '#', needs to contain at least two characters : except for the first one, they need to be alphabetic or '-'. They can't exceed 42o"));
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
			cl->sendMessage(formatError(ERR_CHANNELISFULL, targetChan->getName(), "Cannot join channel (+l)"));
			continue ;
		}
		std::string	key = targetChan->getKey();
		if (!key.empty() && key != (i < ksize ? keys[i] : ""))
		{
			cl->sendMessage(formatError(ERR_BADCHANNELKEY, targetChan->getName(), "Cannot join channel (+k)"));
			continue ;
		}
		if (targetChan->isInviteOnly())
		{
			if (std::find(targetChan->getInvitations().begin(), targetChan->getInvitations().end(), cl) == targetChan->getInvitations().end())
			{
				cl->sendMessage(formatError(ERR_INVITEONLYCHAN, targetChan->getName(), "Cannot join channel (+i)"));
				continue ;
			}
			targetChan->deleteInvitation(cl);
		}
		if (!(*cl += targetChan))
		{
			cl->sendMessage(formatMessage("Can't join", targetChan->getName(), "JOIN :Channel already joined"));
			continue ;
		}
		*targetChan += cl;

		std::string joinMsg = ":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 JOIN :" + targetChan->getName() + "\r\n";
		targetChan->mall(joinMsg);

		cl->sendMessage(formatMessage("332", cl->getNick(), targetChan->getName() + " :Welcome to " + targetChan->getName()));

		std::string userList = "";
		const std::vector<std::pair<Client*, std::string> >& members = targetChan->getList();
		for (std::vector<std::pair<Client*, std::string> >::const_iterator it = members.begin(); it != members.end(); ++it)
		{
			if (it != members.begin())
				userList += " ";
			userList += it->first->getNick();
		}
		cl->sendMessage(formatMessage("353", cl->getNick(), "= " + targetChan->getName() + " :" + userList));

		cl->sendMessage(formatMessage("366", cl->getNick(), targetChan->getName() + " :End of /NAMES list"));
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

	std::size_t	tsize = tokens.size();
	if (tsize < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, cl->getNick(), "PART :Not enough parameters"));
		return (1);
	}
	if (tsize > 3)
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
			cl->reason = (tsize > 2 ? tokens[2] : "");
			*targetChan -= cl;
			std::string partMsg = ":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 PART " + chans[i] + " :" + cl->reason + "\r\n";
			targetChan->mall(partMsg);
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
	status += "--- DEBUG CLIENT CHANNELS ---\r\n";
	Channel **chans = cl->getChannels();
	for (int i = 0; i < CHPERCL; ++i)
	{
		if (chans[i] != NULL)
			status += "  - " + chans[i]->getName() + (chans[i]->getKey().empty() ? " (key: NA)" : " (key: " + chans[i]->getKey() + ")") + "\r\n";
	}
	status += "----------------------------\r\n";

	cl->sendMessage(status);
	return (1);
}

char	Server::privmsg(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		cl->sendMessage(formatMessage("NOTICE", target, "Current registration level: " + levelToString(cl->getRegisterLevel()) + " (needs to be 3)"));
		return (1);
	}

	// Vérifier si un destinataire est spécifié
	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NORECIPIENT, target, "No recipient given (PRIVMSG)"));
		return (1);
	}

	// Vérifier si un message est spécifié
	if (tokens.size() < 3)
	{
		cl->sendMessage(formatError(ERR_NOTEXTTOSEND, target, "No text to send"));
		return (1);
	}

	if (tokens.size() != 3)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, target, "PRIVMSG :Too many parameters"));
		return (1);
	}

	std::string recipient = tokens.at(1);
	std::string message = tokens.at(2);

	// Diviser les destinataires par virgules
	std::vector<std::string> recipients = ft_split(recipient, ',');

	// Vérifier le nombre maximum de destinataires (limite raisonnable)
	if (recipients.size() > 10)
	{
		cl->sendMessage(formatError(ERR_TOOMANYTARGETS, target, "Too many targets"));
		return (1);
	}

	std::size_t rsize = recipients.size();

	for (std::size_t i = 0; i < rsize; ++i)
	{
		std::string currentTarget = recipients[i];

		// destinataire vide
		if (currentTarget.empty())
		{
			cl->sendMessage(formatError(ERR_NORECIPIENT, target, "No recipient given (PRIVMSG)"));
			continue;
		}

		bool found = false;
		std::size_t csize = _clients.size();

		// si canal
		if (currentTarget[0] == '#' && currentTarget.length() > 1)
		{
			Channel *chan = getChannel(currentTarget);
			if (chan == NULL)
			{
				cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, target, currentTarget + " :No such channel"));
				continue;
			}

			// client membre canal
			const std::vector<std::pair<Client *, std::string> > &chanList = chan->getList();
			std::size_t lsize = chanList.size();
			bool isInChannel = chan->isUserInChannel(cl);

			if (!isInChannel)
			{
				cl->sendMessage(formatError(ERR_CANNOTSENDTOCHAN, target, currentTarget + " :Cannot send to channel"));
				continue;
			}

			// Envoyer msg a tous les membres canal sauf l'expéditeur
			for (std::size_t j = 0; j < lsize; ++j)
			{
				if (chanList[j].first != cl)
					chanList[j].first->sendMessage(":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 PRIVMSG " + currentTarget + " :" + message + "\r\n");
			}
			continue;
		}

		// Rechercher utilisateur
		for (std::size_t j = 0; j < csize; ++j)
		{
			if (_clients[j]->getNick() == currentTarget && _clients[j]->isRegistered())
			{
				_clients[j]->sendMessage(":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 PRIVMSG " + currentTarget + " :" + message + "\r\n");
				found = true;
				break;
			}
		}

		// Si aucun utilisateur trouvé
		if (!found)
		{
			cl->sendMessage(formatError(ERR_NOSUCHNICK, target, currentTarget + " :No such nickname"));
		}
	}

	return (1);
}

char	Server::topic(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}

	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "TOPIC :Not enough parameters"));
		return (1);
	}

	if (tokens.size() > 3)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, target, "TOPIC :Too many parameters"));
		return (1);
	}

	std::string channelName = tokens[1];
	Channel *chan = getChannel(channelName);

	if (!chan)
	{
		cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
		return (1);
	}

	// Vérifier utilisateur dans canal
	const std::vector<std::pair<Client *, std::string> > &chanList = chan->getList();
	bool isInChannel = chan->isUserInChannel(cl);

	if (!isInChannel)
	{
		cl->sendMessage(formatError(ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
		return (1);
	}

	// ARG == 2 -> retourner le topic actuel
	if (tokens.size() == 2)
	{
		if (chan->getTopic().empty())
			cl->sendMessage(formatMessage(RPL_NOTOPIC, target, channelName + " :No topic is set"));
		else
			cl->sendMessage(formatMessage(RPL_TOPIC, target, channelName + " :" + chan->getTopic()));
		return (1);
	}

	// Vérifier si le mode +t est activé
	if (chan->isTopicRestricted())
	{
		// utilisateur opérateur ?
		if (chanList.empty() || chan->isUserOperator(cl) == false)
		{
			cl->sendMessage(formatError(ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
			return (1);
		}
	}

	// nouveau topic
	std::string newTopic = tokens[2];
	chan->setTopic(newTopic);

	// mall topic
	std::string topicMsg = ":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 TOPIC " + channelName + " :" + newTopic + "\r\n";
	chan->mall(topicMsg);

	return (1);
}

char	Server::mode(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}

	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
		return (1);
	}

	std::string channelName = tokens[1];

	// Canal error
	if (channelName[0] != '#')
	{
		cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
		return (1);
	}

	Channel *chan = getChannel(channelName);
	if (!chan)
	{
		cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
		return (1);
	}

	// Vérifier si l'utilisateur est opérateur du canal
	const std::vector<std::pair<Client *, std::string> > &chanList = chan->getList();
	if (chanList.empty() || chan->isUserOperator(cl) == false)
	{
		cl->sendMessage(formatError(ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
		return (1);
	}

	// Si pas de mode spécifié, retourner les modes actuels
	if (tokens.size() == 2)
	{
		std::string modeString = "+";
		if (chan->isInviteOnly()) modeString += "i";
		if (chan->isTopicRestricted()) modeString += "t";
		if (chan->hasUserLimit())
		{
			std::ostringstream convert;
			convert << chan->getUserLimit();
			modeString += "l";
			modeString += " " + convert.str() + " ";
		}

		if (chan->hasKey())
		{
			modeString += "k";
			modeString += " " + chan->getKey() + " ";
		}

		cl->sendMessage(formatMessage(RPL_CHANNELMODEIS, target, channelName + " " + modeString));
		return (1);
	}

	std::string modeStr = tokens[2];
	bool adding = true;
	size_t paramIndex = 3;

	for (size_t i = 0; i < modeStr.length(); ++i)
	{
		char c = modeStr[i];

		if (c == '+')
		{
			adding = true;
		}
		else if (c == '-')
		{
			adding = false;
		}
		else if (c == 'i')
		{
			chan->setInviteOnly(adding);
		}
		else if (c == 't')
		{
			chan->setTopicRestricted(adding);
		}
		else if (c == 'l')
		{
			if (adding)
			{
				if (paramIndex < tokens.size())
				{
					int limit = std::atoi(tokens[paramIndex].c_str());
					if (limit > 0)
						chan->setUserLimit(limit);
					paramIndex++;
				}
			}
			else
			{
				chan->setUserLimit(0); // Supprimer la limite
			}
		}
		else if (c == 'k')
		{
			if (adding)
			{
				if (paramIndex < tokens.size())
				{
					chan->setKey(tokens[paramIndex]);
					paramIndex++;
				}
			}
			else
			{
				chan->setKey("");
			}
		}
		else if (c == 'o')
		{
			if (paramIndex < tokens.size())
			{
				Client *targetUser = getClient(tokens[paramIndex]);
				if (targetUser && chan->isUserInChannel(targetUser))
				{
					if (adding)
					{
						chan->addOperator(targetUser);
					}
					else
					{
						chan->removeOperator(targetUser);
					}
				}
				else if (!targetUser)
				{
					cl->sendMessage(formatError(ERR_NOSUCHNICK, target, tokens[paramIndex] + " :No such nick"));
				}
				else
				{
					cl->sendMessage(formatError(ERR_USERNOTINCHANNEL, target, tokens[paramIndex] + " " + channelName + " :They aren't on that channel"));
				}
				paramIndex++;
			}
			else
			{
				cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
			}
		}
		else
		{
			cl->sendMessage(formatError(ERR_UNKNOWNMODE, target, std::string(1, c) + " :is unknown mode char to me"));
			continue;
		}
	}

	// Notifier le changement de mode
	std::string modeMsg = ":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 MODE " + channelName + " " + modeStr;
	if (paramIndex > 3)
	{
		for (size_t i = 3; i < paramIndex && i < tokens.size(); ++i)
			modeMsg += " " + tokens[i];
	}
	modeMsg += "\r\n";
	chan->mall(modeMsg);

	return (1);
}

char	Server::kick(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}

	if (tokens.size() < 3)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "KICK :Not enough parameters"));
		return (1);
	}

	std::string channelName = tokens[1];
	std::string kickNick = tokens[2];
	std::string reason = (tokens.size() > 3) ? tokens[3] : cl->getNick();

	Channel *chan = getChannel(channelName);
	if (!chan)
	{
		cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
		return (1);
	}

	// Kick est opérateur ?
	const std::vector<std::pair<Client *, std::string> > &chanList = chan->getList();
	if (chanList.empty() || chan->isUserOperator(cl) == false)
	{
		cl->sendMessage(formatError(ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
		return (1);
	}

	// Find user to kick
	Client *kickUser = getClient(kickNick);
	if (!kickUser)
	{
		cl->sendMessage(formatError(ERR_NOSUCHNICK, target, kickNick + " :No such nick"));
		return (1);
	}

	// Vérifier si l'utilisateur est dans le canal
	bool isInChannel = chan->isUserInChannel(kickUser);

	if (!isInChannel)
	{
		cl->sendMessage(formatError(ERR_NOTONCHANNEL, target, kickNick + " :They aren't on that channel"));
		return (1);
	}

	// Effectuer le kick
	std::string kickMsg = ":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 KICK " + channelName + " " + kickNick + " :" + reason + "\r\n";
	chan->mall(kickMsg);

	// Retirer l'utilisateur du canal
	*kickUser -= chan;
	*chan -= kickUser;

	return (1);
}

char	Server::who(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}

	if (tokens.size() < 2)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "WHO :Not enough parameters"));
		return (1);
	}

	std::string mask = tokens[1];

	// Si c'est un canal
	if (mask[0] == '#')
	{
		Channel *chan = getChannel(mask);
		if (!chan)
		{
			cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, target, mask + " :No such channel"));
			cl->sendMessage(formatMessage(RPL_ENDOFWHO, target, mask + " :End of WHO list"));
			return (1);
		}

		const std::vector<std::pair<Client *, std::string> > &chanList = chan->getList();
		for (size_t i = 0; i < chanList.size(); ++i)
		{
			Client *user = chanList[i].first;
			std::string op = chan->isUserOperator(user) ? "@" : ""; // @ = opérateur, "" = non opérateur

			// Format: RPL_WHOREPLY
			cl->sendMessage(formatMessage(RPL_WHOREPLY, target,
				mask + " " + user->getUsername() + " 127.0.0.1 " +
				user->getNick() + " " + op + " :0 " + user->getRealname()));
		}
	}
	else
	{
		// WHO pour un utilisateur spécifique
		Client *user = getClient(mask);
		if (user && user->isRegistered())
		{
			cl->sendMessage(formatMessage(RPL_WHOREPLY, target,
				"* " + user->getUsername() + " 127.0.0.1 " +
				user->getNick() + " :0 " + user->getRealname()));
		}
	}

	cl->sendMessage(formatMessage(RPL_ENDOFWHO, target, mask + " :End of WHO list"));
	return (1);
}

char	Server::invite(Client *const cl, const std::vector<std::string> &tokens)
{
	std::string target = cl->getNick().empty() ? "*" : cl->getNick();

	if (!cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_NOTREGISTERED, target, "You have not registered"));
		return (1);
	}

	if (tokens.size() < 3)
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "INVITE :Not enough parameters"));
		return (1);
	}

	if (tokens.size() > 3)
	{
		cl->sendMessage(formatError(ERR_TOOMANYPARAMS, target, "INVITE :Too many parameters"));
		return (1);
	}

	std::string targetNick = tokens[1];
	std::string channelName = tokens[2];

	Channel *chan = getChannel(channelName);
	if (!chan)
	{
		cl->sendMessage(formatError(ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
		return (1);
	}

	// Si utilisateur pas dans canal
	if (!chan->isUserInChannel(cl))
	{
		cl->sendMessage(formatError(ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
		return (1);
	}

	// Utilisateur cible
	Client *targetUser = getClient(targetNick);
	if (!targetUser)
	{
		cl->sendMessage(formatError(ERR_NOSUCHNICK, target, targetNick + " :No such nick"));
		return (1);
	}

	// User dans le canal
	if (chan->isUserInChannel(targetUser))
	{
		cl->sendMessage(formatError(ERR_USERONCHANNEL, target, targetNick + " " + channelName + " :is already on channel"));
		return (1);
	}

	// MODE +i, seuls les opérateurs peuvent inviter
	if (chan->isInviteOnly() && !chan->isUserOperator(cl))
	{
		cl->sendMessage(formatError(ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
		return (1);
	}

	// Add invitation
	chan->addInvitation(targetUser);

	// Envoyer l'invitation à l'utilisateur cible
	targetUser->sendMessage(":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 INVITE " + targetNick + " :" + channelName + "\r\n");

	// Confirmer l'invitation à celui qui invite
	cl->sendMessage(formatMessage(RPL_INVITING, target, targetNick + " " + channelName));

	return (1);
}