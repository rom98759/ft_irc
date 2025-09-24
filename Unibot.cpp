/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Unibot.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 00:30:28 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/24 12:09:25 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Unibot.hpp"

Unibot::Unibot(const std::string &password, int port)
	: _password(password), _port(port), _fd(-1), _running(true)
{
}

Unibot::~Unibot()
{
	if (_fd >= 0)
		close(_fd);
	std::cout << "Unibot Shutdown !" << std::endl;
}

void	Unibot::logMessage(const std::string &msg, bool isError) const
{
	if (isError)
		std::cerr << "Error: " << msg << std::endl;
	else
		std::cout << msg << std::endl;
}

// void	Unibot::parseIRCMessage(const std::string& message)
// {
// 	size_t pos = 0;
// 	std::string msg = message;

// 	// Skip prefix if present
// 	if (msg[0] == ':')
// 	{
// 		pos = msg.find(' ');
// 		if (pos == std::string::npos)
// 			return;
// 		msg = msg.substr(pos + 1);
// 	}

// 	// Get command/numeric
// 	pos = msg.find(' ');
// 	std::string cmd = (pos == std::string::npos) ? msg : msg.substr(0, pos);

// 	logMessage("[PARSED] Command: " + cmd, false);
// 	_incomingMessages.push(message);
// }

int	getNumericResponse(const std::string& message)
{
	size_t pos = message.find(' ');
	if (pos == std::string::npos || message[0] != ':')
		return -1;

	std::string numeric = message.substr(pos + 1);
	pos = numeric.find(' ');
	if (pos == std::string::npos)
		return -1;

	numeric = numeric.substr(0, pos);
	return std::atoi(numeric.c_str());
}

void	Unibot::disconnect()
{
	if (_fd >= 0)
	{
		close(_fd);
		_fd = -1;
		logMessage("Disconnected from server", false);
	}
}

bool	Unibot::initSocket()
{
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
	{
		logMessage("Socket creation failed: " + std::string(strerror(errno)), true);
		return false;
	}

	return true;
}

bool	Unibot::setupConnection()
{
	_serv_addr.sin_family = AF_INET;
	_serv_addr.sin_port = htons(_port);
	inet_pton(AF_INET, "127.0.0.1", &_serv_addr.sin_addr);

	if (connect(_fd, (struct sockaddr *)&_serv_addr, sizeof(_serv_addr)) < 0)
	{
		logMessage("Connection failed: " + std::string(strerror(errno)), true);
		disconnect();
		return false;
	}
	return true;
}

bool	Unibot::connectServer()
{
	std::ostringstream oss;
	oss << "Connecting to server on port " << _port;
	logMessage(oss.str(), false);

	if (!initSocket() || !setupConnection())
		return false;

	logMessage("Connected to server", false);
	return true;
}

std::string	Unibot::getLastMessage()
{
	if (_incomingMessages.empty())
		return "";
	std::string msg = _incomingMessages.front();
	_incomingMessages.pop();
	return msg;
}

void	Unibot::handleIncomingMessages()
{
	char buffer[512];
	ssize_t bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytesRead < 0)
	{
		logMessage("Receive error: " + std::string(strerror(errno)), true);
		_running = false;
		return;
	}
	else if (bytesRead == 0)
	{
		logMessage("Server closed the connection", false);
		_running = false;
		return;
	}

	buffer[bytesRead] = '\0';
	std::string received(buffer);
	std::istringstream stream(received);
	std::string line;

	// Split the received data into lines
	while (std::getline(stream, line, '\n'))
	{
		// Remove \r if present
		if (!line.empty() && line[line.length()-1] == '\r')
			line.erase(line.length()-1);

		if (!line.empty())
		{
			logMessage("Received: " + line, false);
			_incomingMessages.push(line);
		}
	}
}

void	Unibot::sendMessage(const std::string &msg)
{
	_outgoingMessages.push(msg + "\r\n");
}

void	Unibot::flushOutgoingMessages()
{
	while (!_outgoingMessages.empty())
	{
		const std::string &msg = _outgoingMessages.front();
		ssize_t bytesSent = send(_fd, msg.c_str(), msg.length(), MSG_NOSIGNAL);
		if (bytesSent < 0)
		{
			logMessage("Send error: " + std::string(strerror(errno)), true);
			_running = false;
			return;
		}
		logMessage("Sent: " + msg, false);
		_outgoingMessages.pop();
	}
}

