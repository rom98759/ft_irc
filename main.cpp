/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/26 12:02:16 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/26 12:02:16 by kzhen-cl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include "ft_irc.h"

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

static inline char	ft_start_srv(Server *const srv)
{
	std::cout << srv->getPort() << std::endl << srv->getPw() << std::endl;
	return (!!srv);
}

int	main(int argc, char *argv[])
{
	t_srv_set		srv_set = {0, *(argv + 2)};

	if (argc != 3)
	{
		std::cerr << "Error. Program waiting for a <port> and a <password>." << std::endl;
		return (2);
	}
	srv_set._port = ft_atous(*(argv + 1));
	if (!srv_set._port)
	{
		std::cerr << "Error. Incorrect <port> format." << std::endl;
		return (2);
	}
	if (srv_set._pw.empty())
	{
		std::cerr << "Error. Unset <password>." << std::endl;
		return (2);
	}
	return (ft_start_srv((Server *)&srv_set));
}
