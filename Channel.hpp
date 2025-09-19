/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/15 13:27:45 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/19 16:05:26 by rcaillie         ###   ########.fr       */
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
		std::string			_topic;
		std::string			_modes;			// Modes du canal (+tnlk)
		bool				_topicRestricted;	// Mode +t
		bool				_hasUserLimit;		// Mode +l
		bool				_hasKey;			// Mode +k
		bool				_inviteOnly;		// Mode +i
		std::vector<
			std::pair<
				Client *,
				std::string
			>
		>					_list;
		std::vector<Client *>	_invitations;	// Liste des utilisateurs invités (pour mode +i)
	public:
		Channel(const std::string &name, const std::string &key);
		Channel(void) {};
		void				mall(const std::string &message, Client *except = NULL) const;
	public: /* -Getters- */
		const std::string	getName(void) const { return (_name); };
		const int			&getClientsLimit(void) const { return (_clientsLimit); };
		const std::string	&getKey(void) const { return (_key); };
		const std::string	&getTopic(void) const { return (_topic); };
		const std::string	&getModes(void) const { return (_modes); };
		bool				isTopicRestricted(void) const { return (_topicRestricted); };
		bool				hasUserLimit(void) const { return (_hasUserLimit); };
		bool				hasKey(void) const { return (_hasKey); };
		bool				isInviteOnly(void) const { return (_inviteOnly); };
		const std::vector<
			std::pair<
				Client *,
				std::string
			>
		>					&getList(void) const { return (_list); };
		char				isFull(void) const { return (_hasUserLimit && (int)_list.size() >= _clientsLimit); };
		const std::vector<Client *>& getInvitations(void) const { return (_invitations); };
	public: /* -Setters- */
		void				setName(const std::string &newName) { _name = newName; };
		void				setClientsLimit(const int &i) { _clientsLimit = i; };
		void				setKey(const std::string &newKey) { _key = newKey; _hasKey = !newKey.empty(); };
		void				setTopic(const std::string &newTopic) { _topic = newTopic; };
		void				setTopicRestricted(bool restricted) { _topicRestricted = restricted; };
		void				setUserLimit(int limit) { _clientsLimit = limit; _hasUserLimit = (limit > 0); };
		void				setInviteOnly(bool inviteOnly) { _inviteOnly = inviteOnly; };
		void				addInvitation(Client *user);
		void				deleteInvitation(Client *user);
		void				addOperator(Client *user);
		void				removeOperator(Client *user);

	public: /* -Helper Methods- */
		bool				isUserInChannel(Client *user) const;
		bool				isUserOperator(Client *user) const;
	public: /* -Operators- */
		Channel				&operator+=(Client *const);
		Channel				&operator-=(Client *const);
};
