/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/26 15:30:10 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/29 13:57:07 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"

// Initialisation des variables statiques
Server	*Server::_instance = NULL;
bool	Server::_running = true;


/* ****************************** |CDstructors| ****************************** */
Server::Server(const unsigned short &port, const std::string &pw)
	: _port(port), _pw(pw), _fd(-1) {}

/*
Behaviour:
	1- Ensures every event is ended.
	2- Frees variables and close fds.
*/
Server::~Server(void)
{
	std::size_t	csize = _clients.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		_clients[i]->sendMessage(formatMessage("NOTICE", _clients[i]->getNick().empty() ? "*" : _clients[i]->getNick(), "Server shutdown."));
		this->operator-=(_clients[i]);
	}
	close(_fd);
	std::cout << "\nServer Shutdown !" << std::endl;
}
/* ****************************** |CDstructors| ****************************** */



/* ******************************* |Operators| ******************************* */
Server	&Server::operator+=(Client *const cl)
{
	_clients.push_back(cl);
	return (*this);
}

Server	&Server::operator-=(Client *const cl)
{
	std::size_t	csize = _clients.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		if (_clients[i] == cl)
		{
			delete (cl);
			_clients.erase(_clients.begin() + i);
			break;
		}
	}
	return (*this);
}
/* ******************************* |Operators| ******************************* */

/**
 * Analyse une commande IRC et retourne ses paramètres
 * @param message Le message complet
 * @param command La commande à extraire (vide si on veut parser tout le message)
 * @return Un vecteur contenant tous les paramètres
 */
static std::vector<std::string> parseIrcMessage(const std::string &message, const std::string &command = "")
{
	std::vector<std::string> params;

	// Si une commande est spécifiée, extraire ce qui suit la commande
	std::string paramStr;
	if (command.empty())
		paramStr = message;
	else
	{
		size_t cmdPos = message.find(command);
		if (cmdPos == std::string::npos)
			return params; // Commande non trouvée

		paramStr = message.substr(cmdPos + command.size());
	}

	// Ignorer les espaces initiaux
	size_t start = paramStr.find_first_not_of(" \t\r\n\v\f");
	if (start == std::string::npos)
		return params; // Que des espaces

	// Découper en paramètres
	std::istringstream iss(paramStr.substr(start));
	std::string token;

	while (iss >> token)
	{
		// Si on trouve un paramètre commençant par ':', prendre tout le reste
		if (token[0] == ':')
		{
			std::string trailing;
			std::getline(iss, trailing);
			params.push_back(token.substr(1) + trailing);
			break;
		}
		else
		{
			params.push_back(token);
		}
	}

	return params;
}

// Fonction utilitaire pour ignorer les espaces (gardée pour compatibilité)
static std::size_t ft_skipSpaces(const std::string &s, const std::size_t &start = 0)
{
	std::size_t pos = s.find_first_not_of(" \t\n\v\f\r", start);
	return (pos == std::string::npos) ? s.length() : pos;
}

// Ces fonctions sont maintenues pour la compatibilité avec le code existant
// mais utilisent maintenant les nouvelles fonctions de parsing

