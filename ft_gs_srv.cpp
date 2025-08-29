/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_gs_srv.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kzhen-cl <marvin@d42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/29 11:42:15 by kzhen-cl          #+#    #+#             */
/*   Updated: 2025/08/29 11:42:15 by kzhen-cl         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_irc.h"

static inline Server	*gs(Server *const srv)
{
	static Server *const	save = srv;

	return (save);
}

void	ft_set_srv(Server *const srv)
{
	gs(srv);
}

Server	*ft_get_srv(void)
{
	return (gs(NULL));
}
