/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 13:27:45 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/15 13:27:45 by kzhen-cl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <vector>

class	Client;

class	Channel
{
	private:
		std::string			_name;
		std::string			_key;
		int					_clientsLimit;
		std::vector<
			std::pair<
				Client *,
				std::string
			>
		>					_list;
	public:
		Channel(const std::string &name, const std::string &key);
		Channel(void) {};
		void				mall(const std::string &message, Client *except = NULL) const;
	public: /* -Getters- */
		const std::string	getName(void) const { return (_name); };
		const int			&getClientsLimit(void) const { return (_clientsLimit); };
		const std::string	&getKey(void) const { return (_key); };
		const std::vector<
			std::pair<
				Client *,
				std::string
			>
		>					&getList(void) const { return (_list); };
		char				isFull(void) const { return ((int)_list.size() == _clientsLimit); };
	public: /* -Setters- */
		void				setName(const std::string &newName) { _name = newName; };
		void				setClientsLimit(const int &i) { _clientsLimit = i; };
		void				setKey(const std::string &newKey) { _key = newKey; };
	public: /* -Operators- */
		Channel				&operator+=(Client *const);
		Channel				&operator-=(Client *const);
};
