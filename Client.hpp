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

#include "Channel.hpp"

// Inclusions de la bibliothèque standard
#include <string>
#include <iostream>
#include <exception>  // Pour std::exception

// Inclusions système
#include <unistd.h>  // Pour close

#define CHPERCL 16

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
		std::string						_username;
		std::string						_realname;
		std::string						_buffer;     // Buffer stocker données reçues
		unsigned char					_registerLevel : 2; // Etat d'enregistrement du client
		Channel							_channels[CHPERCL];

	public: /* -CDstructors- */
		Client(const int &fd);
		~Client(void);
	public: /* -Getters- */
		const int						&getFd(void) const { return (_fd); };
		const std::string				&getNick(void) const { return (_nick); };
		const std::string				&getUsername(void) const { return (_username); };
		const std::string				&getRealname(void) const { return (_realname); };
		const std::string				&getBuffer(void) const { return (_buffer); };
		bool							isRegistered(void) const { return (!(_registerLevel ^ 0b11)); };
		unsigned char					getRegisterLevel(void) const { return (_registerLevel); };
		const Channel					*getChannels(void) const { return (_channels); };
	public: /* -Setters- */
		void							setNick(const std::string &nick) { _nick = nick; };
		void							setUsername(const std::string &username) { _username = username; };
		void							setRealname(const std::string &realname) { _realname = realname; };
		void							upRegisterLevel(void) { ++_registerLevel; };
	public: /* -Operators- */
		char							operator+=(const Channel &ch);
		char							operator-=(const Channel &ch);
	public: /* -Methods- */
		bool							readFromSocket(void); // Lire des données depuis le socket
		void							appendToBuffer(const std::string &data); // Ajouter des données au buffer
		void							clearBuffer(void) { _buffer.clear(); }; // Vider le buffer
		bool							sendMessage(const std::string &message); // Envoyer un message au client
	public: /* -Exceptions- */
		static const ErrorFdException	EFE;
};
