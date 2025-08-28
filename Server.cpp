/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/26 15:30:10 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/26 15:30:10 by kzhen-cl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"

/*
Behaviour:
	1- Ensures every event is ended.
	2- Frees variables and close fds.
*/
Server::~Server(void)
{
	if (_clients)
	{
		for (int i = 0; i < (int)_clients->size(); ++i)
			delete ((*_clients)[i]);
		delete (_clients);
	}
	close(_fd);
}

Server	&Server::operator+=(Client *const cl)
{
	_clients->push_back(cl);
	return (*this);
}

Server	&Server::operator-=(Client *const cl)
{
	for (int i = 0; i < (int)_clients->size(); ++i)
	{
		if ((*_clients)[i] == cl)
		{
			delete (cl);
			_clients->erase(_clients->begin() + i);
		}
	}
	return (*this);
}

unsigned char	Server::initVector(void)
{
	try
	{
		_clients = new (std::vector<Client *>);
	}
	catch (const std::exception &e)
	{
		std::cout << "Error. " << e.what() << std::endl;
		return (0);
	}
	return (1);
}
