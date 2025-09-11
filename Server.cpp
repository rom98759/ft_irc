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

		this->operator-=(_clients[0]);
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
std::vector<std::string> parseIrcMessage(const std::string &message, const std::string &command)
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