static std::string extractCommandParams(const std::string &message, const std::string &command)
{
	std::vector<std::string> params = parseIrcMessage(message, command);
	if (params.empty())
		return "";

	// Si le premier paramètre commence par ':', c'est un trailing parameter
	if (!params.empty() && params[0][0] == ':')
		return params[0].substr(1);

	// Sinon, concaténer tous les paramètres
	std::string result;
	for (size_t i = 0; i < params.size(); ++i)
	{
		if (i > 0)
			result += " ";
		result += params[i];
	}
	return result;
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

	// Pour les autres valeurs (normalement impossible avec 2 bits)
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

	// Ignorer les espaces et extraire uniquement le mot de passe
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

	// Extraire uniquement le nickname sans les espaces potentiels après
	std::string nick = cmd.substr(idx);
	std::size_t end_nick = nick.find_first_of(" \t\n\v\f\r");
	if (end_nick != std::string::npos)
		nick = nick.substr(0, end_nick);

	// Vérifier que le pseudo est valide
	if (!ft_isValidNick(nick))
	{
		cl->sendMessage(formatError(ERR_ERRONEUSNICKNAME, cl->getNick().empty() ? "*" : cl->getNick(), nick + " :Erroneous nickname. Only alphanumeric ASCII characters and \"[]{}\\|\" are considered valid."));
		return (1);
	}

	// Vérifier si le pseudo est déjà utilisé par un autre client
	std::size_t clientsCount = _clients.size();
	for (std::size_t i = 0; i < clientsCount; i++)
	{
		if (_clients[i] != cl && _clients[i]->getNick() == nick)
		{
			cl->sendMessage(formatError(ERR_NICKNAMEINUSE, cl->getNick().empty() ? "*" : cl->getNick(), nick + " :Nickname is already in use"));
			return (1);
		}
	}

	// Si le client n'a pas encore de pseudo, c'est une première définition
	bool isNewNick = cl->getNick().empty();

	// Stocker l'ancien pseudo pour notification (si changement)
	std::string oldNick = cl->getNick();
	std::string targetNick = isNewNick ? "*" : oldNick;

	// Définir le nouveau pseudo
	cl->setNick(nick);

	// Si c'est un nouveau pseudo et que le client n'est pas encore complètement enregistré
	if (isNewNick && !cl->isRegistered())
	{
		cl->upRegisterLevel();
		cl->sendMessage(formatMessage("NICK", nick, nick));

		if (cl->isRegistered())
		{
			// Envoyer les messages de bienvenue standard IRC
			cl->sendMessage(formatMessage(RPL_WELCOME, nick, "Welcome to the IRC Network, " + nick));
			cl->sendMessage(formatMessage(RPL_YOURHOST, nick, "Your host is unicorn.42.network, running version 1.0"));
			cl->sendMessage(formatMessage(RPL_MYINFO, nick, "unicorn.42.network 1.0 o o"));
		}
		else
			cl->sendMessage(formatMessage("NOTICE", nick, "Please complete registration with USER command."));
	}
	// Si c'est un changement de pseudo
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

	// Vérifier que le client n'est pas déjà complètement enregistré
	if (cl->isRegistered())
	{
		cl->sendMessage(formatError(ERR_ALREADYREGISTERED, target, "You may not reregister"));
		return (1);
	}

	// Vérifier si on a suffisamment de paramètres
	std::size_t idx = ft_skipSpaces(cmd);
	if (idx >= cmd.size())
	{
		cl->sendMessage(formatError(ERR_NEEDMOREPARAMS, target, "USER :Not enough parameters"));
		return (1);
	}

	// Idéalement, on devrait parser et stocker les informations USER ici
	// USER <username> <hostname> <servername> :<realname>

	// Si le niveau est à 2, c'est probablement que NICK a déjà été traité
	// Dans ce cas, passer directement au niveau 3 (enregistrement complet)
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

	// Sinon, si le niveau est à 1 (PASS validé mais NICK pas encore),
	// on incrémente à 2 et on attend NICK
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

	// Extraire le token sans les espaces potentiels après
	std::string token = cmd.substr(idx);
	std::size_t end_token = token.find_first_of(" \t\n\v\f\r");
	if (end_token != std::string::npos)
		token = token.substr(0, end_token);

	// Répondre au PING avec un PONG (format standard IRC)
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

	// Utiliser la nouvelle fonction pour extraire les paramètres
	std::vector<std::string> params = parseIrcMessage(cmd, "");
	std::string quitMessage = params.empty() ? "Client Quit" : params[0];

	// Message au format IRC standard pour les QUIT
	cl->sendMessage(formatMessage("QUIT", cl->getNick(), "Quit: " + quitMessage));
	disconnectClient(cl, quitMessage);

	return (1);
}

