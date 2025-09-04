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
#include <string.h> // Pour strerror

// Initialisation des variables statiques
Server	*Server::_instance = NULL;
bool	Server::_running = true;

Server::Server(const unsigned short &port, const std::string &pw)
	: _port(port), _pw(pw), _fd(-1) {}

/*
Behaviour:
	1- Ensures every event is ended.
	2- Frees variables and close fds.
*/
Server::~Server(void)
{
	for (int i = 0; i < (int)_clients.size(); ++i)
		delete (_clients[i]);
	close(_fd);
	std::cout << "\nServer Shutdown !" << std::endl;
}

Server	&Server::operator+=(Client *const cl)
{
	_clients.push_back(cl);
	return (*this);
}

Server	&Server::operator-=(Client *const cl)
{
	for (int i = 0; i < (int)_clients.size(); ++i)
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

static std::size_t	ft_skipSpaces(const std::string &s, const std::size_t &start)
{
	return (s.find_first_not_of(" \t\n\v\f\r", start));
}

/* ****************************| EVENTS/COMMANDS |**************************** */
/* ********* char	(Server::*)(Client *const, const std::string &); ********* */
/* *************************************************************************** */

char	Server::pass(Client *const cl, const std::string &cmd)
{
	if (cl->isRegistered())
	{
		cl->sendMessage("Error. Already registered.\r\n");
		return (1);
	}
	std::size_t	idx = ft_skipSpaces(cmd, 4);
	if (idx >= cmd.size())
	{
		cl->sendMessage("Error. No PASS given.\r\n");
		return (1);
	}
	std::string	pass = cmd.substr(idx);
	if (pass == _pw)
	{
		cl->sendMessage("Match ! You are now registered.\r\n");
		cl->setRegistered(1);
	}
	else
		cl->sendMessage("Wrong PASS. Please retry.\r\n");
	return (1);
}

char	Server::nick(Client *const cl, const std::string &cmd)
{
	(void) cmd;
	cl->sendMessage("This feature isn't available.\r\n");
	return (1);
}

char	Server::user(Client *const cl, const std::string &cmd)
{
	(void) cmd;
	cl->sendMessage("This feature isn't available.\r\n");
	return (1);
}

/* ****************************| EVENTS/COMMANDS |**************************** */

inline void	Server::addEvent(const std::string &cmd, char (Server::*f)(Client *const, const std::string &))
{
	_events.push_back(std::make_pair(cmd, f));
}

void	Server::initEvents(void)
{
	addEvent("PASS", &Server::pass);
	addEvent("NICK", &Server::nick);
	addEvent("USER", &Server::user);
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
	for (size_t i = 0; i < _clients.size(); ++i)
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
		for (size_t i = 1; i < _pollfds.size() && activity > 0; ++i)
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

// Gère les données envoyées par un client
void	Server::handleClientInput(size_t pollfdIndex)
{
	// Trouver le client correspondant au descripteur
	for (size_t j = 0; j < _clients.size(); ++j)
	{
		if (_clients[j]->getFd() == _pollfds[pollfdIndex].fd)
		{
			if (!handleClientMessage(_clients[j]))
			{
				// Client déconnecté donc delete
				disconnectClient(_clients[j]);
				break;
			}
		}
	}
}

// Gère les erreurs de connexion avec un client
void	Server::handleClientError(size_t pollfdIndex)
{
	// Gérer la déconnexion ou l'erreur
	for (size_t j = 0; j < _clients.size(); ++j)
	{
		if (_clients[j]->getFd() == _pollfds[pollfdIndex].fd)
		{
			std::cout << "Client déconnecté ou erreur détectée (fd=" << _pollfds[pollfdIndex].fd << ")" << std::endl;
			disconnectClient(_clients[j]);
			break;
		}
	}
}

// Static signal handler for SIGINT and SIGQUIT
void	Server::signalHandler(int signum)
{
	std::cout << "\nInterrupt signal (" << signum << ") received.\n";
	if (_instance)
		_running = false;

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
		client->sendMessage("220 Welcome to IRC Server\r\n");
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

static inline char	ft_match(const std::string &srvCmd, const std::string &clInput)
{
	std::size_t	sc = srvCmd.size();

	return (clInput.size() > sc && srvCmd == clInput.substr(0, sc)
		&& std::isspace(clInput.at(sc)));
}

// Gère les messages reçus d'un client
// Retourne false si le client s'est déconnecté
bool	Server::handleClientMessage(Client *client)
{
	if (!client->readFromSocket())
	{
		return false; // Client déconnecté
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

		for (int i = 0; i < (int)_events.size(); ++i)
			if (ft_match(_events[i].first, message))
				if (!((this->*_events[i].second)(client, message)))
					return (0);

		// Exemple : Echo du message reçu
		// client->sendMessage("ECHO: " + message + "\r\n");
	}

	return true;
}

// Déconnecte proprement un client
void	Server::disconnectClient(Client *client)
{
	std::cout << "Déconnexion du client fd=" << client->getFd() << std::endl;

	// Envoyer message déconnexion autres clients si nécessaire
	// TODO: implémentée apres gestion canaux

	// Supprimer le client de la liste
	*this -= client;
}
