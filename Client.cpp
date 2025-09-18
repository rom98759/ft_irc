/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 10:43:41 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/28 10:43:41 by kzhen-cl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

// Inclusions système supplémentaires nécessaires pour l'implémentation
#include <sys/socket.h>  // Pour recv, send
#include <cstring>       // Pour memset, strerror
#include <errno.h>       // Pour errno

const ErrorFdException	Client::EFE;

Client::Client(const int &fd) : _fd(fd), _registerLevel(0)
{
	if (_fd < 0)
		throw (Client::EFE);
	for (int i = 0; i < CHPERCL; ++i)
		*(_channels + i) = NULL;
}

Client::~Client(void)
{
	close(_fd);
}

char	Client::operator+=(Channel *const ch)
{
	int	i = -1;
	while (++i < CHPERCL)
	{
		if (*(_channels + i) == ch)
			return (0);
		if (!*(_channels + i))
			break;
	}
	*(_channels + i) = ch;
	return (1);
}

char	Client::operator-=(Channel *const ch)
{
	for (int i = 0; i < CHPERCL; ++i)
	{
		if (*(_channels + i) == ch)
		{
			*(_channels + i) = NULL;
			return (1);
		}
	}
	return (0);
}

bool Client::readFromSocket(void)
{
	char buffer[1024];
	ssize_t bytesRead;

	memset(buffer, 0, sizeof(buffer));
	bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, 0);

	// Lu
	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0';
		appendToBuffer(buffer);
		return true;
	}
	// Client disconnected
	else if (bytesRead == 0)
	{
		std::cout << "Client fd=" << _fd << " déconnecté." << std::endl;
		return false;
	}
	else
	{
		// Erreur lecture
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "Erreur lors de la lecture du client fd=" << _fd << ": ";
			std::cerr << strerror(errno) << std::endl;
			return false;
		}
		return true;
	}
}

void Client::appendToBuffer(const std::string &data)
{
	_buffer += data;
}

bool Client::sendMessage(const std::string &message)
{
	ssize_t bytesSent = send(_fd, message.c_str(), message.length(), 0);

	// Erreur d'envoi
	if (bytesSent < 0)
	{
		std::cerr << "Erreur lors de l'envoi au client fd=" << _fd << ": ";
		std::cerr << strerror(errno) << std::endl;
		return false;
	}
	else if (static_cast<size_t>(bytesSent) < message.length())
	{
		std::cerr << "Envoi partiel au client fd=" << _fd << std::endl;
		// Envoi en attente
		// TODO: Implement partial send handling
		// Mettre le reste du message dans une file d'attente
		return false;
	}

	return true;
}

char	Client::cannotJoinNChannels(int n) const
{
	if (n > CHPERCL)
		return (1);
	int	i = -1;
	while (++i < CHPERCL)
		if (*(_channels + i) == NULL)
			break ;
	return (n > (CHPERCL - i));
}

std::string	Client::getSheet(void) const
{
	return ((std::string)"+--- USER INFO ---+\n| ["
		+ getNick() + "] \n"
		+ "|\t- Real Name : " + getRealname() + "\n"
		+ "|\t- Username : " + getUsername() + "\n"
		+ "+--- USER INFO ---+\n");
}
