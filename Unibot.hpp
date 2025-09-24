/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Unibot.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 15:54:06 by rcaillie          #+#    #+#             */
/*   Updated: 2025/09/23 15:54:06 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sstream>
#include <queue>
#include <signal.h>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>

class Unibot
{
	private:
		static Unibot* _instance;      // Instance pour le signal handler

		std::string _password;
		int _port;
		int _fd;                      // socket
		struct sockaddr_in _serv_addr;
		bool _running;

		std::queue<std::string> _incomingMessages; // queue des messages reçus
		std::queue<std::string> _outgoingMessages; // queue des messages à envoyer

	public:
		Unibot(const std::string &password, int port);
		~Unibot();

		bool initSocket();            // créer socket, option non-bloquant
		bool setupConnection();       // configurer la connexion
		bool connectServer();         // connecter avec gestion EINPROGRESS
		void disconnect();            // fermer socket proprement
		void run();                   // boucle principale unique avec poll

		std::string getLastMessage(); // obtenir le message le plus ancien

		void logMessage(const std::string &msg, bool isError) const;


	private:
		void handleIncomingMessages();    // lire messages, push dans _incomingMessages
		void flushOutgoingMessages();     // envoyer messages depuis _outgoingMessages
		void sendMessage(const std::string &msg); // push dans _outgoingMessages
		bool login();                     // sequence de login IRC
		bool joinGameChannel();          // rejoindre le channel #GAME
};

int	getNumericResponse(const std::string& message); // extraire le code numérique d'une réponse IRC
static void SignalHandler(int signum);