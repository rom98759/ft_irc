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
#include <iostream>
#include <unistd.h>

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
		std::string						_buffer;     // Buffer pour stocker les données reçues
		bool							_registered; // État d'enregistrement du client

	public: /* -CDstructors- */
		Client(const int &fd);
		Client(const Client &cpy);
		Client	&operator=(const Client &cpy);
		~Client(void);
	public: /* -Setters- */
		void							setNick(const std::string &nick) { _nick = nick; };
		void							setRegistered(bool status) { _registered = status; };
	public: /* -Getters- */
		const int						&getFd(void) const { return (_fd); };
		const std::string				&getNick(void) const { return (_nick); };
		const std::string				&getBuffer(void) const { return (_buffer); };
		bool							isRegistered(void) const { return (_registered); };
	public: /* -Methods- */
		bool							readFromSocket(void); // Lire des données depuis le socket
		void							appendToBuffer(const std::string &data); // Ajouter des données au buffer
		void							clearBuffer(void) { _buffer.clear(); }; // Vider le buffer
		bool							sendMessage(const std::string &message); // Envoyer un message au client
	public: /* -Exceptions- */
		static const ErrorFdException	EFE;
};
