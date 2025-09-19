/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 15:44:06 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/19 13:05:10 by rcaillie         ###   ########.fr       */
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
	_topic = "";
	_modes = "tl";  // Modes par défaut : +t (topic restricted) et +l (user limit)
	_inviteOnly = false;
	_topicRestricted = true;
	_hasUserLimit = true; // default 10
	_hasKey = !key.empty();
}

Channel	&Channel::operator+=(Client *const cl)
{
	if (!_list.size())
		_list.push_back(std::pair<Client *, std::string>(cl, "~+"));
	else
		_list.push_back(std::pair<Client *, std::string>(cl, "+"));
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

bool Channel::isUserInChannel(Client *user) const
{
	for (size_t i = 0; i < _list.size(); ++i)
	{
		if (_list[i].first == user)
			return true;
	}
	return false;
}

bool Channel::isUserOperator(Client *user) const
{
	for (size_t i = 0; i < _list.size(); ++i)
	{
		if (_list[i].first == user)
			return (_list[i].second.find('~') != std::string::npos);
	}
	return false;
}
