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
Server* Server::_instance = NULL;
bool Server::_running = true;

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
		}
	}
	return (*this);
}

bool Server::initServer(void)
{
	signal(SIGINT, Server::signalHandler);
	signal(SIGQUIT, Server::signalHandler);
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
	{
		std::cerr << "Error. Socket failed." << std::endl;
		return false;
	}

	// Permet la réutilisation du port immédiatement après fermeture
	int opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "Error. Setsockopt failed." << std::endl;
		return false;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		std::cerr << "Error. Bind failed." << std::endl;
		return false;
	}

	if (listen(_fd, SOMAXCONN) < 0)
	{
		std::cerr << "Error. Listen failed." << std::endl;
		return false;
	}

	std::cout << "Server listening on port " << _port << std::endl;
	return true;
}

void Server::run(void)
{
	while (_running)
	{
		// Configuration pour poll()
		struct pollfd fds[1];
		fds[0].fd = _fd;
		fds[0].events = POLLIN;

		// Timeout de 1000ms (1 seconde)
		int activity = poll(fds, 1, 1000);

		if (activity < 0 && errno != EINTR)
		{
			std::cerr << "Error. Poll failed." << std::endl;
			continue;
		}

		// Si le signal a été reçu, sortir proprement
		if (!_running)
			break;

		// S'il y a une activité sur le socket du serveur
		if (activity > 0 && (fds[0].revents & POLLIN))
		{
			// Accepter la nouvelle connexion
			int client_fd = accept(_fd, NULL, NULL);
			if (client_fd < 0)
			{
				std::cerr << "Error. Accept failed." << std::endl;
				continue;
			}

			try {
				Client *tmp = new Client(client_fd);
				*this += tmp;
				std::cout << "New client connected, fd=" << client_fd << std::endl;
			}
			catch (const std::exception &e) {
				std::cerr << "Client error: " << e.what() << std::endl;
			}
		}
	}
}

// Static signal handler for SIGINT and SIGQUIT
void Server::signalHandler(int signum)
{
	std::cout << "\nInterrupt signal (" << signum << ") received.\n";
	if (_instance)
		_running = false; // Permettre une sortie propre de la boucle run()
}
