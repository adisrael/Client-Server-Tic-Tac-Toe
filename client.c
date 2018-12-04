#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char nickname[254] = {0};
char oponente[254] = {0};
int id = 0;
char buffer[256] = {0};
int tablero[9] = {-1, -1, -1, -1, -1, -1, -1, -1, -1};

void print_tablero() {
  for (int i = 0; i < 9; i++) {
    if (tablero[i] == 0) {
      printf(" - ");
    }
    else if (tablero[i] == 1) {
      printf(" X ");
    }
    else if (tablero[i] == 2) {
      printf(" O ");
    }
    else {
      printf("%d\n", tablero[i]);
    }
    if (i != 2 && i != 5 && i != 8) {
      printf("|");
    }
    if (i == 2 || i == 5 ) {
      printf("\n------------\n");
    }
    if (i == 8) {
      printf("\n");
    }

  }
}

int rcv(int sock, int valread, char buffer[256]) {
  printf("\n");
  char message[256] = {0};

  for (int i = 0; i < 256; i++){
    message[i] = 0;
  }

  for (int i = 0; i < 256; i++){
    buffer[i] = 0;
  }

  unsigned char messageID;
  unsigned char payloadSize;

  valread = read( sock , buffer, 256);
  // printf("messageID %d\n", buffer[0] );
  // printf("payloadSize %d\n", buffer[1] );

  // Connection Established
  if (buffer[0] == 2) {
    printf("Connection Accepted by Server\n");
  }

  // Ask Nickname
  else if (buffer[0] == 3) {
    printf("Ingrese un Nickname (<255 chars.): ");
    scanf("%s", &nickname);
    // printf("%s\n", nickname);

    messageID = 4;
    payloadSize = 254;

    message[0] = messageID;
    message[1] = payloadSize;
    for (int k = 2; k < 256; k++) {
      message[k] = nickname[k - 2];
    }
    send(sock, message, 256, 0);

    for (int i = 0; i < 256; i++){
      message[i] = 0;
    }
  }

  // Opponent Found
  else if (buffer[0] == 5) {

    for (int k = 2; k < 256; k++) {
      oponente[k-2] = buffer[k];
    }

    printf("Oponente: %s\n", oponente);
  }

  // Game Start
  else if (buffer[0] == 6) {
    printf("El juego ha comenzado correctamente\n");
  }

  // Start Round
  else if (buffer[0] == 7) {
    printf("Round %d\n", buffer[2]);
  }

  // Scores
  else if (buffer[0] == 8) {
    printf("Score\n");
    printf("%s: %d\n", nickname, buffer[2]);
    printf("%s: %d\n", oponente, buffer[3]);
  }

  // Who's First
  else if (buffer[0] == 9) {
    id = buffer[2];
    printf("%s: %d\n", nickname, id);
  }

  // Boardstate
  else if (buffer[0] == 10) {
    printf("\nSU TURNO!\n");
    // for (int a = 0; a < 9; a++) {
      // printf("%d\n", tablero[a]);
    // }
    for (int k = 2; k < 11; k++) {
      // printf("%d\n", buffer[k]);
      tablero[k-2] = buffer[k];
    }
    print_tablero();

    int opcion = -1;
    while (opcion != 0 && opcion != 1 && opcion != 2) {
      printf("0. Terminar el juego\n");
      printf("1. Envia un mensaje a tu oponente\n");
      printf("2. Realiza una jugada\n");
      printf("Seleccione una opcion: ");
      scanf("%d", &opcion);

      if (opcion != 0 && opcion != 1 && opcion != 2) {
        printf("Opción inválida :(\n");
      }
    }
    if (opcion == 0) {
      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }
      message[0] = 20;
      send(sock, message, 256, 0);
      printf("Te has desconectado! Chao :(!\n");
      return 1;
    }
    else if (opcion == 1) {
      char c;
      payloadSize = 254;
      message[0] = 22;
      message[1] = payloadSize;
      char mensaje[254];
      printf("Ingrese su mensaje: ");
      scanf("%c", &c);

      scanf("%254[^\n]s", &mensaje);
      // fgets(mensaje, 255, stdin);

      if ((strlen(mensaje) > 0) && (mensaje[strlen (mensaje) - 1] == '\n')) {
        mensaje[strlen (mensaje) - 1] = '\0';
      }

      for (int k = 0; k < 255; k++) {
        message[k + 2] = mensaje[k];
      }

      send(sock, message, 255, 0);

      // valread = read( sock , buffer, 256);
      // printf("mensaje: %s",buffer);
    }
    else if (opcion == 2) {
      message[0] = 11;
      message[1] = 9;

      int jugada = 0;
      while (jugada < 1 || jugada > 9) {
        printf("Ingrese el número de casilla correspondiente según el tablero: \n");
        for (int i = 0; i < 9; i++) {
            if (tablero[i] == 0) {
              printf(" %d ", i+1);
            }
            else if (tablero[i] == 1) {
              printf(" X ");
            }
            else if (tablero[i] == 2) {
              printf(" O ");
            }
            else {
              printf("%d\n", tablero[i]);
            }
            if (i != 2 && i != 5 && i != 8) {
              printf("|");
            }
            if (i == 2 || i == 5 ) {
              printf("\n------------\n");
            }
            if (i == 8) {
              printf("\n");
            }

          }
        printf("En que casilla quiere jugar?\n");
        scanf("%d", &jugada);
      }
      tablero[jugada-1] = id;
      for (int k = 2; k < 11; k++) {
        message[k] = tablero[k-2];
        // printf("%d\n", message[k]);
      }
      send(sock, message, 256, 0);


    }

  }

  // Ask New Game
  else if (buffer[0] == 12) {
    int resp = -1;
    while (resp != 0 && resp != 1) {
      printf("Desea jugar otra partida?\n 1. SI\n 0. NO\n");
      scanf("%d", &resp);
    }

    message[0] = 13;
    message[1] = 1;
    message[2] = resp;

    send(sock, message, 256, 0);
  }

  // Error Board
  else if (buffer[0] == 14) {
    printf("Jugada Enviada INVALIDA\n");
  }

  // OK Board
  else if (buffer[0] == 15) {
    printf("Juguada Enviada OK\n");
  }

  // End Round
  else if (buffer[0] == 16) {
    printf("La ronda %d ha finalizado!\n", buffer[2]);
  }

  // Round Winner/Loser
  else if (buffer[0] == 17) {
    if (buffer[3] == 0) {
      if (buffer[4] == id) {
        printf("Felicidades! Ha ganado la ronda %d\n", buffer[2]);
      }
      else {
        printf("Ha perdido la ronda %d :(\n", buffer[2]);
      }
    }
    else {
      printf("La ronda %d ha terminado en empate!\n", buffer[2]);
    }

  }

  // End Game
  else if (buffer[0] == 18) {
    printf("Fin del juego!\n");
  }

  // Game Winner/Loser
  else if (buffer[0] == 19) {
    if (buffer[2] == id) {
      printf("Felicidades! Ha ganado el juego!\n");
    }
    else {
      printf("Ha perdido el juego :(\n");
    }
  }

  // END
  else if (buffer[0] == 20) {
    printf("Fin del juego\n");
    return 1;
  }

  // Recieve Message
  else if (buffer[0] == 23) {
    char mensaje[255];
    for (int i = 0; i < 255; i++) {
      mensaje[i] = buffer[i + 2];
    }
    printf("Mensaje recibido\n");
    printf("%s: %s\n", oponente, mensaje);
  }

  // No reconozco el id
  else {
    printf("Paquete Desconocido\n");
    return 0;
  }

  return 0;
}

