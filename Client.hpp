/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/28 10:43:45 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/28 10:43:45 by kzhen-cl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>

class	ErrorFdException : public std::exception
{
	public:
		const char	*what(void) const throw()
		{
			return ("Error Fd !");
		}
};

class	Client
{
	private:
		const int						_fd;
		std::string						_nick;

	public: /* -CDstructors- */
		Client(const int &fd);
		Client(const Client &cpy);
		Client	&operator=(const Client &cpy);
		~Client(void);
	public: /* -Setters- */
		void							setNick(const std::string &nick) { _nick = nick; };
	public: /* -Getters- */
		const int						&getFd(void) const { return (_fd); };
		const std::string				&getNick(void) const { return (_nick); };
	public: /* -Exceptions- */
		static const ErrorFdException	EFE;
};