// Fonction de débogage pour afficher l'état d'enregistrement du client
char	Server::debug(Client *const cl, const std::string &cmd)
{
	(void)cmd; // cmd n'est pas utilisé ici

	std::string status = "--- DEBUG CLIENT STATUS ---\r\n";
	status += "- Register Level: " + levelToString(cl->getRegisterLevel()) + "/3\r\n";
	status += "- Is Registered: " + std::string(cl->isRegistered() ? "Yes" : "No") + "\r\n";
	status += "- Nickname: " + (cl->getNick().empty() ? "[Not Set]" : cl->getNick()) + "\r\n";
	status += "- Binary RegisterLevel: ";

	// Afficher la valeur binaire de _registerLevel
	unsigned char level = cl->getRegisterLevel();
	for (int i = 1; i >= 0; i--)
		status += ((level >> i) & 1) ? "1" : "0";

	status += "\r\n";
	status += "-------------------------\r\n";

	cl->sendMessage(status);
	return (1);
}
/* ****************************| EVENTS/COMMANDS |**************************** */


inline void	Server::addEvent(const std::string &cmd, char (Server::*f)(Client *const, const std::string &))
{
	_events.push_back(std::make_pair(cmd, f));
}


/* ****************************** |Server Init| ****************************** */
void	Server::initEvents(void)
{
	addEvent("PASS", &Server::pass);
	addEvent("NICK", &Server::nick);
	addEvent("USER", &Server::user);
	addEvent("PING", &Server::ping);
	addEvent("DEBUG", &Server::debug);
}

bool	Server::initServer(void)
{
	// signaux
	signal(SIGINT, Server::signalHandler);
	signal(SIGQUIT, Server::signalHandler);

	// socket
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
	{
		std::cerr << "Error. Socket failed: " << strerror(errno) << std::endl;
		return false;
	}

	// socket opt
	if (!initSocketOptions())
		return false;

	// Lier socket et écouter
	if (!bindAndListen())
		return false;

	initEvents();

	std::cout << "Server listening on port " << _port << std::endl;
	return true;
}
/* ****************************** |Server Init| ****************************** */


// Fonction utilitaire pour formater les messages de réponse selon RFC
std::string Server::formatMessage(const std::string &code, const std::string &target, const std::string &message) const
{
	std::string source = ":" + std::string("irc.server.com"); // Nom de votre serveur
	return (source + " " + code + " " + target + " :" + message + "\r\n");
}

// Fonction utilitaire pour formater les messages d'erreur selon RFC
std::string Server::formatError(const std::string &code, const std::string &target, const std::string &message) const
{
	return formatMessage(code, target, message);
}

void	Server::mall(const std::string &msg) const
{
	int	csize = _clients.size();

	for (int i = 0; i < csize; ++i)
		if (_clients[i]->isRegistered())
			_clients[i]->sendMessage(msg);
}

// Initialiser les options du socket
bool	Server::initSocketOptions(void)
{
	// Reutiliser port apres fermeture
	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "Error. Setsockopt failed: " << strerror(errno) << std::endl;
		close(_fd);
		return false;
	}

	//Configurer mode non bloquant (recv/send)
	int flags = fcntl(_fd, F_GETFL, 0);
	if (flags < 0 || fcntl(_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		std::cerr << "Error. Failed to set non-blocking mode: " << strerror(errno) << std::endl;
		close(_fd);
		return false;
	}

	return true;
}

// Lier le socket à un port et commencer à écouter
bool	Server::bindAndListen(void)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	// Lier le socket à l'adresse et au port
	if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		std::cerr << "Error. Bind failed: " << strerror(errno) << std::endl;
		close(_fd);
		return false;
	}

	// Écouter les connexions entrantes
	if (listen(_fd, SOMAXCONN) < 0)
	{
		std::cerr << "Error. Listen failed: " << strerror(errno) << std::endl;
		close(_fd);
		return false;
	}

	return true;
}


/* ****************************** |Server Loop| ****************************** */
void	Server::run(void)
{
	while (_running)
	{
		// Configurer pollfd serveur / clients
		setupPollFds();

		// Attendre activity 1000 ms = 1 seconde
		int activity = poll(&_pollfds[0], _pollfds.size(), 1000);

		// Check errors
		if (!checkSocketErrors(activity))
			continue;

		// Si signal alors fermer
		if (!_running)
			break;

		// Traiter messages/actions poll
		if (activity > 0)
			handlePollEvents(activity);
	}
}