int main(int argc, char const *argv[])
{
    if (argc < 5) {
      printf("Modo de uso: ./client -i <ip_address> -p <tcp-port> -l\n");
      return 1;
    }

    else if (argc == 5) {
      // sin logs
      // printf("%s\n", argv[2]);
      // printf("%s\n", argv[4]);
      int PORT = atoi(argv[4]);

      // struct sockaddr_in address;
      int sock = 0, valread = 0;
      struct sockaddr_in serv_addr;
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          printf("\n Socket creation error \n");
          return -1;
      }

      memset(&serv_addr, '0', sizeof(serv_addr));

      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(PORT);

      // Convert IPv4 and IPv6 addresses from text to binary form
      if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr)<=0) {
          printf("\nInvalid address/ Address not supported \n");
          return -1;
      }

      if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
          printf("\nConnection Failed \n");
          return -1;
      }
      // char *hello = "Hello from client\n";
      // send(sock , hello , strlen(hello) , 0 );
      // printf("Hello message sent\n");

      char message[256] = {0};

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      unsigned char messageID;
      unsigned char payloadSize;

      // start Connection
      messageID = 1;
      payloadSize = 0;

      // if (payloadSize > 0) {
        // unsigned char payload[payloadSize];
      // }

      message[0] = messageID;
      message[1] = payloadSize;

      send(sock, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      int finished = 0;
      while (!finished) {
        finished = rcv(sock, valread, buffer);
      }


    }

    else if (argc == 6) {
      // con logs
      // printf("%s\n", argv[2]);
      // printf("%s\n", argv[4]);
      printf("%s\n", argv[5]);
      int PORT = atoi(argv[4]);

      // struct sockaddr_in address;
      int sock = 0, valread = 0;
      struct sockaddr_in serv_addr;
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
          printf("\n Socket creation error \n");
          return -1;
      }

      memset(&serv_addr, '0', sizeof(serv_addr));

      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(PORT);

      // Convert IPv4 and IPv6 addresses from text to binary form
      if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr)<=0) {
          printf("\nInvalid address/ Address not supported \n");
          return -1;
      }

      if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
          printf("\nConnection Failed \n");
          return -1;
      }
      // char *hello = "Hello from client\n";
      // send(sock , hello , strlen(hello) , 0 );
      // printf("Hello message sent\n");

      char message[256] = {0};

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      unsigned char messageID;
      unsigned char payloadSize;

      // start Connection
      messageID = 1;
      payloadSize = 0;

      // if (payloadSize > 0) {
        // unsigned char payload[payloadSize];
      // }

      message[0] = messageID;
      message[1] = payloadSize;

      send(sock, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      int finished = 0;
      while (!finished) {
        finished = rcv(sock, valread, buffer);
      }


    }

    return 0;
}
