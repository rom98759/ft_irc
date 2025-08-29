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
Server* Server::_instance = NULL;
volatile bool Server::_running = true;

Server::Server(const unsigned short &port, const std::string &pw)
	: _port(port), _pw(pw), _fd(-1), _clients(NULL)
{}

/*
Behaviour:
	1- Ensures every event is ended.
	2- Frees variables and close fds.
*/
Server::~Server(void)
{
	if (_clients)
	{
		for (int i = 0; i < (int)_clients->size(); ++i)
			delete ((*_clients)[i]);
		delete (_clients);
	}
	close(_fd);
	std::cout << "\nServer Shutdown !" << std::endl;
}

Server	&Server::operator+=(Client *const cl)
{
	_clients->push_back(cl);
	return (*this);
}

Server	&Server::operator-=(Client *const cl)
{
	for (int i = 0; i < (int)_clients->size(); ++i)
	{
		if ((*_clients)[i] == cl)
		{
			delete (cl);
			_clients->erase(_clients->begin() + i);
		}
	}
	return (*this);
}

unsigned char	Server::initVector(void)
{
	try
	{
		_clients = new (std::vector<Client *>);
	}
	catch (const std::exception &e)
	{
		std::cout << "Error. " << e.what() << std::endl;
		return (0);
	}
	return (1);
}

bool Server::initServer(void)
{
	// Configurer les gestionnaires de signaux
	signal(SIGINT, Server::signalHandler);
	signal(SIGQUIT, Server::signalHandler);

	// Créer un socket
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
	{
		std::cerr << "Error. Socket failed: " << strerror(errno) << std::endl;
		return false;
	}

	// Configurer les options du socket
	if (!initSocketOptions())
		return false;

	// Lier le socket et écouter les connexions
	if (!bindAndListen())
		return false;

	std::cout << "Server listening on port " << _port << std::endl;
	return true;
}

// Initialiser les options du socket
bool Server::initSocketOptions(void)
{
	// Permet la réutilisation du port immédiatement après fermeture
	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "Error. Setsockopt failed: " << strerror(errno) << std::endl;
		close(_fd);
		return false;
	}

	// Configurer le socket en mode non-bloquant
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
bool Server::bindAndListen(void)
{
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		std::cerr << "Error. Bind failed: " << strerror(errno) << std::endl;
		close(_fd);
		return false;
	}

	if (listen(_fd, SOMAXCONN) < 0)
	{
		std::cerr << "Error. Listen failed: " << strerror(errno) << std::endl;
		close(_fd);
		return false;
	}

	return true;
}

void Server::run(void)
{
	// Initialiser le vecteur de clients
	if (!initVector())
		return;

	_running = true;
	while (_running)
	{
		// Configurer les structures pollfd pour le serveur et les clients
		setupPollFds();

		// Attendre l'activité avec un timeout de 1000ms (1 seconde)
		int activity = poll(&_pollfds[0], _pollfds.size(), 1000);

		// Vérifier les erreurs et continuer si nécessaire
		if (!checkSocketErrors(activity))
			continue;

		// Si le signal a été reçu, sortir proprement
		if (!_running)
			break;

		// Traiter les événements détectés par poll
		if (activity > 0)
			handlePollEvents(activity);
	}
}

// Configurer les structures pollfd pour le serveur et les clients
void Server::setupPollFds(void)
{
	_pollfds.clear();

	// Ajouter le socket serveur
	pollfd server_pollfd;
	server_pollfd.fd = _fd;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;
	_pollfds.push_back(server_pollfd);

	// Ajouter tous les clients
	for (size_t i = 0; i < _clients->size(); ++i)
	{
		pollfd client_pollfd;
		client_pollfd.fd = (*_clients)[i]->getFd();
		client_pollfd.events = POLLIN;
		client_pollfd.revents = 0;
		_pollfds.push_back(client_pollfd);
	}

	// std::cout << "Surveillance de " << _pollfds.size() << " descripteurs..." << std::endl;
}

