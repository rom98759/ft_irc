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

#include "ft_irc.h"
#include <sys/socket.h>
#include <signal.h>
#include <stdlib.h>

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

static inline void	ft_tmp_add_client(Server *const srv, const int &fd)
{
	try
	{
		Client *const	tmp = new Client(fd);
		*srv += tmp;
		std::cout << "New Client." << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cout << "Error. " << e.what() << std::endl;
	}
}

static inline void	ft_srv_shutdown(int sig)
{
	(void) sig;
	SRV_SHUTDOWN(ft_get_srv());
	exit(0);
}

static inline char	ft_start_srv(void)
{
	Server *const	srv = ft_get_srv();

	signal(SIGINT, ft_srv_shutdown);
	signal(SIGQUIT, ft_srv_shutdown);
	if (!srv->initVector())
	{
		SRV_SHUTDOWN(srv);
		return (1);
	}
	for (char i = -5; i < 10; ++i) // TMP
		ft_tmp_add_client(srv, i); // TMP
	std::cout << srv->getPort() << std::endl << srv->getPw() << std::endl << srv->getFd() << std::endl; // TMP
	while (1)
		;
	SRV_SHUTDOWN(srv);
	return (0);
}

int	main(int argc, char *argv[])
{
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	t_srv_set				srv_set = {0, *(argv + 2), 0, NULL};

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
	srv_set._fd = socket(AF_INET, SOCK_STREAM, 0);
	if (srv_set._fd < 0)
	{
		std::cerr << "Error. Socket Failed." << std::endl;
		return (1);
	}
	ft_set_srv((Server *)&srv_set);
	return (ft_start_srv());
}
