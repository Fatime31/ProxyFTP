#include  <stdio.h>
#include  <stdlib.h>
#include  <sys/socket.h>
#include  <netdb.h>
#include  <string.h>
#include  <unistd.h>
#include  <stdbool.h>
#include "./simpleSocketAPI.h"


#define SERVADDR "127.0.0.1"        // Définition de l'adresse IP d'écoute
#define SERVPORT "0"                // Définition du port d'écoute, si 0 port choisi dynamiquement
#define LISTENLEN 1                 // Taille de la file des demandes de connexion
#define MAXBUFFERLEN 1024           // Taille du tampon pour les échanges de données
#define MAXHOSTLEN 64               // Taille d'un nom de machine
#define MAXPORTLEN 64               // Taille d'un numéro de port
#define PORTFTP "21"                // Port de connexion TCP

int main(){
    int ecode;                       // Code retour des fonctions
    char serverAddr[MAXHOSTLEN];     // Adresse du serveur
    char serverPort[MAXPORTLEN];     // Port du server
    int descSockRDV;                 // Descripteur de socket de rendez-vous
    int descSockCOM;                 // Descripteur de socket de communication
    struct addrinfo hints;           // Contrôle la fonction getaddrinfo
    struct addrinfo *res;            // Contient le résultat de la fonction getaddrinfo
    struct sockaddr_storage myinfo;  // Informations sur la connexion de RDV
    struct sockaddr_storage from;    // Informations sur le client connecté
    socklen_t len;                   // Variable utilisée pour stocker les
				                     // longueurs des structures de socket
    char buffer[MAXBUFFERLEN];       // Tampon de communication entre le client et le serveur

    // Initialisation de la socket de RDV IPv4/TCP
    descSockRDV = socket(AF_INET, SOCK_STREAM, 0);
    if (descSockRDV == -1) {
         perror("Erreur création socket RDV\n");
         exit(2);
    }
    // Publication de la socket au niveau du système
    // Assignation d'une adresse IP et un numéro de port
    // Mise à zéro de hints
    memset(&hints, 0, sizeof(hints));
    // Initialisation de hints
    hints.ai_flags = AI_PASSIVE;      // mode serveur, nous allons utiliser la fonction bind
    hints.ai_socktype = SOCK_STREAM;  // TCP
    hints.ai_family = AF_INET;        // seules les adresses IPv4 seront présentées par
				                      // la fonction getaddrinfo

     // Récupération des informations du serveur
     ecode = getaddrinfo(SERVADDR, SERVPORT, &hints, &res);
     if (ecode) {
         fprintf(stderr,"getaddrinfo: %s\n", gai_strerror(ecode));
         exit(1);
     }
     // Publication de la socket
     ecode = bind(descSockRDV, res->ai_addr, res->ai_addrlen);
     if (ecode == -1) {
         perror("Erreur liaison de la socket de RDV");
         exit(3);
     }
     // Nous n'avons plus besoin de cette liste chainée addrinfo
     freeaddrinfo(res);

     // Récuppération du nom de la machine et du numéro de port pour affichage à l'écran
     len=sizeof(struct sockaddr_storage);
     ecode=getsockname(descSockRDV, (struct sockaddr *) &myinfo, &len);
     if (ecode == -1)
     {
         perror("SERVEUR: getsockname");
         exit(4);
     }
     ecode = getnameinfo((struct sockaddr*)&myinfo, sizeof(myinfo), serverAddr,MAXHOSTLEN,
                         serverPort, MAXPORTLEN, NI_NUMERICHOST | NI_NUMERICSERV);
     if (ecode != 0) {
             fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(ecode));
             exit(4);
     }
     printf("L'adresse d'ecoute est: %s\n", serverAddr);
     printf("Le port d'ecoute est: %s\n", serverPort);

     // Definition de la taille du tampon contenant les demandes de connexion
     ecode = listen(descSockRDV, LISTENLEN);
     if (ecode == -1) {
         perror("Erreur initialisation buffer d'écoute");
         exit(5);
     }

	len = sizeof(struct sockaddr_storage);
     // Attente connexion du client
     // Lorsque demande de connexion, creation d'une socket de communication avec le client
     descSockCOM = accept(descSockRDV, (struct sockaddr *) &from, &len);
     if (descSockCOM == -1){
         perror("Erreur accept\n");
         exit(6);
     }
    // Echange de données avec le client connecté
    strcpy(buffer, "220 Bienvenue pour se connecter rentrer login@adresseServeur\n");
    write(descSockCOM, buffer, strlen(buffer));


     //lis la commande USER anonymous@ftp.fau.de
     ecode =read(descSockCOM,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Recu du client %s\n",buffer );

     //Recuperer le login de l'utilisateur
     char login [50];
     sscanf(buffer,"%[^@]@%s",login,serverAddr);
     printf("Login %s\nServeur %s\n\n",login,serverAddr );

     //Connexion au serveur
     int SockServeurCMD;
     ecode= connect2Server(serverAddr,PORTFTP,&SockServeurCMD);
     if (ecode == -1) {perror("Pb connexion serveur FTP"); exit(2);}
     printf("Bien connecté au serveur FTP\n");

     //lis le retour du serveur
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Recu du client %s\n",buffer );

     //Envoyer User au serveur
     strcat(login,"\r\n");
     write(SockServeurCMD, login, strlen(login));
     printf("Envoi de l'identifiant effectué : %s\n", login);

     //Recupère le message de connexion du serveur et renvoie au client
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Reponse du serveur a l'identifiant : %s\n",buffer);
     write(descSockCOM, buffer, strlen(buffer));

     //Recupération du mot de passe et renvoie au serveur
     ecode =read(descSockCOM,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Mot de passe recupérer : %s\n",buffer);
     write(SockServeurCMD, buffer,strlen(buffer));

     //Recupération réponse serveur et renvoie au client
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Reponse du serveur au mot de passe : %s\n",buffer);
     write(descSockCOM, buffer, strlen(buffer));

     //demande de système au serveur
     ecode =read(descSockCOM,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Commande recupérer : %s\n",buffer);
     write(SockServeurCMD, buffer,strlen(buffer));

     //Reponse serveur à SYST
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Reponse du serveur à la commande SYST : %s\n",buffer);
     write(descSockCOM, buffer, strlen(buffer));

     //Echange de fichier avec le SERVEUR

     //Recupération du PORT du client
     ecode =read(descSockCOM,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Recupération de PORT : %s\n",buffer);

     //découpage de la reponse
     char ip1[4];
     char ip2[4];
     char ip3[4];
     char ip4[4];
     char port1[4];
     char port2[4];
     char ip[20];
     char port[7];
     sscanf(buffer,"PORT %[^,],%[^,],%[^,],%[^,],%[^,],%s",ip1,ip2,ip3,ip4,port1,port2);
     sprintf(ip,"%s.%s.%s.%s",ip1,ip2,ip3,ip4);
     printf ("%s\n",ip);
     sprintf(port,"%d",strtol(port1,NULL,10)*256+strtol(port2,NULL,10));
     printf("%s\n", port);

     //Création de la connexion avec PORT
     int SockClient;
     ecode= connect2Server(ip,port,&SockClient);
     if (ecode == -1) {perror("Pb connexion serveur FTP"); exit(2);}
     printf("Bien connecté au serveur client\n\n");

     //Envoie du message de Bienvenue
     strcpy(buffer, "200 Connexion au Port reussi\n");
     write(descSockCOM, buffer, strlen(buffer));

     //Recupération  port FTP mode passif
     strcpy(buffer, "PASV\r\n");
     write(SockServeurCMD, buffer, strlen(buffer));

     //Recupérer la reponse du serveur
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Reponse du serveur à la commande PASV : %s\n",buffer);

     //découpage de la reponse
     sscanf(buffer,"227 Entering Passive Mode (%[^,],%[^,],%[^,],%[^,],%[^,],%[^)]).",ip1,ip2,ip3,ip4,port1,port2);
     sprintf(ip,"%s.%s.%s.%s",ip1,ip2,ip3,ip4);
     printf ("%s\n",ip);
     sprintf(port,"%d",strtol(port1,NULL,10)*256+strtol(port2,NULL,10));
     printf("%s\n", port);

     //Création de la connexion avec PORT
     int SockFTP;
     ecode= connect2Server(ip,port,&SockFTP);
     if (ecode == -1) {perror("Pb connexion serveur FTP"); exit(2);}
     printf("Bien connecté au serveur FTP\n\n");

     //Recupération de la commande LIST
     ecode =read(descSockCOM,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Commande recupérer : %s\n",buffer);
     write(SockServeurCMD, buffer,strlen(buffer));

     //Recupération reponse du serveur à la commande List
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Commande LIST : %s\n",buffer);
     write(descSockCOM, buffer, strlen(buffer));

     //Envoi de tous les fichiers trouvé
     do {
       ecode =read(SockFTP,buffer,MAXBUFFERLEN-1);
       if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
       buffer [ecode] = '\0';
       write(SockClient, buffer,strlen(buffer));
       sleep(0.5);
     } while (ecode == MAXBUFFERLEN-1) ;

     //Fermeture connexion
     close(SockFTP);
     close(SockClient);

     //Recupération réponse Serveur ftp et renvoie serveur client
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Reponse du serveur à la commande LIST : %s\n",buffer);
     write(descSockCOM, buffer, strlen(buffer));
     printf("Envoyer\n");

     //Recupération commande exit
     ecode =read(descSockCOM,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Reponse du serveur à la commande EXIT : %s\n",buffer);
     write(SockServeurCMD, buffer, strlen(buffer));

     //Renvoie message exit
     ecode =read(SockServeurCMD,buffer,MAXBUFFERLEN-1);
     if (ecode == -1) {perror("Pb lecture dans socket"); exit(2);}
     buffer [ecode] = '\0';
     printf("Reponse du serveur à la commande EXIT : %s\n",buffer);
     write(descSockCOM, buffer, strlen(buffer));

    //Fermeture de la connexion
    close(SockServeurCMD);
    close(descSockCOM);
    close(descSockRDV);
}