// Static signal handler for SIGINT and SIGQUIT
void	Server::signalHandler(int signum)
{
	std::cout << "\nInterrupt signal (" << signum << ") received.\n";
	if (_instance)
		_running = false;

}

// Configurer les structures pollfd pour le serveur et les clients
void	Server::setupPollFds(void)
{
	_pollfds.clear();

	// Ajouter socket serveur
	pollfd server_pollfd;
	server_pollfd.fd = _fd;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;
	_pollfds.push_back(server_pollfd);

	// Ajouter tous clients
	std::size_t	csize = _clients.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		pollfd client_pollfd;
		client_pollfd.fd = _clients[i]->getFd();
		client_pollfd.events = POLLIN;
		client_pollfd.revents = 0;
		_pollfds.push_back(client_pollfd);
	}
}

// Vérifier les erreurs de poll et décider de continuer ou non
bool	Server::checkSocketErrors(int activity)
{
	// Erreur poll
	if (activity < 0)
	{
		// Signal detecté
		if (errno == EINTR)
		{
			std::cout << "Poll interrupted by signal." << std::endl;
			return false;
		}
		std::cerr << "Error. Poll failed: " << strerror(errno) << std::endl;
		return false;
	}

	else if (activity == 0)
	{
		// Timeout, rien à faire
		return false;
	}

	return true;
}

// Traiter les événements détectés par poll
void	Server::handlePollEvents(int activity)
{
	// Nouvelle connexion (ET binaire si true) sur server
	if (_pollfds[0].revents & POLLIN)
	{
		handleNewConnection();
		activity--;
	}

	// activité sockets clients si activity > 0
	if (activity > 0)
	{
		std::size_t	psize = _pollfds.size();
		for (std::size_t i = 1; i < psize && activity > 0; ++i)
		{
			if (_pollfds[i].revents & POLLIN)
			{
				handleClientInput(i);
				activity--;
			}
			else if (_pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				handleClientError(i);
				activity--;
			}
		}
	}
}

/* ********************* |Server Loop : Client Handling| ********************* */
// Gère les données envoyées par un client
void	Server::handleClientInput(size_t pollfdIndex)
{
	// Trouver le client correspondant au descripteur
	std::size_t	csize = _clients.size();
	for (std::size_t j = 0; j < csize; ++j)
	{
		if (_clients[j]->getFd() == _pollfds[pollfdIndex].fd)
		{
			int result = handleClientMessage(_clients[j]);

			if (result == 0)
			{
				// Client déconnecté brutalement, appeler disconnectClient
				disconnectClient(_clients[j], "[ERROR] Connection timeout");
				break;
			}
			else if (result == 2)
			{
				// Client déjà déconnecté via QUIT, ne rien faire de plus
				break;
			}
			// Si result == 1, tout va bien, continuer
		}
	}
}

// Gère les erreurs de connexion avec un client
void	Server::handleClientError(size_t pollfdIndex)
{
	// Gérer la déconnexion ou l'erreur
	std::size_t	csize = _clients.size();
	for (std::size_t j = 0; j < csize; ++j)
	{
		if (_clients[j]->getFd() == _pollfds[pollfdIndex].fd)
		{
			std::cout << "Client déconnecté ou erreur détectée (fd=" << _pollfds[pollfdIndex].fd << ")" << std::endl;
			disconnectClient(_clients[j], "[ERROR]");
			break;
		}
	}
}

// Gère une nouvelle connexion au serveur
void	Server::handleNewConnection(void)
{
	struct sockaddr_in client_addr;
	socklen_t addr_len = sizeof(client_addr);

	int client_fd = accept(_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (client_fd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			std::cerr << "Error. Accept failed: " << strerror(errno) << std::endl;
		return;
	}

	// Configurer le socket client
	if (!configureClientSocket(client_fd))
		return;

	// Créer nouveau client
	Client* client = createClient(client_fd, client_addr);
	if (client)
		client->sendMessage(formatMessage("NOTICE", "Auth", "*** Looking up your hostname...") +
							formatMessage("NOTICE", "Auth", "*** Found your hostname") +
							formatMessage("NOTICE", "Auth", "*** Please enter password with /PASS <password>"));
}

// Configure un socket client en mode non-bloquant
bool	Server::configureClientSocket(int client_fd)
{
	// Mode non bloquant client
	int flags = fcntl(client_fd, F_GETFL, 0);
	if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		std::cerr << "Error. Failed to set client socket non-blocking: " << strerror(errno) << std::endl;
		close(client_fd);
		return false;
	}
	return true;
}

