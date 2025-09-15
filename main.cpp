/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/26 12:02:16 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/09/10 15:00:58 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"

const Channel	g_nChan("", ""); // Not a channel (Used to fill arrays of "no channels" => equivalent to NULL)

static unsigned short	ft_atous(const char *const a)
{
	int		us = 0;
	char	i = 0;

	for (; i < 5 && *(a + i); ++i)
	{
		if (*(a + i) >= '0' && *(a + i) <= '9')
			us = us * 10 + *(a + i) - '0';
		else
			return (0);
	}
	return (us * (us > 0 && us <= 65635 && !*(a + i)));
}

int	main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return (2);
	}

	unsigned short port = ft_atous(argv[1]);
	if (!port)
	{
		std::cerr << "Error. Incorrect <port> format." << std::endl;
		return (2);
	}

	std::string pw = argv[2];
	if (pw.empty())
	{
		std::cerr << "Error. Unset <password>." << std::endl;
		return (2);
	}

	Server srv(port, pw);
	Server::setInstance(&srv);

	if (!srv.initServer())
		return (1);

	srv.run();

	// La méthode run() se terminera proprement après un signal
	return 0;
}