bool	Unibot::login()
{
	static int nickAttempt = 0;
	std::ostringstream oss;
	oss << "unibot";
	if (nickAttempt > 0)
		oss << nickAttempt;
	std::string nickname = oss.str();

	// Envoi du PASS
	sendMessage("PASS " + _password);

	// Envoi du NICK
	sendMessage("NICK " + nickname);

	// Envoi du USER
	sendMessage("USER unibot 0 * :Unibot IRC");

	// Flush immédiatement
	flushOutgoingMessages();

	std::string response;
	bool loggedIn = false;
	int maxAttempts = 10;
	int attempts = 0;

	// Attente de la réponse du serveur avec timeout
	while (!loggedIn && _running && attempts < maxAttempts)
	{
		attempts++;
		struct pollfd fds[1];
		fds[0].fd = _fd;
		fds[0].events = POLLIN;

		int ret = poll(fds, 1, 1000); // 1 second timeout
		if (ret <= 0) continue;

		if (fds[0].revents & POLLIN)
		{
			handleIncomingMessages();
			while (!_incomingMessages.empty())
			{
				response = getLastMessage();
				int numeric = getNumericResponse(response);

				if (numeric == 001) // RPL_WELCOME
				{
					loggedIn = true;
					break;
				}
				else if (numeric == 433) // ERR_NICKNAMEINUSE
				{
					logMessage("Nickname already in use, trying another one", false);
					nickAttempt++;
					oss.str("");
					oss << "unibot";
					if (nickAttempt > 0)
						oss << nickAttempt;
					nickname = oss.str();
					sendMessage("NICK " + nickname);
					flushOutgoingMessages();
				}
			}
		}
	}

	if (!loggedIn)
	{
		logMessage("Login failed after multiple attempts", true);
		return false;
	}

	logMessage("Login successful with nickname: " + nickname, false);
	return true;
}

bool	Unibot::joinGameChannel()
{
	bool joined = false;
	int maxAttempts = 10;
	int attempts = 0;

	sendMessage("JOIN #GAME");
	flushOutgoingMessages();


	// Attente de la réponse du serveur
	std::string response;
	handleIncomingMessages();
	while (_running && attempts < maxAttempts)
	{
		attempts++;
		response = getLastMessage();
		if (!response.empty())
		{
			logMessage("Received: " + response, false);
			if (response.find("JOIN :#GAME") != std::string::npos)
			{
				logMessage("Successfully joined channel #GAME", false);
				joined = true;
				break;
			}
		}
	}

	logMessage("Join #GAME " + std::string(joined ? "succeeded" : "failed"), !joined);
	return joined;
}

void	Unibot::run()
{
	if (!connectServer())
	{
		logMessage("Failed to connect to server", true);
		return;
	}

	logMessage("[DEBUG] Starting login sequence", false);
	if (!login())
	{
		logMessage("Login failed", true);
		disconnect();
		return;
	}
	if (!joinGameChannel())
	{
		logMessage("Failed to join #GAME channel", true);
		disconnect();
		return;
	}

	while (_running)
	{
		struct pollfd fds[1];
		fds[0].fd = _fd;
		fds[0].events = POLLIN;

		int ret = poll(fds, 1, 5000); // 5 seconds timeout
		if (ret < 0)
		{
			logMessage("Poll error: " + std::string(strerror(errno)), true);
			break;
		}
		else if (ret == 0)
		{
			// logMessage("Poll timeout, no data received", false);
			continue;
		}

		// General event handling
		if (fds[0].revents & POLLIN)
			handleIncomingMessages();

		//
		flushOutgoingMessages();
	}
	disconnect();
}

void	SignalHandler(int signum)
{
	std::cout << "Caught signal " << signum << ", shutting down..." << std::endl;
}

int	main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./Unibot <port> <password>" << std::endl;
		return (1);
	}
	int port = atoi(argv[1]);
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Error: Invalid port number." << std::endl;
		return (1);
	}

	signal(SIGINT, SignalHandler);
	signal(SIGTERM, SignalHandler);
	signal(SIGQUIT, SignalHandler);
	signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE crash send

	// Initialize and run the bot
	Unibot bot(argv[2], port);
	bot.run();

	return (0);
}
