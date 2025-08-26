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

typedef struct s_srv_set
{
	unsigned short	_port;
	std::string		_pw;
}	t_srv_set;

class	Server
{
	private:
		const unsigned short	_port;
		const std::string		_pw;
	public: /* -CDstructors- */
		Server(void) : _port(), _pw() {};
		~Server(void);
	public: /* -Getters- */
		const unsigned short	&getPort(void) const { return (_port); };
		const std::string		&getPw(void) const { return (_pw); };
};