// Crée un nouveau client et l'ajoute à la liste
Client	*Server::createClient(int client_fd, struct sockaddr_in &client_addr)
{
	try {
		Client *tmp = new Client(client_fd);
		*this += tmp;

		// Obtenir l'adresse IP du client
		char ip_str[INET_ADDRSTRLEN];
		// Convertir adresse IP en chaîne
		inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

		std::cout << "New client connected from " << ip_str << ":"
			<< ntohs(client_addr.sin_port) << ", fd=" << client_fd << std::endl;
		// ntohs(client_addr.sin_port) = recuperer adresse:port

		return tmp;
	}
	catch (const std::exception &e) {
		std::cerr << "Client error: " << e.what() << std::endl;
		close(client_fd);
		return NULL;
	}
}

// Gère les messages reçus d'un client
// Retourne:
// 0 = Client s'est déconnecté brutalement (appeler disconnectClient)
// 1 = Tout va bien, continuer
// 2 = Client a quitté proprement via QUIT (ne pas appeler disconnectClient)
int	Server::handleClientMessage(Client *client)
{
	if (!client->readFromSocket())
	{
		return 0; // Client déconnecté brutalement
	}

	size_t	pos;
	while (69 != *(int *)"UNICORN")
	{
		std::string buffer = client->getBuffer();

		// Si buffer avec fin de ligne
		pos = buffer.find("\r\n");
		if (pos == std::string::npos)
			break ;

		std::string message = buffer.substr(0, pos);
		client->clearBuffer();

		if (pos + 2 < buffer.length())
		{
			// Conserver reste buffer prochain passage
			client->appendToBuffer(buffer.substr(pos + 2));
		}

		std::cout << "Message complet reçu du client fd=" << client->getFd() << ": " << message << std::endl;

		// Découper le message en tokens pour identifier la commande
		std::vector<std::string> tokens = parseIrcMessage(message);
		if (tokens.empty())
			continue;

		std::string command = tokens[0];

		// Vérifier si c'est une commande QUIT pour traitement spécial
		if (command == "QUIT")
		{
			// Extraire les paramètres proprement
			std::string params = extractCommandParams(message, "QUIT");

			// Traiter la commande QUIT
			this->quit(client, params);
			// Indiquer que le client a été correctement déconnecté via QUIT
			return 2;
		}

		// Traiter les autres commandes normalement
		std::size_t	esize = _events.size();
		for (std::size_t i = 0; i < esize; ++i)
		{
			if (_events[i].first == command)
			{
				// Extraire les paramètres proprement
				std::string params = extractCommandParams(message, _events[i].first);

				// Si la commande retourne 0, le client doit être déconnecté
				if (!((this->*_events[i].second)(client, params)))
					return 0;
			}
		}
	}

	return 1; // Tout va bien
}

// Déconnecte proprement un client
void Server::disconnectClient(Client *client, const std::string &reason)
{
	if (!client)
		return;

	std::cout << "Déconnexion du client fd=" << client->getFd() << std::endl;

	// Sauvegarder les informations du client avant modification
	bool isRegistered = client->isRegistered();
	std::string nickname = isRegistered ? client->getNick() : "";

	// Envoyer le message de déconnexion AVANT de supprimer le client
	if (isRegistered)
		mall(formatMessage("QUIT", nickname, "Quit: " + reason));

	// Supprimer le client à la fin
	*this -= client;
	// Ne plus utiliser la variable client après cette ligne!
}
/* ********************* |Server Loop : Client Handling| ********************* */

/* ****************************** |Server Loop| ****************************** */
