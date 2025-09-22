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
		_clients[i]->sendMessage(formatMessage("NOTICE", _clients[i]->getNick().empty() ? "*" : _clients[i]->getNick(), "Server shutdown."));

	while (!_clients.empty())
		disconnectClient(_clients[0], "Server shutdown");

	while (!_channels.empty())
		*this -= _channels[0];
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
			break ;
		}
	}
	return (*this);
}

Server	&Server::operator+=(Channel *const ch)
{
	_channels.push_back(ch);
	return (*this);
}

Server	&Server::operator-=(Channel *const ch)
{
	std::size_t	csize = _channels.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		if (_channels[i] == ch)
		{
			delete (ch);
			_channels.erase(_channels.begin() + i);
			break ;
		}
	}
	return (*this);
}
/* ******************************* |Operators| ******************************* */

/**
 * Analyse une commande IRC et retourne ses paramètres
 * @param message Le message complet
 * @return Un vecteur contenant tous les paramètres
 */
const std::vector<std::string> parseIrcMessage(const std::string &message)
{
	std::vector<std::string> params;
	const size_t MAX_PARAMS = 15; // RFC 1459: Maximum 15 paramètres

	params.clear();

	size_t start = message.find_first_not_of(" \t\r\n\v\f");
	if (start == std::string::npos)
		return params;

	if (message[start] == ':')
		return params;

	// Découper en paramètres
	std::istringstream iss(message.substr(start));
	std::string token;

	while (iss >> token && params.size() < MAX_PARAMS)
	{
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

	// Si limite paramètres atteint
	if (params.size() >= MAX_PARAMS)
	{
		std::string remaining;
		if (std::getline(iss, remaining) && !remaining.empty())
		{
			// Tronquer et ajouter un indicateur
			params.push_back("ERR_TOOMANYPARAMS");
		}
	}

	return params;
}

inline void	Server::addEvent(const std::string &cmd, char (Server::*f)(Client *const, const std::vector<std::string> &tokens))
{
	_events.push_back(std::make_pair(cmd, f));
}


/* **************************** |Channel Related| **************************** */
Channel	*Server::getChannel(const std::string &name) const
{
	std::size_t	csize = _channels.size();
	for (std::size_t i = 0; i < csize; ++i)
		if (_channels[i]->getName() == name)
			return (_channels[i]);
	return (NULL);
}
/* **************************** |Channel Related| **************************** */


Client	*Server::getClient(const std::string &nick) const
{
	std::size_t	csize = _clients.size();
	for (std::size_t i = 0; i < csize; ++i)
	{
		Client *const	current = _clients[i];
		if (current->getNick() == nick && current->isRegistered())
			return (current);
	}
	return (NULL);
}


/* ****************************** |Server Init| ****************************** */
void	Server::initEvents(void)
{
	addEvent("PASS", &Server::pass);
	addEvent("NICK", &Server::nick);
	addEvent("USER", &Server::user);
	addEvent("WHOIS", &Server::whois);
	addEvent("PING", &Server::ping);
	addEvent("JOIN", &Server::join);
	addEvent("PART", &Server::part);
	addEvent("DEBUG", &Server::debug);
	addEvent("PRIVMSG", &Server::privmsg);
	addEvent("TOPIC", &Server::topic);
	addEvent("MODE", &Server::mode);
	addEvent("KICK", &Server::kick);
	addEvent("WHO", &Server::who);
	addEvent("INVITE", &Server::invite);
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
inline std::string formatMessage(const std::string &code, const std::string &target, const std::string &message)
{
	std::string source = ":" + std::string("127.0.0.1"); // Nom de votre serveur
	return (source + " " + code + " " + target + " :" + message + "\r\n");
}

// Fonction utilitaire pour formater les messages d'erreur selon RFC
inline std::string formatError(const std::string &code, const std::string &target, const std::string &message)
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

	// Mode non bloquant
	if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
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

// signal handler for SIGINT and SIGQUIT
void	Server::signalHandler(int signum)
{
	std::cout << "\n🛑 Server shutdown signal (" << signum << ") received." << std::endl;
	std::cout << "Closing all client connections..." << std::endl;
	if (_instance)
		_running = false;
}

// Configurer structures pollfd serveur et clients
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
		// Signal detecté pour poll
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
				// Client déconnecté brutalement => disconnectClient
				disconnectClient(_clients[j], "Connection timeout");
				break;
			}
			else if (result == 2)
			{
				// Client déconnecté QUIT
				break;
			}
			// result == 1 OK
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
			std::string nickname = _clients[j]->isRegistered() ? _clients[j]->getNick() : "unregistered";
			std::cout << "📡 Client '" << nickname << "' disconnected abruptly (fd=" << _pollfds[pollfdIndex].fd << ")" << std::endl;
			disconnectClient(_clients[j], "Connection lost");
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
		std::cerr << "Error. Accept failed" << std::endl;
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
	if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
	{
		std::cerr << "Error. Failed to set client socket non-blocking" << std::endl;
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

	while (69 != *(int *)"UNICORN")
	{
		std::string buffer = client->getBuffer();

		// Chercher la première fin de ligne complète
		size_t pos = std::string::npos;
		size_t lineEndSize = 0;

		// Chercher \r\n (protocole IRC standard)
		pos = buffer.find("\r\n");
		if (pos != std::string::npos)
		{
			lineEndSize = 2;
		}
		else
		{
			// Sinon chercher juste \n (compatibilité Unix)
			pos = buffer.find("\n");
			if (pos != std::string::npos)
			{
				lineEndSize = 1;
			}
		}

		if (pos == std::string::npos)
			break ; // Pas de ligne complète, attendre plus de données

		std::string message = buffer.substr(0, pos);
		client->clearBuffer();

		if (pos + lineEndSize < buffer.length())
		{
			// Conserver reste buffer prochain passage
			client->appendToBuffer(buffer.substr(pos + lineEndSize));
		}

		if (message.length() > 510) // 512 - 2 (\r\n)
		{
			std::cout << "Message trop long du client fd=" << client->getFd() << " (" << message.length() << " octets), tronqué" << std::endl;
			message = message.substr(0, 510);
			client->sendMessage(formatError(ERR_INPUTTOOLONG, client->getNick().empty() ? "*" : client->getNick(), "Message trop long, tronqué à 510 caractères"));
		}

		std::cout << "Message complet reçu du client fd=" << client->getFd() << ": [" << message << "]" << std::endl;

		// Découper le message en tokens pour identifier la commande
		const std::vector<std::string> tokens = parseIrcMessage(message);
		if (tokens.empty())
			continue;

		std::string command = tokens[0];

		if (command == "QUIT")
		{
			this->quit(client, tokens);
			return 2;
		}

		// Traiter les autres commandes normalement
		std::size_t	esize = _events.size();
		for (std::size_t i = 0; i < esize; ++i)
		{
			if (_events[i].first == command)
			{
				// Si la commande retourne 0, le client doit être déconnecté
				if (!((this->*_events[i].second)(client, tokens)))
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
	{
		client->reason = reason;
		Channel	**const chans = client->getChannels();
		for (int i = 0; i < CHPERCL; ++i)
			if (*(chans + i) != NULL)
				**(chans + i) -= client;
		mall(":" + nickname + " QUIT :Quit: " + reason + "\r\n");
	}

	// Supprimer le client à la fin
	*this -= client;
}
