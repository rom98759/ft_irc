/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IrcCodes.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rcaillie <rcaillie@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/10 14:30:15 by rcaillie          #+#    #+#             */
/*   Updated: 2025/09/10 14:30:15 by rcaillie         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Les codes de réponse et d'erreur IRC selon RFC 1459/2812

// Réponses (001-099)
#define RPL_WELCOME           "001" // Bienvenue sur le réseau IRC
#define RPL_YOURHOST          "002" // Votre hôte est [serveur]
#define RPL_CREATED           "003" // Ce serveur a été créé [date]
#define RPL_MYINFO            "004" // Information sur le serveur

// Commandes (200-399)
#define RPL_UMODEIS           "221" // Mode utilisateur
#define RPL_LUSERCLIENT       "251" // Utilisateurs connectés
#define RPL_LUSEROP           "252" // Opérateurs connectés
#define RPL_LUSERUNKNOWN      "253" // Connexions inconnues
#define RPL_LUSERCHANNELS     "254" // Canaux formés
#define RPL_LUSERME           "255" // Information sur le serveur
#define RPL_AWAY              "301" // [utilisateur] est absent
#define RPL_UNAWAY            "305" // Vous n'êtes plus marqué comme absent
#define RPL_NOWAWAY           "306" // Vous êtes maintenant marqué comme absent
#define RPL_WHOISUSER         "311" // Infos WHOIS utilisateur
#define RPL_ENDOFWHO          "315" // Fin de la liste WHO
#define RPL_ENDOFWHOIS        "318" // Fin de la liste WHOIS
#define RPL_CHANNELMODEIS     "324" // Mode du canal
#define RPL_NOTOPIC           "331" // Pas de sujet défini pour le canal
#define RPL_TOPIC             "332" // Sujet du canal
#define RPL_NAMREPLY          "353" // Liste des utilisateurs d'un canal
#define RPL_ENDOFNAMES        "366" // Fin de la liste NAMES

// Erreurs (400-599)
#define ERR_NOSUCHNICK        "401" // Pseudo/canal inexistant
#define ERR_NOSUCHCHANNEL     "403" // Canal inexistant
#define ERR_CANNOTSENDTOCHAN  "404" // Impossible d'envoyer au canal
#define ERR_UNKNOWNCOMMAND    "421" // Commande inconnue
#define ERR_NOMOTD            "422" // MOTD manquant
#define ERR_NONICKNAMEGIVEN   "431" // Aucun pseudo fourni
#define ERR_ERRONEUSNICKNAME  "432" // Pseudo erroné
#define ERR_NICKNAMEINUSE     "433" // Pseudo déjà utilisé
#define ERR_NOTONCHANNEL      "442" // Vous n'êtes pas sur ce canal
#define ERR_NOTREGISTERED     "451" // Vous n'êtes pas enregistré
#define ERR_TOOMANYPARAMS     "460" // Trop de paramètres
#define ERR_NEEDMOREPARAMS    "461" // Pas assez de paramètres
#define ERR_ALREADYREGISTERED "462" // Déjà enregistré
#define ERR_PASSWDMISMATCH    "464" // Mot de passe incorrect
#define ERR_CHANNELISFULL     "471" // Canal plein
#define ERR_UNKNOWNMODE       "472" // Mode inconnu
#define ERR_INVITEONLYCHAN    "473" // Canal en invitation uniquement
#define ERR_BANNEDFROMCHAN    "474" // Banni du canal
#define ERR_BADCHANNELKEY     "475" // Mauvaise clé de canal
#define ERR_BADCHANMASK       "476" // Mauvais masque de canal
#define ERR_CHANOPRIVSNEEDED  "482" // Vous n'êtes pas opérateur du canal
