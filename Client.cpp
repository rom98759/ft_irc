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
	char buffer[512];  // RFC 1459: Max 512 octets incluant CR-LF
	ssize_t bytesRead;

	if (_buffer.length() > 512)
	{
		std::cerr << "Buffer overflow attempt from client " << _fd << std::endl;
		return false;
	}

	memset(buffer, 0, sizeof(buffer));
	bytesRead = recv(_fd, buffer, sizeof(buffer) - 2, 0);  // -2 pour CR-LF

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
		std::cerr << "Erreur de lecture du client fd=" << _fd << std::endl;
		return false;
	}
}

void Client::appendToBuffer(const std::string &data)
{
	_buffer += data;
}

bool Client::sendMessage(const std::string &message)
{
	if (message.empty())
		return true;
	// if (message.length() > 510) // 512 - 2 (\r\n)
	// {
	// 	std::cout << "Message trop long du client fd=" << _fd << " (" << message.length() << " octets), tronqué" << std::endl;
	// 	message = message.substr(0, 510);
	// }
	ssize_t bytesSent = send(_fd, message.c_str(), message.length(), 0);

	// Erreur d'envoi
	if (bytesSent < 0)
	{
		std::cerr << "Erreur lors de l'envoi au client fd=" << _fd << std::endl;
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
