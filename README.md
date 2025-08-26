# ft_irc

## **1. Objectif général du projet ft\_irc**

Le projet **ft\_irc** consiste à créer un **serveur de chat en réseau** et un **client** pour se connecter à ce serveur. L’objectif est de comprendre :

* La communication entre clients via un serveur central.
* La gestion d’utilisateurs, de canaux (channels) et de messages.
* Les bases des protocoles réseau en C (sockets, TCP/IP, select…).
* La sécurité et la robustesse (gestion des erreurs, flood, déconnexions…).

---

## **2. Structure du projet**

Le projet se compose généralement de **deux parties principales** :

### a) **Le serveur (ft\_irc)**

* **Écoute les connexions** sur un port TCP.
* **Gère plusieurs clients en simultané** (multi-client).
* Crée et gère des **canaux (channels)** comme `#general` ou `#random`.
* Transmet les messages d’un client aux autres clients d’un canal.
* Implémente des **commandes IRC** : join, part, msg, nick, quit, etc.
* Contrôle des règles comme **le flood** (messages trop fréquents) ou les pseudos déjà utilisés.

### b) **Le client**

* Permet à l’utilisateur de se connecter au serveur.
* Envoie des **commandes** et des messages.
* Reçoit les messages envoyés par d’autres utilisateurs ou par le serveur.
* Affiche l’état des canaux et des utilisateurs connectés.

---

## **3. Concepts clés à comprendre**

### a) **Sockets et TCP**

* Le serveur ouvre une **socket TCP** pour écouter un port.
* Les clients se connectent à cette socket.
* La communication se fait **en flux de caractères**, en respectant le protocole IRC (messages terminés par `\r\n`).

### b) **Select ou poll**

* Permet au serveur de **gérer plusieurs clients en simultané** sans créer un thread par client.
* Le serveur "écoute" tous les clients et réagit quand un message arrive.

### c) **IRC commands**

* Chaque message d’un client peut être une commande IRC ou un message normal.
* Les commandes principales à implémenter :

  * `NICK <pseudo>` : changer ou définir son pseudo.
  * `USER <username>` : définir les infos du client.
  * `JOIN <channel>` : rejoindre un canal.
  * `PART <channel>` : quitter un canal.
  * `PRIVMSG <dest> <message>` : envoyer un message à un utilisateur ou un canal.
  * `QUIT` : se déconnecter.

### d) **Gestion des canaux**

* Chaque canal a :

  * Un nom (`#general`).
  * Une liste d’utilisateurs.
* Le serveur doit **diffuser les messages** du canal uniquement aux membres de ce canal.

### e) **Gestion des clients**

* Le serveur doit **gérer dynamiquement** :

  * Les connexions et déconnexions.
  * L’attribution de pseudos uniques.
  * Les messages entrants et sortants.
  * Les erreurs réseau (client qui coupe la connexion, etc.).

### f) **Robustesse**

* Vérification des entrées pour éviter crashs.
* Protection contre flood (messages trop rapides).
* Gestion correcte des cas limites (pseudo vide, canal inexistant, etc.).

---

## **4. Schéma conceptuel du fonctionnement**

```
+---------+       TCP/IP        +---------+
| Client1 | <----------------> |         |
+---------+                     |         |
                                | Serveur |
+---------+       TCP/IP        |         |
| Client2 | <----------------> |         |
+---------+                     +---------+
                                   |
                           Channels / Users
                           /         \
                      #general      #random
                      /    \        /    \
                   User1  User2  User3  User4
```

* Le **serveur central** gère tout le flux.
* Les clients ne communiquent **jamais directement** entre eux.
* Les **channels** organisent les échanges.

---

## **5. Points à retenir pour bien commencer**

1. Comprendre le **fonctionnement réseau avec TCP et sockets**.
2. Comprendre la **différence client/serveur**.
3. Apprendre à **parser les messages IRC** pour identifier commandes et arguments.
4. Concevoir une **structure interne claire** :

   * Liste des clients connectés.
   * Liste des canaux et membres.
5. Gérer correctement :

   * Connexions/déconnexions.
   * Messages entrants et sortants.
   * Commandes invalides ou malformées.
6. Tester avec plusieurs clients pour vérifier la diffusion des messages.
