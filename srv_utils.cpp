/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   srv_utils.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/29 14:30:00 by rcaillie          #+#    #+#             */
/*   Updated: 2025/08/29 14:30:00 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_irc.h"

static Server *g_srv = NULL;

void	ft_set_srv(Server *const srv)
{
	g_srv = srv;
}

Server	*ft_get_srv(void)
{
	return (g_srv);
}
