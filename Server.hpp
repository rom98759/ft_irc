/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/26 15:30:15 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/26 15:30:15 by kzhen-cl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <unistd.h>
#include <vector>

class	Client;

typedef struct s_srv_set
{
	unsigned short			_port;
	std::string				_pw;
	int						_fd;
	std::vector<Client *>	*_clients;
}	t_srv_set;

class	Server
{
	private:
		const unsigned short		_port;
		const std::string			_pw;
		const int					_fd;
		std::vector<Client *>		*_clients;

	public: /* -CDstructors- */
		Server(void) : _port(), _pw(), _fd() {};
		~Server(void);
	public: /* -Getters- */
		const unsigned short		&getPort(void) const { return (_port); };
		const std::string			&getPw(void) const { return (_pw); };
		const int					&getFd(void) const { return (_fd); };
		const std::vector<Client *>	&getClients(void) const { return (*_clients); };
	public: /* -Methods- */
		Server						&operator+=(Client *const cl);
		Server						&operator-=(Client *const cl);
		unsigned char				initVector(void);
};
