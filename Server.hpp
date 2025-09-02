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

class	Server
{
	private:
		const unsigned short		_port;
		const std::string			_pw;
		int							_fd;
		std::vector<Client *>		_clients;
		static Server*              _instance;
		static bool					_running;
		std::vector<
			std::pair<
				const std::string,
				char (Server::*)(Client *const, const std::string &)
			>
		>							_events;

		// Structure pour poll()
		std::vector<pollfd>         _pollfds;

		// Méthodes privées pour l'organisation interne
		void                        setupPollFds(void);
		void                        handlePollEvents(int activity);
		bool                        checkSocketErrors(int activity);
		void                        handleClientInput(size_t pollfdIndex);
		void                        handleClientError(size_t pollfdIndex);
		void						addEvent(const std::string &, char (Server::*)(Client *const, const std::string &));
		void						initEvents(void);
	private: /* -Events/Commands- */
		char						pass(Client *const, const std::string &);
		char						nick(Client *const, const std::string &);
		char						user(Client *const, const std::string &);

	public: /* -CDstructors- */
		Server(const unsigned short &port, const std::string &pw);
		~Server(void);
	public: /* -Getters- */
		const unsigned short		&getPort(void) const { return (_port); };
		const std::string			&getPw(void) const { return (_pw); };
		const int					&getFd(void) const { return (_fd); };
		const std::vector<Client *>	&getClients(void) const { return (_clients); };
		static const Server			*getInstance(void) { return (_instance); };
		static const bool			&isRunning(void) { return (_running); };
	public: /* -Methods- */
		Server						&operator+=(Client *const cl);
		Server						&operator-=(Client *const cl);

		// Méthodes d'initialisation
		bool						initServer(void);
		bool                        initSocketOptions(void);
		bool                        bindAndListen(void);

		// Méthode principale
		void						run(void);

		// Gestion des clients
		void                        handleNewConnection(void);
		bool                        configureClientSocket(int client_fd);
		Client*                     createClient(int client_fd, struct sockaddr_in &client_addr);
		bool                        handleClientMessage(Client *client);
		void                        disconnectClient(Client *client);

		// Gestion des signaux
		static void					signalHandler(int sig);

		// Setter de l'instance
		static void                 setInstance(Server* srv) { _instance = srv; }
};
