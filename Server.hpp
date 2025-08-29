/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/26 15:30:15 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/29 13:57:10 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <unistd.h>
#include <vector>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <poll.h>
#include <fcntl.h>
#include <string.h>

class	Client;

typedef struct s_srv_set
{
	unsigned short			_port;
	std::string				_pw;
	int						_fd;
	std::vector<Client *>	*_clients;
}	t_srv_set;

class	Server
{
	private:
		const unsigned short		_port;
		const std::string			_pw;
		int							_fd;
		std::vector<Client *>		*_clients;
		static Server*              _instance;
		static volatile bool        _running;

		// Structure pour poll()
		std::vector<pollfd>         _pollfds;

		// Méthodes privées pour l'organisation interne
		void                        setupPollFds(void);
		void                        handlePollEvents(int activity);
		bool                        checkSocketErrors(int activity);

	public: /* -CDstructors- */
		Server(void) : _port(), _pw(), _fd() {};
		Server(const unsigned short &port, const std::string &pw);
		~Server(void);
	public: /* -Getters- */
		const unsigned short		&getPort(void) const { return (_port); };
		const std::string			&getPw(void) const { return (_pw); };
		const int					&getFd(void) const { return (_fd); };
		const std::vector<Client *>	&getClients(void) const { return (*_clients); };
	public: /* -Methods- */
		Server						&operator+=(Client *const cl);
		Server						&operator-=(Client *const cl);

		// Méthodes d'initialisation
		unsigned char				initVector(void);
		bool						initServer(void);
		bool                        initSocketOptions(void);
		bool                        bindAndListen(void);

		// Méthode principale
		void						run(void);

		// Gestion des clients
		void                        handleNewConnection(void);
		bool                        handleClientMessage(Client *client);
		void                        disconnectClient(Client *client);

		// Gestion des signaux
		static void					signalHandler(int sig);

		// Méthodes pour la gestion de l'instance
		static void                 setInstance(Server* srv) { _instance = srv; }
		static Server*              getInstance() { return _instance; }
		static bool                 isRunning() { return _running; }
};
