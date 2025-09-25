/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Unibot.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/23 00:30:28 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/24 13:12:50 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Unibot.hpp"
#include <sys/time.h>
#include <sstream>

Unibot::Unibot(const std::string &password, int port, const std::string &channel, const std::string &key)
	: _prefix("UB "), _password(password), _port(port), _currentChannel(channel), _key(key), _fd(-1), _running(true) {}

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
		if (errno == 11)
			return ;
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
		if (!line.empty() && line[line.length() - 1] == '\r')
			line.erase(line.length() - 1);
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

void	Unibot::sendToChannel(const std::string &msg)
{
	sendMessage("PRIVMSG " + _currentChannel + " :" + msg);
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

	bool loggedIn = false;
	int maxAttempts = 10;
	int attempts = 0;

	while (!loggedIn && _running && attempts < maxAttempts)
	{
		attempts++;
		struct pollfd fds[1];
		fds[0].fd = _fd;
		fds[0].events = POLLIN;
		int ret = poll(fds, 1, _timeout); // 5 second timeout
		if (ret <= 0)
			continue;
		if (fds[0].revents & POLLIN)
		{
			handleIncomingMessages();
			while (!_incomingMessages.empty())
			{
				std::string response = getLastMessage();
				int numeric = getNumericResponse(response);
				if (numeric == 464)
				{ // ERR_PASSWDMISMATCH
					logMessage("Password incorrect", true);
					_running = false;
					return false;
				}
				else if (numeric == 004)
				{ // RPL_WELCOME
					loggedIn = true;
					break;
				}
				else if (numeric == 433)
				{ // ERR_NICKNAMEINUSE
					logMessage("Nickname already in use, trying another one", false);
					nickAttempt++;
					oss.str("");
					oss << "unibot" << nickAttempt;
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

static inline long	now(void)
{
	struct timeval	tv;

	gettimeofday(&tv, 0);
	return ((tv.tv_sec * 1000L) + (tv.tv_usec / 1000L));
}

static char	contains(const std::vector<int> &err, int num)
{
	std::size_t	esize = err.size();
	for (std::size_t i = 0; i < esize; ++i)
		if (err[i] == num)
			return (1);
	return (0);
}

/*
Use only after having sent message to get determine it was a success or not.
	- if _timeout is reached, returns 0
	- if err is found, returns 0
	- if rpl is found, returns 1
*/
char	Unibot::recvUntilTimeoutOrError(int rpl, const std::vector<int> &err)
{
	const long			begin = now();

	while (now() - begin < _timeout)
	{
		handleIncomingMessages();
		if (!_incomingMessages.empty())
		{
			std::string	response = getLastMessage();
			int			responseNum = getNumericResponse(response);
			if (responseNum == rpl)
				return (1);
			if (contains(err, responseNum))
				return (0);
		}
		usleep(100);
	}
	return (0);
}

bool	Unibot::joinChannel(const std::string &channel)
{
	sendMessage("JOIN " + channel + ' ' + _key);
	flushOutgoingMessages();

	struct pollfd			fds[1];

	(*fds).fd = _fd;
	(*fds).events = POLLIN;
	const int				ret = poll(fds, 1, _timeout); // 5 seconds timeout
	if (ret > 0 && ((*fds).revents & POLLIN))
	{
		static const int	errArray[5] = {403, 475, 471, 473, 476};
		if (recvUntilTimeoutOrError(366, std::vector<int>(errArray, errArray + 5)))
		{
			logMessage("Successfully joined channel " + channel, false);
			_currentChannel = channel;
			return (1);
		}
	}

	logMessage("Failed to join " + channel, true);
	return false;
}

static inline const std::vector<std::string>	ft_splitSpaces(const std::string &cmd)
{
	std::vector<std::string>	tokens;
	std::istringstream			iss(cmd);
	std::string					token;

	while (iss >> token)
		tokens.push_back(token);
	return (tokens);
}

void	Unibot::handleCommands(const std::string &message)
{
	std::cout << "Handling message: " << message << std::endl;
	const std::string			target = " PRIVMSG " + _currentChannel + " :" + _prefix;
	const std::size_t			pos = message.find(target);
	if (pos == std::string::npos)
		return ;
	const std::string			cmd = message.substr(pos + target.size());
	if (std::isspace(cmd[0]))
		return ;
	std::cout << "Command found: " << cmd << std::endl;

	std::vector<std::string>	tokens = ft_splitSpaces(cmd);
	if (tokens[0] == "prefix")
	{
		if (tokens.size() != 2)
		{
			sendToChannel("<current_prefix>prefix: 1 parameter: <new_prefix>");
			return ;
		}
		tokens[1] = cmd.substr(cmd.rfind(tokens[1]));
		_prefix = tokens[1];
		sendToChannel("prefix changed: [" + tokens[1] + "]");
	}
	else if (tokens[0] == "channel")
	{
		std::size_t	tsize = tokens.size();
		if (tsize < 2 || tsize > 3)
		{
			sendToChannel("<prefix>channel: 1-2 parameters: <channel> [<key>]");
			return ;
		}
		if (tokens[1] == _currentChannel)
		{
			sendToChannel("<prefix>channel: can't change to <current_channel>");
			return ;
		}
		_key = (tsize == 3 ? tokens[2] : "");
		std::string	oldChannel = _currentChannel;
		if (joinChannel(tokens[1]))
			sendMessage("PART " + oldChannel + " :channel changed: [" + _currentChannel + (tsize == 3 ? ("](" + tokens[2] + ")") : "]"));
		else
			sendToChannel("<prefix>channel: can't join " + tokens[1]);
	}
}

void	Unibot::run()
{
	if (!connectServer())
	{
		logMessage("Failed to connect to server", true);
		return;
	}
	if (!login())
	{
		logMessage("Login failed", true);
		disconnect();
		return;
	}
	fcntl(_fd, F_SETFL, O_NONBLOCK);
	if (!joinChannel(_currentChannel))
	{
		disconnect();
		return;
	}
	clearIncomingMessages();
	while (_running)
	{
		struct pollfd fds[1];
		fds[0].fd = _fd;
		fds[0].events = POLLIN;
		int ret = poll(fds, 1, _timeout); // 5 seconds timeout
		if (ret < 0)
		{
			logMessage("Poll error: " + std::string(strerror(errno)), true);
			break;
		}
		else if (ret == 0)
		{
			// Timeout, no data received
			continue;
		}
		if (fds[0].revents & POLLIN)
		{
			handleIncomingMessages();
			// Traiter tous les messages entrants immédiatement
			while (!_incomingMessages.empty())
			{
				std::string msg = getLastMessage();
				handleCommands(msg);
			}
		}
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
	if (argc < 4 || argc > 5)
	{
		std::cerr << "Usage: ./Unibot <port> <password> <channel> [<key>]" << std::endl;
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
	Unibot bot(argv[2], port, *(argv + 3), (argc == 5 ? *(argv + 4) : ""));
	bot.run();

	return (0);
}
