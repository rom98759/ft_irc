/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Unibot.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 00:30:28 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/24 10:22:04 by rcaillie         ###   ########.fr       */
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
	std::string message(buffer);
	logMessage("Received: " + message, false);

	// Here you can parse the message and respond accordingly
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

void	Unibot::run()
{
	if (!connectServer())
	{
		logMessage("Failed to connect to server", true);
		return;
	}

	logMessage("[DEBUG] Starting login sequence", false);
	// if (!login())
	// {
	// 	logMessage("Login failed", true);
	// 	disconnect();
	// 	return;
	// }

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
			logMessage("Poll timeout, no data received", false);
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
