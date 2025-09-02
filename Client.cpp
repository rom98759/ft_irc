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

#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include "Client.hpp"

const ErrorFdException	Client::EFE;

Client::Client(const int &fd) : _fd(fd), _registered(false)
{
	if (_fd < 0)
		throw (Client::EFE);
}

Client::Client(const Client &cpy) : _fd(cpy._fd), _registered(cpy._registered)
{
	_nick = cpy._nick;
	_buffer = cpy._buffer;
}

Client	&Client::operator=(const Client &cpy)
{
	_nick = cpy._nick;
	_buffer = cpy._buffer;
	_registered = cpy._registered;
	return (*this);
}

Client::~Client(void)
{
	close(_fd);
}

bool Client::readFromSocket(void)
{
	char buffer[1024];
	ssize_t bytesRead;

	memset(buffer, 0, sizeof(buffer));
	bytesRead = recv(_fd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead > 0)
	{
		buffer[bytesRead] = '\0'; // Assurer que la chaîne est bien terminée
		appendToBuffer(buffer);
		std::cout << "Reçu du client fd=" << _fd << ": " << buffer << std::endl;
		return true;
	}
	else if (bytesRead == 0)
	{
		std::cout << "Client fd=" << _fd << " déconnecté." << std::endl;
		return false; // Connexion fermée par le client
	}
	else
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "Erreur lors de la lecture du client fd=" << _fd << ": ";
			std::cerr << strerror(errno) << std::endl;
			return false;
		}
		return true; // Pas de données disponibles pour le moment
	}
}

void Client::appendToBuffer(const std::string &data)
{
	_buffer += data;
}

bool Client::sendMessage(const std::string &message)
{
	ssize_t bytesSent = send(_fd, message.c_str(), message.length(), 0);

	if (bytesSent < 0)
	{
		std::cerr << "Erreur lors de l'envoi au client fd=" << _fd << ": ";
		std::cerr << strerror(errno) << std::endl;
		return false;
	}
	else if (static_cast<size_t>(bytesSent) < message.length())
	{
		std::cerr << "Envoi partiel au client fd=" << _fd << std::endl;
		// Idéalement, on gérerait ici l'envoi partiel en mettant en file d'attente le reste
		return false;
	}

	return true;
}
