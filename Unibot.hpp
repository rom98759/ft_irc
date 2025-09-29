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

class	Unibot
{
	private: /* -Tic Tac Toe- */
		class	X3T // x3 T => TTT => Tic Tac Toe
		{
			private:
				char						_table[9];
			public: /* -Constructor- */
				X3T(void);
			public: /* -Getters/Predicate Methods- */
				std::vector<std::string>	getFormattedTable(void) const;
				char						isWin(void) const;
				char						isFull(void) const;
				char						isPlayable(const char &index) const;
			public: /* -Setters- */
				void						play(const char &sign, const char &index);
				void						fillTable(const char &sign);
		};
		X3T						_x3t[10];
		void					reportX3T(void);

	private:
		static const int		_timeout = 5000;
		static Unibot*			_instance;                                                       // Instance pour le signal handler

		std::string				_prefix;
		std::string				_password;
		int						_port;
		std::string				_currentChannel;
		std::string				_key;
		int						_fd;                                                             // socket
		struct sockaddr_in		_serv_addr;
		bool					_running;
		std::string				_nick;

		std::queue<std::string> _incomingMessages;                                               // queue des messages reçus
		std::queue<std::string> _outgoingMessages;                                               // queue des messages à envoyer

		std::string				_p[2];
		static const char		_sign[2];
		unsigned char			_game : 1;
		unsigned char			_gm : 2;
		unsigned char			_turn : 1;
		unsigned char			_forcedMove : 4;
		long					_inviteStart;

	public:
		Unibot(const std::string &password, int port, const std::string &channel, const std::string &key);
		~Unibot();

		bool					initSocket();                                                    // créer socket, option non-bloquant
		bool					setupConnection();                                               // configurer la connexion
		bool					connectServer();                                                 // connecter avec gestion EINPROGRESS
		void					disconnect();                                                    // fermer socket proprement
		void					run();                                                           // boucle principale unique avec poll

		std::string				getLastMessage();                                                // obtenir le message le plus ancien

		void					logMessage(const std::string &msg, bool isError) const;


	private:
		void					handleIncomingMessages();                                        // lire messages, push dans _incomingMessages
		void					flushOutgoingMessages();                                         // envoyer messages depuis _outgoingMessages
		char					handleEvents(const std::string &message);                        // gérer les événements (KICK, Client PART/NICK etc.);
		void					handleCommands(const std::string &message);                      // gérer les commandes spécifiques
		void					sendMessage(const std::string &msg);                             // push dans _outgoingMessages
		void					sendToChannel(const std::string &msg);                           // formatte pour envoyer directement dans le canal
		void					sendToClient(const std::string &client, const std::string &msg); // formatte pour envoyer directement à un utilisateur
		bool					login();                                                         // sequence de login IRC
		bool					joinChannel(const std::string &channel);                         // rejoindre le channel #GAME

		void					clearIncomingMessages() { while (!_incomingMessages.empty()) _incomingMessages.pop(); }
		char					recvUntilTimeoutOrError(int rpl, const std::vector<int> &err = std::vector<int>());

		void					resetPlayers(void);
		void					resetGame(void);
		void					resetInvite(void);

		void					playGameBasic(const std::string &client, const std::vector<std::string> &tokens);
		void					playGameBidimensional(const std::string &client, const std::vector<std::string> &tokens);
		void					inviteGame(const std::string &client, const std::vector<std::string> &tokens);
		void					replyGame(const std::string &client, const std::vector<std::string> &tokens);
};

int			getNumericResponse(const std::string& message); // extraire le code numérique d'une réponse IRC
static void	SignalHandler(int signum);
