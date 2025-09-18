/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 15:44:06 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/18 19:30:42 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "IrcCodes.hpp"

Channel::Channel(const std::string &name, const std::string &key)
{
	_name = name;
	_key = key;
	_clientsLimit = 10;
}

Channel	&Channel::operator+=(Client *const cl)
{
	if (!_list.size())
		_list.push_back(std::pair<Client *, std::string>(cl, "~+"));
	else
		_list.push_back(std::pair<Client *, std::string>(cl, "+"));
	std::string joinMsg = ":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 JOIN :" + _name + "\r\n";
	mall(joinMsg, cl);
	return (*this);
}

Channel	&Channel::operator-=(Client *const cl)
{
	std::size_t	lsize = _list.size();
	for (std::size_t i = 0; i < lsize; ++i)
	{
		if (_list[i].first == cl)
		{
			_list.erase(_list.begin() + i);
			std::string partMsg = ":" + cl->getNick() + "!" + cl->getUsername() + "@127.0.0.1 PART " + _name;
			if (!cl->reason.empty())
				partMsg += " :" + cl->reason;
			partMsg += "\r\n";
			mall(partMsg);
			break ;
		}
	}
	return (*this);
}

void	Channel::mall(const std::string &message, Client *except) const
{
	std::size_t	lsize = _list.size();
	for (std::size_t i = 0; i < lsize; ++i)
		if (_list[i].first != except)
			_list[i].first->sendMessage(message);
}
