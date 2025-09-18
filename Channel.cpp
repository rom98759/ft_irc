/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 15:44:06 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/15 15:44:06 by kzhen-cl         ###   ########.fr       */
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
	mall(formatMessage("NEWCOMER", _name, cl->getNick() + " joined."), cl);
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
			mall(formatMessage("DEPARTURE", _name, cl->getNick() + " left." + (cl->reason.empty() ? "" : (" (" + cl->reason + ")"))));
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
