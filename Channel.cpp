/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 15:44:06 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/22 10:13:53 by rcaillie         ###   ########.fr       */
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
	_inviteOnly = false;
	_topicRestricted = true;
	_hasUserLimit = true; // default 10
	_hasKey = !key.empty();
}

Channel	&Channel::operator+=(Client *const cl)
{
	if (!_list.size())
		_list.push_back(std::pair<Client *, std::string>(cl, "@+"));
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
	if (_list.empty())
		*Server::getInstance() -= this;
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
			return (_list[i].second.find('@') != std::string::npos);
	}
	return false;
}

void Channel::addInvitation(Client *user)
{
	for (size_t i = 0; i < _invitations.size(); ++i)
	{
		if (_invitations[i] == user)
			return;
	}
	_invitations.push_back(user);
}

void	Channel::deleteInvitation(Client *user)
{
	std::size_t isize = _invitations.size();
	for (std::size_t i = 0; i < isize; ++i)
	{
		if (_invitations[i] == user)
		{
			_invitations.erase(_invitations.begin() + i);
			break ;
		}
	}
}

void	Channel::addOperator(Client *user)
{
	for (size_t i = 0; i < _list.size(); ++i)
	{
		if (_list[i].first == user)
		{
			_list[i].second += '@';
			break ;
		}
	}
}

void	Channel::removeOperator(Client *user)
{
	for (size_t i = 0; i < _list.size(); ++i)
	{
		if (_list[i].first == user)
		{
			size_t pos = _list[i].second.find('@');
			std::cout << pos << std::endl;
			if (pos != std::string::npos)
				_list[i].second.erase(pos, 1);
			break ;
		}
	}
}
