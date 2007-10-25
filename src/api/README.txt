Interface Web.
Avril 2005
###################################################
#                                                 #
# README.TXT                                      #
# Carlos Vaz Et Christophe Guérin                 #
# Réalisé dans le Cadre du Projet Synthèse        #
# Du Baccalauréat en Informatique                 #
#                                                 #
###################################################
Ce Fichier README contient de l'information au sujet de la configuration et des fichiers nécessaires pour utiliser l'interface Web du Moteur de Traduction.
________________________________________________________________
Table des Matières.
1. Comment doit être l'environnement sur le serveur pour utiliser l'interface ?
2. Quels sont les fichiers nécessaires et que font-ils
3. Qu'elles sont les commandes pour lancer un client ou un serveur
4. Quelles sont les modifications à prévoir
5. Guide d'utilisation de l'interface
===================================================================
===================================================================
1. Comment doit être l'environnement sur le serveur pour utiliser l'interface ?
- Dans le répertoire contenant l'interface web (php):
  - Avoir un répertoire "temp" appartenant au groupe "webauthor" dans le répertoire de l'interface web où le groupe peut écrire. - Ce répertoire servira à recevoir les fichiers temporaires qui seront effacés tout de suite après leur utilisation;
  - Avoir les fichiers apparaissants au point 2 ci-dessous;
===================================================================
===================================================================
2. Quels sont les fichiers nécessaires et que font-ils:
Les fichiers HTML:
- index.html: Point d'entré du site de démonstration cette page permet de choisir la langue de navigation;
- interface_fra.html: page en français
- interface_eng.html: page en anglais
Si quelqu'un veut modifier l'interface web, c'est dans ces fichiers qu'il doit le faire.
Les fichiers PHP (Scripts):
Ces scripts sont très importants puisqu'ils font presque tout le travail. Ils sont bien documentés par des commentaires qui expliquent le rôle de chaque variable et fonction. Voici la description de ces scripts.
- trad_fra: script qui s'occupe de prendre le texte tapé, le mettre dans un fichier et le donner au client ACE avec le # de port relatif à la paire de langues choisies et l'adresse du serveur. Ce script s'occupe aussi d'afficher la traduction accompagnée du texte original.
- trad_chin_fra: Script qui permet de prendre soit le fichier par défaut (CPP.gb) soit un des sites web listés dans interface_fra.html. Ensuite, le script met leur contenu dans un fichier pour tout donner au client ACE avec le # de port relatif à la paire de langues (chinois vers l'anglais) et l'adresse du serveur. Ce script s'occupe aussi d'afficher la traduction dans son format original.
- trad_eng: idem a trad_fra, mais le tout avec une interface en Anglais
- trad_chin_eng: idem a trad_chin_fra, mais le tout avec une interface en Anglais
Les autres fichiers
- corp_0_f.jpeg: logo du CNRC\NRC
- paddle_client: fichier binaire résultant de la compilation de paddle_client.cc
- CPP.gb: fichier contenant du text chinois (avec l'encodage GB2312) sont contenu peut être changé mais pas son nom sinon il faudra aussi changer le nom dans trad_chin_fra et trad_chin_eng.
Les fichiers d'ACE qui peuvent être ailleurs:
- paddle_client.cc: Code source de l'outil de communication entre l'interface web et le serveur ACE. Cet outil est programmé en C++ avec les librairies d'ACE.
- paddle_server.cc: Code source du programme ACE qui fait le lien entre le client et le moteur de traduction.
- paddle_server: fichier binaire résultant de la compilation de paddle_server.cc
- Makefile: Outil pour la compilation de paddle_client.cc et paddle_server.cc (en même temps).
===================================================================
===================================================================
3. Qu'elles sont les commandes pour lancer un client ou un serveur:
Le serveur peut être utilisé en mode d'écoute ou comme application interactive en lui fournissant un fichier à traduire voici comment utiliser les deux méthodes:

paddle_server [-v] [-pPortNum] srclang tgtlang [infile [outfile]]

Pour lancer un serveur en mode écoute:
Un serveur prend le # du port sur lequel il doit écouter en paramètre ainsi que la paire de langue à utiliser pour la traduction.
EX: " ./paddle_server -p20002 en fr"
ou si on désire lancer l'application pour traduire un fichier de manière interactive:
" ./paddle_server lang_source langue_traduite nom_du_fichier" pour la nouvelle version qui prend les paires de langue qui sont disponibles (français "fr", anglais"en" et chinois "ch"), il faut s'assurer que les langues soient mises dans la direction que se fera la traduction et que cette traduction soit disponible.
EX: " ./paddle_server fr en "
Pour lancer un client:
Habituellement, le script PHP se charge de cette tâche, mais on peut le lancer de manière interactive comme suit:
" ./paddle_client fichier port adresse "
EX: " ./paddle_client file.txt 20002 ltrcgatssh2.crtl.ca "
====================================================================
====================================================================
4. Quelles sont les modifications à prévoir (idées futures)
- Il faudrait vérifier pourquoi les caractères chinois collés dans le « textarea » ne sont pas traduits. Ils sont tout simplement réaffichés sans avoir été traités par le moteur de traduction. (paulp: tokenization)
- Il serait envisageable de créer un script qui rendrait toutes les adresses relatives, contenues dans les sites web, en adresses absolues, afin de conserver des adresses valides pour les éléments tel les images dans les pages traduites. (paulp: injection de la balise <base href="http://unurl.com"> voir http://dev.crtl.ca/~paulp/relative.html pour exemple)
=====================================================================
=====================================================================
5. Guide d'utilisation de l'interface
- La page de départ :
Lorsqu'on ouvre la page de démarrage pour l'interface, les options de poursuivre en français ou en anglais, alors il faut peser la langue de son choix.
- La page de l'interface (idem pour les deux langues officielles) Cette page comprend 2 sections :
- La première consiste à une boîte de texte où il est possible de rentrer des phrases. Par la suite, il faut choisir la direction dans laquelle la traduction sera fait. Après avoir remplie la boîte de texte et choisi la paire de langues, on appuie sur le bouton soumettre.
- La deuxième comprend une liste de sites web qui ont un encodage de GB2312. Dans cette section, il suffit de « clicker » sur le bouton radio placé à coté du lien afin qu'il soit sélectionné pour être traduit. Les liens qui sont présent, ont été mis à titre de référence afin de voir la page originale du site qu'on traduit. De plus, dans cette section, il existe un fichier de texte défini, c'est-à-dire que le document ne changera pas quoiqu'il en soit.
- Les pages de résultat de traduction :
Ces pages affichent le résultat de la traduction qui a été demandée. Il est possible qu'il n'y ait pas de réponse, on doit en premier lieu s'assurer que les serveurs que l'on désire utiliser doivent être fonctionnels. Après la lecture de la réponse, on peut soit retourner à l'interface pour faire une nouvelle traduction en cliquant sur les boutons à cet effet ou bien aller visiter le site du CNRC en appuyant sur le logo.

