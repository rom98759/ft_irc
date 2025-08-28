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
#include "Client.hpp"

const ErrorFdException	Client::EFE;

Client::Client(const int &fd) : _fd(fd)
{
	if (_fd < 0)
		throw (Client::EFE);
}

Client::Client(const Client &cpy) : _fd(cpy._fd)
{
	_nick = cpy._nick;
}

Client	&Client::operator=(const Client &cpy)
{
	_nick = cpy._nick;
	return (*this);
}

Client::~Client(void)
{
	//close(_fd); For the moment tests are done without real fds so closing them is an issue
}
