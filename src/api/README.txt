Interface Web.
Avril 2005
###################################################
#                                                 #
# README.TXT                                      #
# Carlos Vaz Et Christophe Gu�rin                 #
# R�alis� dans le Cadre du Projet Synth�se        #
# Du Baccalaur�at en Informatique                 #
#                                                 #
###################################################
Ce Fichier README contient de l'information au sujet de la configuration et des fichiers n�cessaires pour utiliser l'interface Web du Moteur de Traduction.
________________________________________________________________
Table des Mati�res.
1. Comment doit �tre l'environnement sur le serveur pour utiliser l'interface ?
2. Quels sont les fichiers n�cessaires et que font-ils
3. Qu'elles sont les commandes pour lancer un client ou un serveur
4. Quelles sont les modifications � pr�voir
5. Guide d'utilisation de l'interface
===================================================================
===================================================================
1. Comment doit �tre l'environnement sur le serveur pour utiliser l'interface ?
- Dans le r�pertoire contenant l'interface web (php):
  - Avoir un r�pertoire "temp" appartenant au groupe "webauthor" dans le r�pertoire de l'interface web o� le groupe peut �crire. - Ce r�pertoire servira � recevoir les fichiers temporaires qui seront effac�s tout de suite apr�s leur utilisation;
  - Avoir les fichiers apparaissants au point 2 ci-dessous;
===================================================================
===================================================================
2. Quels sont les fichiers n�cessaires et que font-ils:
Les fichiers HTML:
- index.html: Point d'entr� du site de d�monstration cette page permet de choisir la langue de navigation;
- interface_fra.html: page en fran�ais
- interface_eng.html: page en anglais
Si quelqu'un veut modifier l'interface web, c'est dans ces fichiers qu'il doit le faire.
Les fichiers PHP (Scripts):
Ces scripts sont tr�s importants puisqu'ils font presque tout le travail. Ils sont bien document�s par des commentaires qui expliquent le r�le de chaque variable et fonction. Voici la description de ces scripts.
- trad_fra: script qui s'occupe de prendre le texte tap�, le mettre dans un fichier et le donner au client ACE avec le # de port relatif � la paire de langues choisies et l'adresse du serveur. Ce script s'occupe aussi d'afficher la traduction accompagn�e du texte original.
- trad_chin_fra: Script qui permet de prendre soit le fichier par d�faut (CPP.gb) soit un des sites web list�s dans interface_fra.html. Ensuite, le script met leur contenu dans un fichier pour tout donner au client ACE avec le # de port relatif � la paire de langues (chinois vers l'anglais) et l'adresse du serveur. Ce script s'occupe aussi d'afficher la traduction dans son format original.
- trad_eng: idem a trad_fra, mais le tout avec une interface en Anglais
- trad_chin_eng: idem a trad_chin_fra, mais le tout avec une interface en Anglais
Les autres fichiers
- corp_0_f.jpeg: logo du CNRC\NRC
- paddle_client: fichier binaire r�sultant de la compilation de paddle_client.cc
- CPP.gb: fichier contenant du text chinois (avec l'encodage GB2312) sont contenu peut �tre chang� mais pas son nom sinon il faudra aussi changer le nom dans trad_chin_fra et trad_chin_eng.
Les fichiers d'ACE qui peuvent �tre ailleurs:
- paddle_client.cc: Code source de l'outil de communication entre l'interface web et le serveur ACE. Cet outil est programm� en C++ avec les librairies d'ACE.
- paddle_server.cc: Code source du programme ACE qui fait le lien entre le client et le moteur de traduction.
- paddle_server: fichier binaire r�sultant de la compilation de paddle_server.cc
- Makefile: Outil pour la compilation de paddle_client.cc et paddle_server.cc (en m�me temps).
===================================================================
===================================================================
3. Qu'elles sont les commandes pour lancer un client ou un serveur:
Le serveur peut �tre utilis� en mode d'�coute ou comme application interactive en lui fournissant un fichier � traduire voici comment utiliser les deux m�thodes:

paddle_server [-v] [-pPortNum] srclang tgtlang [infile [outfile]]

Pour lancer un serveur en mode �coute:
Un serveur prend le # du port sur lequel il doit �couter en param�tre ainsi que la paire de langue � utiliser pour la traduction.
EX: " ./paddle_server -p20002 en fr"
ou si on d�sire lancer l'application pour traduire un fichier de mani�re interactive:
" ./paddle_server lang_source langue_traduite nom_du_fichier" pour la nouvelle version qui prend les paires de langue qui sont disponibles (fran�ais "fr", anglais"en" et chinois "ch"), il faut s'assurer que les langues soient mises dans la direction que se fera la traduction et que cette traduction soit disponible.
EX: " ./paddle_server fr en "
Pour lancer un client:
Habituellement, le script PHP se charge de cette t�che, mais on peut le lancer de mani�re interactive comme suit:
" ./paddle_client fichier port adresse "
EX: " ./paddle_client file.txt 20002 ltrcgatssh2.crtl.ca "
====================================================================
====================================================================
4. Quelles sont les modifications � pr�voir (id�es futures)
- Il faudrait v�rifier pourquoi les caract�res chinois coll�s dans le � textarea � ne sont pas traduits. Ils sont tout simplement r�affich�s sans avoir �t� trait�s par le moteur de traduction. (paulp: tokenization)
- Il serait envisageable de cr�er un script qui rendrait toutes les adresses relatives, contenues dans les sites web, en adresses absolues, afin de conserver des adresses valides pour les �l�ments tel les images dans les pages traduites. (paulp: injection de la balise <base href="http://unurl.com"> voir http://dev.crtl.ca/~paulp/relative.html pour exemple)
=====================================================================
=====================================================================
5. Guide d'utilisation de l'interface
- La page de d�part :
Lorsqu'on ouvre la page de d�marrage pour l'interface, les options de poursuivre en fran�ais ou en anglais, alors il faut peser la langue de son choix.
- La page de l'interface (idem pour les deux langues officielles) Cette page comprend 2 sections :
- La premi�re consiste � une bo�te de texte o� il est possible de rentrer des phrases. Par la suite, il faut choisir la direction dans laquelle la traduction sera fait. Apr�s avoir remplie la bo�te de texte et choisi la paire de langues, on appuie sur le bouton soumettre.
- La deuxi�me comprend une liste de sites web qui ont un encodage de GB2312. Dans cette section, il suffit de � clicker � sur le bouton radio plac� � cot� du lien afin qu'il soit s�lectionn� pour �tre traduit. Les liens qui sont pr�sent, ont �t� mis � titre de r�f�rence afin de voir la page originale du site qu'on traduit. De plus, dans cette section, il existe un fichier de texte d�fini, c'est-�-dire que le document ne changera pas quoiqu'il en soit.
- Les pages de r�sultat de traduction :
Ces pages affichent le r�sultat de la traduction qui a �t� demand�e. Il est possible qu'il n'y ait pas de r�ponse, on doit en premier lieu s'assurer que les serveurs que l'on d�sire utiliser doivent �tre fonctionnels. Apr�s la lecture de la r�ponse, on peut soit retourner � l'interface pour faire une nouvelle traduction en cliquant sur les boutons � cet effet ou bien aller visiter le site du CNRC en appuyant sur le logo.

