#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SEC_PER_DAY   86400
#define SEC_PER_HOUR  3600
#define SEC_PER_MIN   60

typedef struct Hms {
  int h;
  int m;
  int s;
} Hms;

Hms get_time() {
  Hms now;

  struct timeval start;
  struct timezone tz;

  gettimeofday(&start, &tz);

  // Form the seconds of the day
  long hms = start.tv_sec % SEC_PER_DAY;
  hms += tz.tz_dsttime * SEC_PER_HOUR;
  hms -= tz.tz_minuteswest * SEC_PER_MIN;
  // mod `hms` to insure in positive range of [0...SEC_PER_DAY)
  hms = (hms + SEC_PER_DAY) % SEC_PER_DAY;

  // Tear apart hms into h:m:s
  now.h = hms / SEC_PER_HOUR;
  now.m = (hms % SEC_PER_HOUR) / SEC_PER_MIN;
  now.s = (hms % SEC_PER_HOUR) % SEC_PER_MIN; // or hms % SEC_PER_MIN

  return now;
}

void write_log(FILE *log, char buffer[256]) {
  Hms now = get_time();
  fprintf(log, "%d:%d:%d\t%d\t%s", now.h, now.m, now.s, buffer[0], buffer);
}

int tablero[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
char nickname1[254], nickname2[254];
int ronda = 1;
int score1 = 0, score2 = 0;
int player1 = 0, player2 = 0;
int turno = 1;
int player1_socket, player2_socket;
// socks

int revisar_jugada(int tablero_modificado[9]) {
  // reviso que la jugada sea valida
  // si es valida modifica el tablero y retorna 1
  // else retorna 0
  int distinto[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  int distintos = 0;
  int valido = 1;
  for (int pos = 0; pos < 9; pos++) {
    if (tablero_modificado[pos] != tablero[pos]) {
      distinto[pos] = 1;
      distintos++;
      if (tablero[pos] != 0) {
        valido = 0;
      }
    }
  }
  if (valido) {
    if (distintos != 1) {
      return 0;
    }
    for (int pos = 0; pos < 9; pos++) {
      if (distinto[pos] == 1) {
        tablero[pos] = tablero_modificado[pos];
        return 1;
      }
    }
  }

  return 0;

}

int check_game(int tablero[9]) {
  for (int i = 0; i < 3; i++) {
    // check filas
    if (tablero[0 + i * 3] == tablero[1 + i * 3] && tablero[1 + i * 3] == tablero[2 + i * 3] && tablero[0 + i * 3] != 0) {
      if (tablero[0 + i * 3] == 1) {
        return 11;
      }
      else {
        return 12;
      }
    }

    // check columnas
    if (tablero[0 + i] == tablero[3 + i] && tablero[3 + i] == tablero[6 + i] && tablero[0 + i] != 0) {
      if (tablero[0 + i] == 1) {
        return 11;
      }
      else {
        return 12;
      }
    }
  }

  // check diagonal 1
  if (tablero[0] == tablero[4] && tablero[4] == tablero[8] && tablero[4] != 0) {
    if (tablero[4] == 1) {
      return 11;
    }
    else {
      return 12;
    }
  }

  // check diagonal 2
  if (tablero[2] == tablero[4] && tablero[4] == tablero[6] && tablero[4] != 0) {
    if (tablero[4] == 1) {
      return 11;
    }
    else {
      return 12;
    }
  }

  return 0;
}

int check_full_board(int tablero[9]) {
  for (int i = 0; i < 9; i++) {
    if (tablero[i] == 0) {
      return 0;
    }
  }
  return 1;
}

void clean_board(int tablero[9]) {
  for (int i = 0; i < 9; i++) {
    tablero[i] = 0;
  }
}

int rcv(int player_sock, int valread, char buffer[256]) {
  char message[256] = {0};

  for (int i = 0; i < 256; i++){
    message[i] = 0;
  }

  for (int i = 0; i < 256; i++){
    buffer[i] = 0;
  }

  valread = read( player_sock , buffer, 256);
  // printf("messageID %d\n", buffer[0] );
  // printf("payloadSize %d\n", buffer[1] );

  // Update Board
  if (buffer[0] == 11) {
    int tablero_modificado[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
    for (int k = 2; k < 11; k++) {
      // printf("%d\n", buffer[k]);
      tablero_modificado[k-2] = buffer[k];
    }
    if (revisar_jugada(tablero_modificado)) {
      Hms now = get_time();
      printf("%d:%d:%d\t", now.h, now.m, now.s);
      printf("Jugada Valida\n");
      message[0] = 15;
      message[1] = 0;

      send(player_sock, message, 256, 0);
      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      int check = check_game(tablero);
      if (check) {
        Hms now = get_time();
        printf("%d:%d:%d\t", now.h, now.m, now.s);
        printf("Fin de la ronda %d\n", ronda);
        if (check == player1) {
          printf("%s ha ganado la ronda!\n", nickname1);
          score1++;
          printf("Puntaje de %s: %d\n", nickname1, score1);
          printf("Puntaje de %s: %d\n", nickname2, score2);

          clean_board(tablero);

          if (score1 == 3 && player1 == 1) {
            return 21;
          }
          if (score1 == 3 && player1 == 2) {
            return 22;
          }

          return check;

        }
        else {
          printf("%s ha ganado la ronda!\n", nickname2);

          score1++;
          printf("Puntaje de %s: %d\n", nickname1, score1);
          printf("Puntaje de %s: %d\n", nickname2, score2);

          clean_board(tablero);

          if (score2 == 3 && player2 == 1) {
            return 21;
          }
          if (score2 == 3 && player2 == 2) {
            return 22;
          }


          return check;
        }
        return 3;
      }

      int full = check_full_board(tablero);
      if (full) {
        Hms now = get_time();
        printf("%d:%d:%d\t", now.h, now.m, now.s);
        printf("Fin de la ronda %d\n", ronda);
        printf("La ronda ha terminado en empate!\n");

        clean_board(tablero);
        ronda++;
        return 0;
      }

      if (turno == 1) {
        turno = 2;
      }
      else {
        turno = 1;
      }
      return turno;
    }
    else {
      Hms now = get_time();
      printf("%d:%d:%d\t", now.h, now.m, now.s);
      printf("Jugada INVALIDA... Repitiendo\n");
      message[0] = 14;
      message[1] = 0;

      send(player_sock, message, 256, 0);
      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

    }


  }

  // Recieve Message
  if (buffer[0] == 22) {
    for (int i = 0; i < 256; i++){
      message[i] = 0;
    }
    char mensaje[255];
    for (int i = 0; i < 255; i++) {
      mensaje[i] = buffer[i + 2];
    }
    Hms now = get_time();
    printf("%d:%d:%d\t", now.h, now.m, now.s);
    printf("Mensaje recibido: %s\n", mensaje);
    if (player_sock == player1_socket) {
      int payloadSize = 254;
      message[0] = 23;
      message[1] = payloadSize;
      for (int i = 0; i < 255; i++) {
        message[i + 2] = mensaje[i];
      }
      send(player2_socket, message, 256, 0);
      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }
    }
    else {
      int payloadSize = 254;
      message[0] = 23;
      message[1] = payloadSize;
      for (int i = 0; i < 255; i++) {
        message[i + 2] = mensaje[i];
      }
      send(player1_socket, message, 256, 0);
      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }
    }

  }


  if (buffer[0] == 20) {
    Hms now = get_time();
    printf("%d:%d:%d\t", now.h, now.m, now.s);
    printf("Un jugador se ha desconectado\n");
    printf("Fin del juego!\n");
    if (player_sock == player1_socket) {
      message[0] = 20;
      message[1] = 0;
      send(player2_socket, message, 256, 0);
    }
    else {
      message[0] = 20;
      message[1] = 0;
      send(player1_socket, message, 256, 0);
    }
    return 20;
  }

  // printf("turno: %d\n", turno);
  return turno;
}

int main(int argc, char *argv[]) {
  // printf("%d\n", argc);
  if (argc < 5) {
    printf("Modo de uso: ./server -i <ip_address> -p <tcp-port> -l\n");
    return 1;
  }

  if (argc == 5) {
    // sin logs
    // printf("%s\n", argv[2]);
    // printf("%s\n", argv[4]);
    // int sockfd, newsockfd, portno, clilen, n;
    // sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // if (sockfd < 0) {
    //   error("ERROR opening socket");
    // }

    ////
    int server_fd, valread;
    // int clients[2];
    int count = 0;
    struct sockaddr_in address;
    // int opt = 1;
    int addrlen = sizeof(address);
    char buffer[256] = {0};
    int PORT = atoi(argv[4]);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    memset(&address, '0', sizeof(address));
    address.sin_family = AF_INET;
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, argv[2], &address.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    address.sin_addr.s_addr = INADDR_ANY; // localhost address
    address.sin_port = htons( PORT );

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while (count == 0) {
      player1_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
      if ((player1_socket)<0) {
          perror("accept");
          exit(EXIT_FAILURE);
      }
      else {
        // clients[0] = player1_socket;
        count++;

        valread = read( player1_socket , buffer, 256);
        // printf("messageID %x\n", buffer[0] );
        // printf("payloadSize %x\n", buffer[1] );
        if (buffer[0] == 1) {
          Hms now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Connection Requested\n");
          char message[256] = {0};
          unsigned char messageID;
          unsigned char payloadSize;

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          // accept Connection
          messageID = 2;
          payloadSize = 0;

          message[0] = messageID;
          message[1] = payloadSize;

          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Accepting Connection\n");
          send(player1_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          messageID = 3;
          payloadSize = 0;

          message[0] = messageID;
          message[1] = payloadSize;
          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Asking for Nickname\n");
          send(player1_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          valread = read(player1_socket, buffer, 256);
          // printf("messageID %x\n", buffer[0] );
          // printf("payloadSize %x\n", buffer[1] );
          if (buffer[0] == 4) {

            for (int k = 2; k < 256; k++) {
              nickname1[k-2] = buffer[k];
            }
            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player1: %s\n", nickname1);
          }
        }

      }
    }
    while (count == 1) {
      if ((player2_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
          perror("accept");
          exit(EXIT_FAILURE);
      }
      else {
        // clients[1] = player2_socket;
        count++;

        valread = read( player2_socket , buffer, 256);
        // printf("messageID %x\n", buffer[0] );
        // printf("payloadSize %x\n", buffer[1] );
        if (buffer[0] == 1) {
          Hms now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Connection Requested\n");
          char message[256] = {0};
          unsigned char messageID;
          unsigned char payloadSize;

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          // accept Connection
          messageID = 2;
          payloadSize = 0;

          message[0] = messageID;
          message[1] = payloadSize;

          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Accepting Connection\n");
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          messageID = 3;
          payloadSize = 0;

          message[0] = messageID;
          message[1] = payloadSize;
          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Asking for Nickname\n");
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          valread = read(player2_socket, buffer, 256);
          // printf("messageID %x\n", buffer[0] );
          // printf("payloadSize %x\n", buffer[1] );
          if (buffer[0] == 4) {

            for (int k = 2; k < 256; k++) {
              nickname2[k-2] = buffer[k];
            }
            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player2: %s\n", nickname2);
          }
        }
      }

    }

    if (count == 2) {
      char message[256] = {0};
      unsigned char messageID;
      unsigned char payloadSize;

      messageID = 5;
      payloadSize = 254;

      message[0] = messageID;
      message[1] = payloadSize;

      for (int k = 2; k < 256; k++) {
        message[k] = nickname2[k - 2];
      }

      send(player1_socket, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      messageID = 5;
      payloadSize = 254;

      message[0] = messageID;
      message[1] = payloadSize;

      for (int k = 2; k < 256; k++) {
        message[k] = nickname1[k - 2];
      }

      send(player2_socket, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      // Game Start

      messageID = 6;
      payloadSize = 0;

      message[0] = messageID;
      message[1] = payloadSize;

      send(player1_socket, message, 256, 0);
      send(player2_socket, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      message[0] = 7;
      message[1] = 1;
      message[2] = ronda;

      send(player1_socket, message, 256, 0);
      send(player2_socket, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      message[0] = 8;
      message[1] = 2;
      message[2] = score1;
      message[3] = score2;

      send(player1_socket, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      message[0] = 8;
      message[1] = 2;
      message[2] = score2;
      message[3] = score1;
      send(player2_socket, message, 256, 0);

      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }


      // Who's First

      srand(time(NULL));
      message[0] = 9;
      message[1] = 1;
      int r = (rand() % 2 + 1);
      // int r = 1; // con 1 cliente
      message[2] = r;
      player1 = r;
      Hms now = get_time();
      printf("%d:%d:%d\t", now.h, now.m, now.s);
      printf("Player 1 ID: %d\n", message[2]);

      send(player1_socket, message, 256, 0);
      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      message[0] = 9;
      message[1] = 1;
      if (r == 1) {
        message[2] = 2;
        player2 = 2;
      }
      else {
        message[2] = 1;
        player2 = 1;
      }

      now = get_time();
      printf("%d:%d:%d\t", now.h, now.m, now.s);
      printf("Player 2 ID: %d\n", message[2]);

      send(player2_socket, message, 256, 0);
      for (int i = 0; i < 256; i++){
        message[i] = 0;
      }

      // Boardstate

      message[0] = 10;
      message[1] = 9;
      for (int k = 2; k < 11; k++) {
        message[k] = tablero[k-2];
        // printf("%d\n", message[k]);
      }

      int termino = 0;
      while (termino != 20) {
        if (turno == player1) {
          send(player1_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          termino = rcv(player1_socket, valread, buffer);
        }
        else {
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          termino = rcv(player2_socket, valread, buffer);
        }

        if (termino == 20) {
          break;
        }

        if (termino == 1 || termino == 2) {
          message[0] = 10;
          message[1] = 9;
          for (int k = 2; k < 11; k++) {
            message[k] = tablero[k-2];
            // printf("%d\n", message[k]);
          }
        }

        if (termino == 11) {
          message[0] = 16;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 17;
          message[1] = 3;
          message[2] = ronda;
          message[3] = 0;
          message[4] = 1;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          ronda++;

          message[0] = 7;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 8;
          message[1] = 2;
          message[2] = score1;
          message[3] = score2;

          send(player1_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 8;
          message[1] = 2;
          message[2] = score2;
          message[3] = score1;
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          srand(time(NULL));
          message[0] = 9;
          message[1] = 1;
          int r = (rand() % 2 + 1);
          // int r = 1; // con 1 cliente
          message[2] = r;
          player1 = r;
          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Player 1 ID: %d\n", message[2]);

          send(player1_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 9;
          message[1] = 1;
          if (r == 1) {
            message[2] = 2;
            player2 = 2;
          }
          else {
            message[2] = 1;
            player2 = 1;
          }

          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Player 2 ID: %d\n", message[2]);

          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 10;
          message[1] = 9;
          for (int k = 2; k < 11; k++) {
            message[k] = tablero[k-2];
            // printf("%d\n", message[k]);
          }
        }

        if (termino == 12) {
          message[0] = 16;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 17;
          message[1] = 3;
          message[2] = ronda;
          message[3] = 0;
          message[4] = 2;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          ronda++;

          message[0] = 7;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 8;
          message[1] = 2;
          message[2] = score1;
          message[3] = score2;

          send(player1_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 8;
          message[1] = 2;
          message[2] = score2;
          message[3] = score1;
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          srand(time(NULL));
          message[0] = 9;
          message[1] = 1;
          int r = (rand() % 2 + 1);
          // int r = 1; // con 1 cliente
          message[2] = r;
          player1 = r;

          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Player 1 ID: %d\n", message[2]);

          send(player1_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 9;
          message[1] = 1;
          if (r == 1) {
            message[2] = 2;
            player2 = 2;
          }
          else {
            message[2] = 1;
            player2 = 1;
          }

          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Player 2 ID: %d\n", message[2]);

          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 10;
          message[1] = 9;
          for (int k = 2; k < 11; k++) {
            message[k] = tablero[k-2];
            // printf("%d\n", message[k]);
          }
        }

        if (termino == 0) {
          message[0] = 16;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 17;
          message[1] = 2;
          message[2] = ronda;
          message[3] = 1;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          ronda++;

          message[0] = 7;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 8;
          message[1] = 2;
          message[2] = score1;
          message[3] = score2;

          send(player1_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 8;
          message[1] = 2;
          message[2] = score2;
          message[3] = score1;
          send(player2_socket, message, 256, 0);

          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          srand(time(NULL));
          message[0] = 9;
          message[1] = 1;
          int r = (rand() % 2 + 1);
          // int r = 1; // con 1 cliente
          message[2] = r;
          player1 = r;
          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Player 1 ID: %d\n", message[2]);

          send(player1_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 9;
          message[1] = 1;
          if (r == 1) {
            message[2] = 2;
            player2 = 2;
          }
          else {
            message[2] = 1;
            player2 = 1;
          }

          now = get_time();
          printf("%d:%d:%d\t", now.h, now.m, now.s);
          printf("Player 2 ID: %d\n", message[2]);

          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 10;
          message[1] = 9;
          for (int k = 2; k < 11; k++) {
            message[k] = tablero[k-2];
            // printf("%d\n", message[k]);
          }
        }

        if (termino == 21) {
          message[0] = 16;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 17;
          message[1] = 3;
          message[2] = ronda;
          message[3] = 0;
          message[4] = 1;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 18;
          message[1] = 0;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 19;
          message[1] = 1;
          message[2] = 1;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 12;
          message[1] = 0;

          send(player1_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          for (int i = 0; i < 256; i++){
            buffer[i] = 0;
          }

          valread = read( player1_socket , buffer, 256);
          // printf("messageID %d\n", buffer[0] );
          // printf("payloadSize %d\n", buffer[1] );
          if (buffer[0] == 13) {
            if (buffer[2] == 0) {
              now = get_time();
              printf("%d:%d:%d\t", now.h, now.m, now.s);
              printf("%s no quiere jugar otra partida :(\n", nickname1);
              message[0] = 20;
              message[1] = 0;

              send(player1_socket, message, 256, 0);
              send(player2_socket, message, 256, 0);
              break;

            }

            else {
              now = get_time();
              printf("%d:%d:%d\t", now.h, now.m, now.s);
              printf("%s quiere jugar otra partida :D\n", nickname1);

              message[0] = 12;
              message[1] = 0;

              send(player2_socket, message, 256, 0);
              for (int i = 0; i < 256; i++){
                message[i] = 0;
              }

              for (int i = 0; i < 256; i++){
                buffer[i] = 0;
              }
              valread = read( player1_socket , buffer, 256);
              // printf("messageID %d\n", buffer[0] );
              // printf("payloadSize %d\n", buffer[1] );
              if (buffer[0] == 13) {
                if (buffer[2] == 0) {
                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("%s no quiere jugar otra partida :(\n", nickname2);
                  message[0] = 20;
                  message[1] = 0;

                  send(player1_socket, message, 256, 0);
                  send(player2_socket, message, 256, 0);
                  break;
                }
                else {
                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("%s quiere jugar otra partida :D\n", nickname1);

                  messageID = 6;
                  payloadSize = 0;

                  message[0] = messageID;
                  message[1] = payloadSize;

                  send(player1_socket, message, 256, 0);
                  send(player2_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  ronda = 0;
                  message[0] = 7;
                  message[1] = 1;
                  message[2] = ronda;

                  send(player1_socket, message, 256, 0);
                  send(player2_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  score1 = 0, score2 = 0;
                  message[0] = 8;
                  message[1] = 2;
                  message[2] = score1;
                  message[3] = score2;

                  send(player1_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  message[0] = 8;
                  message[1] = 2;
                  message[2] = score2;
                  message[3] = score1;
                  send(player2_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }


                  // Who's First

                  srand(time(NULL));
                  message[0] = 9;
                  message[1] = 1;
                  int r = (rand() % 2 + 1);
                  // int r = 1; // con 1 cliente
                  message[2] = r;
                  player1 = r;
                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("Player 1 ID: %d\n", message[2]);

                  send(player1_socket, message, 256, 0);
                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  message[0] = 9;
                  message[1] = 1;
                  if (r == 1) {
                    message[2] = 2;
                    player2 = 2;
                  }
                  else {
                    message[2] = 1;
                    player2 = 1;
                  }

                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("Player 2 ID: %d\n", message[2]);

                  send(player2_socket, message, 256, 0);
                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  // Boardstate

                  message[0] = 10;
                  message[1] = 9;
                  for (int k = 2; k < 11; k++) {
                    message[k] = tablero[k-2];
                    // printf("%d\n", message[k]);
                  }
                }
              }
            }

          }
        }

        if (termino == 22) {
          message[0] = 16;
          message[1] = 1;
          message[2] = ronda;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 17;
          message[1] = 3;
          message[2] = ronda;
          message[3] = 0;
          message[4] = 2;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 18;
          message[1] = 0;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 19;
          message[1] = 1;
          message[2] = 2;

          send(player1_socket, message, 256, 0);
          send(player2_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          message[0] = 12;
          message[1] = 0;

          send(player1_socket, message, 256, 0);
          for (int i = 0; i < 256; i++){
            message[i] = 0;
          }

          for (int i = 0; i < 256; i++){
            buffer[i] = 0;
          }

          valread = read( player1_socket , buffer, 256);
          // printf("messageID %d\n", buffer[0] );
          // printf("payloadSize %d\n", buffer[1] );
          if (buffer[0] == 13) {
            if (buffer[2] == 0) {
              now = get_time();
              printf("%d:%d:%d\t", now.h, now.m, now.s);
              printf("%s no quiere jugar otra partida :(\n", nickname1);
              message[0] = 20;
              message[1] = 0;

              send(player1_socket, message, 256, 0);
              send(player2_socket, message, 256, 0);
              break;

            }

            else {
              now = get_time();
              printf("%d:%d:%d\t", now.h, now.m, now.s);
              printf("%s quiere jugar otra partida :D\n", nickname1);

              message[0] = 12;
              message[1] = 0;

              send(player2_socket, message, 256, 0);
              for (int i = 0; i < 256; i++){
                message[i] = 0;
              }

              for (int i = 0; i < 256; i++){
                buffer[i] = 0;
              }
              valread = read( player1_socket , buffer, 256);
              // printf("messageID %d\n", buffer[0] );
              // printf("payloadSize %d\n", buffer[1] );
              if (buffer[0] == 13) {
                if (buffer[2] == 0) {
                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("%s no quiere jugar otra partida :(\n", nickname2);
                  message[0] = 20;
                  message[1] = 0;

                  send(player1_socket, message, 256, 0);
                  send(player2_socket, message, 256, 0);
                  break;
                }
                else {
                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("%s quiere jugar otra partida :D\n", nickname1);

                  messageID = 6;
                  payloadSize = 0;

                  message[0] = messageID;
                  message[1] = payloadSize;

                  send(player1_socket, message, 256, 0);
                  send(player2_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  ronda = 0;
                  message[0] = 7;
                  message[1] = 1;
                  message[2] = ronda;

                  send(player1_socket, message, 256, 0);
                  send(player2_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  score1 = 0, score2 = 0;
                  message[0] = 8;
                  message[1] = 2;
                  message[2] = score1;
                  message[3] = score2;

                  send(player1_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  message[0] = 8;
                  message[1] = 2;
                  message[2] = score2;
                  message[3] = score1;
                  send(player2_socket, message, 256, 0);

                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }


                  // Who's First

                  srand(time(NULL));
                  message[0] = 9;
                  message[1] = 1;
                  int r = (rand() % 2 + 1);
                  // int r = 1; // con 1 cliente
                  message[2] = r;
                  player1 = r;
                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("Player 1 ID: %d\n", message[2]);

                  send(player1_socket, message, 256, 0);
                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  message[0] = 9;
                  message[1] = 1;
                  if (r == 1) {
                    message[2] = 2;
                    player2 = 2;
                  }
                  else {
                    message[2] = 1;
                    player2 = 1;
                  }

                  now = get_time();
                  printf("%d:%d:%d\t", now.h, now.m, now.s);
                  printf("Player 2 ID: %d\n", message[2]);

                  send(player2_socket, message, 256, 0);
                  for (int i = 0; i < 256; i++){
                    message[i] = 0;
                  }

                  // Boardstate

                  message[0] = 10;
                  message[1] = 9;
                  for (int k = 2; k < 11; k++) {
                    message[k] = tablero[k-2];
                    // printf("%d\n", message[k]);
                  }
                }
              }
            }

          }
        }



      }



    } // fin del if (count == 2)

    return 0;
    ////

    // bzero((char *) &serv_addr, sizeof(serv_addr));
  }

  else if (argc == 6) {
      // conn logs

      FILE *log  = fopen("logs.txt", "w");

      if (!log)
    	{
    		printf("¡Error abriendo el archivo!\n");
    		return 2;
    	}

      // printf("%s\n", argv[2]);
      printf("%s\n", argv[5]);
      // int sockfd, newsockfd, portno, clilen, n;
      // sockfd = socket(AF_INET, SOCK_STREAM, 0);
      // if (sockfd < 0) {
      //   error("ERROR opening socket");
      // }

      ////
      int server_fd, valread;
      // int clients[2];
      int count = 0;
      struct sockaddr_in address;
      // int opt = 1;
      int addrlen = sizeof(address);
      char buffer[256] = {0};
      int PORT = atoi(argv[4]);

      // Creating socket file descriptor
      if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
          perror("socket failed");
          exit(EXIT_FAILURE);
      }

      //type of socket created
      memset(&address, '0', sizeof(address));
      address.sin_family = AF_INET;
      // Convert IPv4 and IPv6 addresses from text to binary form
      if(inet_pton(AF_INET, argv[2], &address.sin_addr)<=0) {
          printf("\nInvalid address/ Address not supported \n");
          return -1;
      }
      address.sin_addr.s_addr = INADDR_ANY; // localhost address
      address.sin_port = htons( PORT );

      // Forcefully attaching socket to the port 8080
      if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
          perror("bind failed");
          exit(EXIT_FAILURE);
      }
      if (listen(server_fd, 3) < 0) {
          perror("listen");
          exit(EXIT_FAILURE);
      }
      while (count == 0) {
        player1_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if ((player1_socket)<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else {
          // clients[0] = player1_socket;
          count++;

          valread = read( player1_socket , buffer, 256);
          // printf("messageID %x\n", buffer[0] );
          // printf("payloadSize %x\n", buffer[1] );
          if (buffer[0] == 1) {
            Hms now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Connection Requested\n");
            char message[256] = {0};
            unsigned char messageID;
            unsigned char payloadSize;

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            // accept Connection
            messageID = 2;
            payloadSize = 0;

            message[0] = messageID;
            message[1] = payloadSize;

            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Accepting Connection\n");
            send(player1_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            messageID = 3;
            payloadSize = 0;

            message[0] = messageID;
            message[1] = payloadSize;
            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Asking for Nickname\n");
            send(player1_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            valread = read(player1_socket, buffer, 256);
            // printf("messageID %x\n", buffer[0] );
            // printf("payloadSize %x\n", buffer[1] );
            if (buffer[0] == 4) {

              for (int k = 2; k < 256; k++) {
                nickname1[k-2] = buffer[k];
              }
              now = get_time();
              printf("%d:%d:%d\t", now.h, now.m, now.s);
              printf("Player1: %s\n", nickname1);
            }
          }

        }
      }
      while (count == 1) {
        if ((player2_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        else {
          // clients[1] = player2_socket;
          count++;

          valread = read( player2_socket , buffer, 256);
          // printf("messageID %x\n", buffer[0] );
          // printf("payloadSize %x\n", buffer[1] );
          if (buffer[0] == 1) {
            Hms now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Connection Requested\n");
            char message[256] = {0};
            unsigned char messageID;
            unsigned char payloadSize;

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            // accept Connection
            messageID = 2;
            payloadSize = 0;

            message[0] = messageID;
            message[1] = payloadSize;

            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Accepting Connection\n");
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            messageID = 3;
            payloadSize = 0;

            message[0] = messageID;
            message[1] = payloadSize;
            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Asking for Nickname\n");
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            valread = read(player2_socket, buffer, 256);
            // printf("messageID %x\n", buffer[0] );
            // printf("payloadSize %x\n", buffer[1] );
            if (buffer[0] == 4) {

              for (int k = 2; k < 256; k++) {
                nickname2[k-2] = buffer[k];
              }
              now = get_time();
              printf("%d:%d:%d\t", now.h, now.m, now.s);
              printf("Player2: %s\n", nickname2);
            }
          }
        }

      }

      if (count == 2) {
        char message[256] = {0};
        unsigned char messageID;
        unsigned char payloadSize;

        messageID = 5;
        payloadSize = 254;

        message[0] = messageID;
        message[1] = payloadSize;

        for (int k = 2; k < 256; k++) {
          message[k] = nickname2[k - 2];
        }

        send(player1_socket, message, 256, 0);

        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }

        messageID = 5;
        payloadSize = 254;

        message[0] = messageID;
        message[1] = payloadSize;

        for (int k = 2; k < 256; k++) {
          message[k] = nickname1[k - 2];
        }

        send(player2_socket, message, 256, 0);

        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }

        // Game Start

        messageID = 6;
        payloadSize = 0;

        message[0] = messageID;
        message[1] = payloadSize;

        send(player1_socket, message, 256, 0);
        send(player2_socket, message, 256, 0);

        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }

        message[0] = 7;
        message[1] = 1;
        message[2] = ronda;

        send(player1_socket, message, 256, 0);
        send(player2_socket, message, 256, 0);

        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }

        message[0] = 8;
        message[1] = 2;
        message[2] = score1;
        message[3] = score2;

        send(player1_socket, message, 256, 0);

        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }

        message[0] = 8;
        message[1] = 2;
        message[2] = score2;
        message[3] = score1;
        send(player2_socket, message, 256, 0);

        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }


        // Who's First

        srand(time(NULL));
        message[0] = 9;
        message[1] = 1;
        int r = (rand() % 2 + 1);
        // int r = 1; // con 1 cliente
        message[2] = r;
        player1 = r;
        Hms now = get_time();
        printf("%d:%d:%d\t", now.h, now.m, now.s);
        printf("Player 1 ID: %d\n", message[2]);

        send(player1_socket, message, 256, 0);
        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }

        message[0] = 9;
        message[1] = 1;
        if (r == 1) {
          message[2] = 2;
          player2 = 2;
        }
        else {
          message[2] = 1;
          player2 = 1;
        }

        now = get_time();
        printf("%d:%d:%d\t", now.h, now.m, now.s);
        printf("Player 2 ID: %d\n", message[2]);

        send(player2_socket, message, 256, 0);
        for (int i = 0; i < 256; i++){
          message[i] = 0;
        }

        // Boardstate

        message[0] = 10;
        message[1] = 9;
        for (int k = 2; k < 11; k++) {
          message[k] = tablero[k-2];
          // printf("%d\n", message[k]);
        }

        int termino = 0;
        while (termino != 20) {
          if (turno == player1) {
            send(player1_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            termino = rcv(player1_socket, valread, buffer);
          }
          else {
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            termino = rcv(player2_socket, valread, buffer);
          }

          if (termino == 20) {
            break;
          }

          if (termino == 1 || termino == 2) {
            message[0] = 10;
            message[1] = 9;
            for (int k = 2; k < 11; k++) {
              message[k] = tablero[k-2];
              // printf("%d\n", message[k]);
            }
          }

          if (termino == 11) {
            message[0] = 16;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 17;
            message[1] = 3;
            message[2] = ronda;
            message[3] = 0;
            message[4] = 1;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            ronda++;

            message[0] = 7;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 8;
            message[1] = 2;
            message[2] = score1;
            message[3] = score2;

            send(player1_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 8;
            message[1] = 2;
            message[2] = score2;
            message[3] = score1;
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            srand(time(NULL));
            message[0] = 9;
            message[1] = 1;
            int r = (rand() % 2 + 1);
            // int r = 1; // con 1 cliente
            message[2] = r;
            player1 = r;
            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player 1 ID: %d\n", message[2]);

            send(player1_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 9;
            message[1] = 1;
            if (r == 1) {
              message[2] = 2;
              player2 = 2;
            }
            else {
              message[2] = 1;
              player2 = 1;
            }

            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player 2 ID: %d\n", message[2]);

            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 10;
            message[1] = 9;
            for (int k = 2; k < 11; k++) {
              message[k] = tablero[k-2];
              // printf("%d\n", message[k]);
            }
          }

          if (termino == 12) {
            message[0] = 16;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 17;
            message[1] = 3;
            message[2] = ronda;
            message[3] = 0;
            message[4] = 2;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            ronda++;

            message[0] = 7;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 8;
            message[1] = 2;
            message[2] = score1;
            message[3] = score2;

            send(player1_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 8;
            message[1] = 2;
            message[2] = score2;
            message[3] = score1;
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            srand(time(NULL));
            message[0] = 9;
            message[1] = 1;
            int r = (rand() % 2 + 1);
            // int r = 1; // con 1 cliente
            message[2] = r;
            player1 = r;
            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player 1 ID: %d\n", message[2]);

            send(player1_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 9;
            message[1] = 1;
            if (r == 1) {
              message[2] = 2;
              player2 = 2;
            }
            else {
              message[2] = 1;
              player2 = 1;
            }

            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player 2 ID: %d\n", message[2]);

            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 10;
            message[1] = 9;
            for (int k = 2; k < 11; k++) {
              message[k] = tablero[k-2];
              // printf("%d\n", message[k]);
            }
          }

          if (termino == 0) {
            message[0] = 16;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 17;
            message[1] = 2;
            message[2] = ronda;
            message[3] = 1;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            ronda++;

            message[0] = 7;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 8;
            message[1] = 2;
            message[2] = score1;
            message[3] = score2;

            send(player1_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 8;
            message[1] = 2;
            message[2] = score2;
            message[3] = score1;
            send(player2_socket, message, 256, 0);

            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            srand(time(NULL));
            message[0] = 9;
            message[1] = 1;
            int r = (rand() % 2 + 1);
            // int r = 1; // con 1 cliente
            message[2] = r;
            player1 = r;
            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player 1 ID: %d\n", message[2]);

            send(player1_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 9;
            message[1] = 1;
            if (r == 1) {
              message[2] = 2;
              player2 = 2;
            }
            else {
              message[2] = 1;
              player2 = 1;
            }

            now = get_time();
            printf("%d:%d:%d\t", now.h, now.m, now.s);
            printf("Player 2 ID: %d\n", message[2]);

            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 10;
            message[1] = 9;
            for (int k = 2; k < 11; k++) {
              message[k] = tablero[k-2];
              // printf("%d\n", message[k]);
            }
          }

          if (termino == 21) {
            message[0] = 16;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 17;
            message[1] = 3;
            message[2] = ronda;
            message[3] = 0;
            message[4] = 1;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 18;
            message[1] = 0;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 19;
            message[1] = 1;
            message[2] = 1;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 12;
            message[1] = 0;

            send(player1_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            for (int i = 0; i < 256; i++){
              buffer[i] = 0;
            }

            valread = read( player1_socket , buffer, 256);
            // printf("messageID %d\n", buffer[0] );
            // printf("payloadSize %d\n", buffer[1] );
            if (buffer[0] == 13) {
              if (buffer[2] == 0) {
                now = get_time();
                printf("%d:%d:%d\t", now.h, now.m, now.s);
                printf("%s no quiere jugar otra partida :(\n", nickname1);
                message[0] = 20;
                message[1] = 0;

                send(player1_socket, message, 256, 0);
                send(player2_socket, message, 256, 0);
                break;

              }

              else {
                now = get_time();
                printf("%d:%d:%d\t", now.h, now.m, now.s);
                printf("%s quiere jugar otra partida :D\n", nickname1);

                message[0] = 12;
                message[1] = 0;

                send(player2_socket, message, 256, 0);
                for (int i = 0; i < 256; i++){
                  message[i] = 0;
                }

                for (int i = 0; i < 256; i++){
                  buffer[i] = 0;
                }
                valread = read( player1_socket , buffer, 256);
                // printf("messageID %d\n", buffer[0] );
                // printf("payloadSize %d\n", buffer[1] );
                if (buffer[0] == 13) {
                  if (buffer[2] == 0) {
                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("%s no quiere jugar otra partida :(\n", nickname2);
                    message[0] = 20;
                    message[1] = 0;

                    send(player1_socket, message, 256, 0);
                    send(player2_socket, message, 256, 0);
                    break;
                  }
                  else {
                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("%s quiere jugar otra partida :D\n", nickname1);

                    messageID = 6;
                    payloadSize = 0;

                    message[0] = messageID;
                    message[1] = payloadSize;

                    send(player1_socket, message, 256, 0);
                    send(player2_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    ronda = 0;
                    message[0] = 7;
                    message[1] = 1;
                    message[2] = ronda;

                    send(player1_socket, message, 256, 0);
                    send(player2_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    score1 = 0, score2 = 0;
                    message[0] = 8;
                    message[1] = 2;
                    message[2] = score1;
                    message[3] = score2;

                    send(player1_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    message[0] = 8;
                    message[1] = 2;
                    message[2] = score2;
                    message[3] = score1;
                    send(player2_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }


                    // Who's First

                    srand(time(NULL));
                    message[0] = 9;
                    message[1] = 1;
                    int r = (rand() % 2 + 1);
                    // int r = 1; // con 1 cliente
                    message[2] = r;
                    player1 = r;
                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("Player 1 ID: %d\n", message[2]);

                    send(player1_socket, message, 256, 0);
                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    message[0] = 9;
                    message[1] = 1;
                    if (r == 1) {
                      message[2] = 2;
                      player2 = 2;
                    }
                    else {
                      message[2] = 1;
                      player2 = 1;
                    }

                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("Player 2 ID: %d\n", message[2]);

                    send(player2_socket, message, 256, 0);
                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    // Boardstate

                    message[0] = 10;
                    message[1] = 9;
                    for (int k = 2; k < 11; k++) {
                      message[k] = tablero[k-2];
                      // printf("%d\n", message[k]);
                    }
                  }
                }
              }

            }
          }

          if (termino == 22) {
            message[0] = 16;
            message[1] = 1;
            message[2] = ronda;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 17;
            message[1] = 3;
            message[2] = ronda;
            message[3] = 0;
            message[4] = 2;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 18;
            message[1] = 0;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 19;
            message[1] = 1;
            message[2] = 2;

            send(player1_socket, message, 256, 0);
            send(player2_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            message[0] = 12;
            message[1] = 0;

            send(player1_socket, message, 256, 0);
            for (int i = 0; i < 256; i++){
              message[i] = 0;
            }

            for (int i = 0; i < 256; i++){
              buffer[i] = 0;
            }

            valread = read( player1_socket , buffer, 256);
            // printf("messageID %d\n", buffer[0] );
            // printf("payloadSize %d\n", buffer[1] );
            if (buffer[0] == 13) {
              if (buffer[2] == 0) {
                now = get_time();
                printf("%d:%d:%d\t", now.h, now.m, now.s);
                printf("%s no quiere jugar otra partida :(\n", nickname1);
                message[0] = 20;
                message[1] = 0;

                send(player1_socket, message, 256, 0);
                send(player2_socket, message, 256, 0);
                break;

              }

              else {
                now = get_time();
                printf("%d:%d:%d\t", now.h, now.m, now.s);
                printf("%s quiere jugar otra partida :D\n", nickname1);

                message[0] = 12;
                message[1] = 0;

                send(player2_socket, message, 256, 0);
                for (int i = 0; i < 256; i++){
                  message[i] = 0;
                }

                for (int i = 0; i < 256; i++){
                  buffer[i] = 0;
                }
                valread = read( player1_socket , buffer, 256);
                // printf("messageID %d\n", buffer[0] );
                // printf("payloadSize %d\n", buffer[1] );
                if (buffer[0] == 13) {
                  if (buffer[2] == 0) {
                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("%s no quiere jugar otra partida :(\n", nickname2);
                    message[0] = 20;
                    message[1] = 0;

                    send(player1_socket, message, 256, 0);
                    send(player2_socket, message, 256, 0);
                    break;
                  }
                  else {
                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("%s quiere jugar otra partida :D\n", nickname1);

                    messageID = 6;
                    payloadSize = 0;

                    message[0] = messageID;
                    message[1] = payloadSize;

                    send(player1_socket, message, 256, 0);
                    send(player2_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    ronda = 0;
                    message[0] = 7;
                    message[1] = 1;
                    message[2] = ronda;

                    send(player1_socket, message, 256, 0);
                    send(player2_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    score1 = 0, score2 = 0;
                    message[0] = 8;
                    message[1] = 2;
                    message[2] = score1;
                    message[3] = score2;

                    send(player1_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    message[0] = 8;
                    message[1] = 2;
                    message[2] = score2;
                    message[3] = score1;
                    send(player2_socket, message, 256, 0);

                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }


                    // Who's First

                    srand(time(NULL));
                    message[0] = 9;
                    message[1] = 1;
                    int r = (rand() % 2 + 1);
                    // int r = 1; // con 1 cliente
                    message[2] = r;
                    player1 = r;
                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("Player 1 ID: %d\n", message[2]);

                    send(player1_socket, message, 256, 0);
                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    message[0] = 9;
                    message[1] = 1;
                    if (r == 1) {
                      message[2] = 2;
                      player2 = 2;
                    }
                    else {
                      message[2] = 1;
                      player2 = 1;
                    }

                    now = get_time();
                    printf("%d:%d:%d\t", now.h, now.m, now.s);
                    printf("Player 2 ID: %d\n", message[2]);

                    send(player2_socket, message, 256, 0);
                    for (int i = 0; i < 256; i++){
                      message[i] = 0;
                    }

                    // Boardstate

                    message[0] = 10;
                    message[1] = 9;
                    for (int k = 2; k < 11; k++) {
                      message[k] = tablero[k-2];
                      // printf("%d\n", message[k]);
                    }
                  }
                }
              }

            }
          }



        }



      } // fin del if (count == 2)

      fclose(log);
  }
  return 0;
}
