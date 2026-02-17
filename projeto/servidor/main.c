#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

unsigned char calcular_hash(unsigned char *buffer, size_t tamanho)
{
    unsigned char hash = 0;

    for (size_t i = 0; i < tamanho; i++)
    {
        hash ^= buffer[i];   // XOR
    }

    return hash;
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;

    unsigned char buffer[4096];
    unsigned char hashRecebido;

    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2)
    {
        fprintf(stderr, "ERRO, nenhuma porta fornecida\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERRO ao abrir o socket");

    bzero((char *)&serv_addr, sizeof(serv_addr));

    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERRO ao vincular");

    listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

    if (newsockfd < 0)
        error("ERRO ao aceitar");


    bzero(buffer, 4096);

    int bytesArquivo = read(newsockfd, buffer, 4096);
    if (bytesArquivo < 0)
        error("ERRO ao ler do socket");

    // indo calcular o hash desse buffer/arquivo
    unsigned char hash;
    int bytesHash = read(newsockfd, &hash, 1);

    if (bytesHash <= 0)
        error("ERRO ao ler hash");

    hashRecebido = hash;
    unsigned char hashCalc = calcular_hash(buffer, sizeof(buffer));

    if (hashCalc != hashRecebido)
    {
        write(newsockfd, "Arquivo corrompido", 18);
        error("Arquivo corrompido");
    }

    FILE *f = fopen("arquivo.txt", "wb");

    if (!f)
        error("Erro ao criar arquivo");

    fwrite(buffer, 1, bytesArquivo, f);

    fclose(f);

    write(newsockfd, "Salvei seu arquivo", 18);


    close(sockfd);
    close(newsockfd);

    return 0;
}
