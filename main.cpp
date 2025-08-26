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
	unsigned short	port;

	if (argc != 3)
	{
		std::cerr << "Error. Program waiting for a <port> and a <password>." << std::endl;
		return (2);
	}
	port = ft_atous(*(argv + 1));
	if (!port)
	{
		std::cerr << "Error. Incorrect <port> format." << std::endl;
		return (2);
	}
	return (0);
}
