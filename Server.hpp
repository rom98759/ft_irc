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

// Inclusions de la bibliothèque standard
#include <iostream>
#include <string>
#include <vector>
#include <cstring>  // Pour memset, strerror, etc.
#include <sstream>  // Pour les opérations stringstream

// Inclusions système
#include <unistd.h>     // Pour close, etc.
#include <sys/socket.h> // Pour socket, bind, listen, etc.
#include <netinet/in.h> // Pour sockaddr_in
#include <arpa/inet.h>  // Pour inet_ntop
#include <sys/time.h>   // Pour timeval
#include <poll.h>       // Pour poll
#include <fcntl.h>      // Pour fcntl
#include <signal.h>     // Pour signal
#include <errno.h>      // Pour errno

// Inclusions du projet
#include "IrcCodes.hpp"

// Déclaration anticipée
class	Client;

class	Channel;

extern const Channel	g_nChan;

class	Server
{
	private:
		const unsigned short		_port;
		const std::string			_pw;
		int							_fd;
		std::vector<Client *>		_clients;
		static Server*				_instance;
		static bool					_running;
		std::vector<
			std::pair<
				const std::string,
				char (Server::*)(Client *const, const std::vector<std::string> &tokens)
			>
		>							_events;
		std::vector<Channel>		_channels;

		// Structure pour poll()
		std::vector<pollfd>			_pollfds;

		// Méthodes privées pour l'organisation interne
		void						setupPollFds(void);
		void						handlePollEvents(int activity);
		bool						checkSocketErrors(int activity);
		void						handleClientInput(size_t pollfdIndex);
		void						handleClientError(size_t pollfdIndex);
		void						initEvents(void);
		void						mall(const std::string &msg) const;
		std::string					formatMessage(const std::string &code, const std::string &target, const std::string &message) const;
		std::string					formatError(const std::string &code, const std::string &target, const std::string &message) const;
	private: /* -Events/Commands- */
		void						addEvent(const std::string &cmd, char (Server::*f)(Client *const, const std::vector<std::string> &tokens));
		char						pass(Client *const, const std::vector<std::string> &tokens);
		char						nick(Client *const, const std::vector<std::string> &tokens);
		char						user(Client *const, const std::vector<std::string> &tokens);
		char						ping(Client *const, const std::vector<std::string> &tokens);
		char						quit(Client *const, const std::vector<std::string> &tokens);
		char						debug(Client *const, const std::vector<std::string> &tokens);

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
		const std::vector<Channel>	&getChannels(void) const { return (_channels); };
	public: /* -Methods- */
		Server						&operator+=(Client *const cl);
		Server						&operator-=(Client *const cl);
		Server						&operator+=(const Channel &ch);
		Server						&operator-=(const Channel &ch);

		// Méthodes d'initialisation
		bool						initServer(void);
		bool						initSocketOptions(void);
		bool						bindAndListen(void);

		// Méthode principale
		void						run(void);

		// Gestion des clients
		void						handleNewConnection(void);
		bool						configureClientSocket(int client_fd);
		Client*						createClient(int client_fd, struct sockaddr_in &client_addr);
		int							handleClientMessage(Client *client);
		void						disconnectClient(Client *client, const std::string &reason);

		// Gestion des cannaux
		char						createChannel(const std::string &name, const std::string &key);
		const Channel				&getChannel(const std::string &name) const;

		// Gestion des signaux
		static void					signalHandler(int sig);

		// Setter de l'instance
		static void					setInstance(Server* srv) { _instance = srv; }
};

const std::vector<std::string>	parseIrcMessage(const std::string &message);