// Vérifier les erreurs de poll et décider de continuer ou non
bool Server::checkSocketErrors(int activity)
{
	if (activity < 0)
	{
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
void Server::handlePollEvents(int activity)
{
	// Traiter l'activité sur le socket serveur (nouvelle connexion)
	if (_pollfds[0].revents & POLLIN)
	{
		handleNewConnection();
		activity--;
	}

	// Traiter l'activité sur les sockets clients si activity > 0
	if (activity > 0)
	{
		for (size_t i = 1; i < _pollfds.size() && activity > 0; ++i)
		{
			if (_pollfds[i].revents & POLLIN)
			{
				// Trouver le client correspondant au descripteur
				for (size_t j = 0; j < _clients->size(); ++j)
				{
					if ((*_clients)[j]->getFd() == _pollfds[i].fd)
					{
						if (!handleClientMessage((*_clients)[j]))
						{
							// Si le client s'est déconnecté, on le supprime
							disconnectClient((*_clients)[j]);
							// Sortir de la boucle interne car _clients a changé
							break;
						}
					}
				}
				activity--;
			}
			else if (_pollfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				// Gérer la déconnexion ou l'erreur
				for (size_t j = 0; j < _clients->size(); ++j)
				{
					if ((*_clients)[j]->getFd() == _pollfds[i].fd)
					{
						std::cout << "Client déconnecté ou erreur détectée (fd=" << _pollfds[i].fd << ")" << std::endl;
						disconnectClient((*_clients)[j]);
						// Sortir de la boucle interne car _clients a changé
						break;
					}
				}
				activity--;
			}
		}
	}
}

// Static signal handler for SIGINT and SIGQUIT
void Server::signalHandler(int signum)
{
	std::cout << "\nInterrupt signal (" << signum << ") received.\n";
	if (_instance)
	{
		_running = false; // Permettre une sortie propre de la boucle run()
	}
}

// Gère une nouvelle connexion au serveur
void Server::handleNewConnection(void)
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

	// Configurer le socket client en mode non-bloquant
	int flags = fcntl(client_fd, F_GETFL, 0);
	if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		std::cerr << "Error. Failed to set client socket non-blocking: " << strerror(errno) << std::endl;
		close(client_fd);
		return;
	}

	try {
		Client *tmp = new Client(client_fd);
		*this += tmp;

		// Obtenir l'adresse IP du client
		char ip_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

		std::cout << "New client connected from " << ip_str << ":"
			<< ntohs(client_addr.sin_port) << ", fd=" << client_fd << std::endl;

		// Envoyer un message de bienvenue au client
		tmp->sendMessage("220 Welcome to IRC Server\r\n");
	}
	catch (const std::exception &e) {
		std::cerr << "Client error: " << e.what() << std::endl;
		close(client_fd);
	}
}

// Gère les messages reçus d'un client
// Retourne false si le client s'est déconnecté
bool Server::handleClientMessage(Client *client)
{
	if (!client->readFromSocket())
	{
		return false; // Client déconnecté
	}

	std::string buffer = client->getBuffer();

	// Si le buffer contient une fin de ligne, on traite le message
	size_t pos = buffer.find("\r\n");
	if (pos != std::string::npos)
	{
		std::string message = buffer.substr(0, pos);
		client->clearBuffer();

		if (pos + 2 < buffer.length())
		{
			// Conserver le reste du buffer pour le prochain traitement
			client->appendToBuffer(buffer.substr(pos + 2));
		}

		std::cout << "Message complet reçu du client fd=" << client->getFd() << ": " << message << std::endl;

		// Analyser et traiter le message IRC ici
		// TODO: Implémenter le parsing complet des commandes IRC

		// Exemple simple: Echo du message reçu
		client->sendMessage("ECHO: " + message + "\r\n");
	}

	return true;
}

// Déconnecte proprement un client
void Server::disconnectClient(Client *client)
{
	std::cout << "Déconnexion du client fd=" << client->getFd() << std::endl;

	// Envoyer un message de déconnexion aux autres clients si nécessaire
	// Cette partie sera implémentée une fois que les canaux seront gérés

	// Supprimer le client de la liste
	*this -= client;
}
