#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// checksum
unsigned char calcular_hash(FILE *f)
{
    unsigned char hash = 0;
    unsigned char buffer[4096];

    size_t lidos;
    size_t i;

    // posiciona no início do arquivo
    fseek(f, 0, SEEK_SET);


    while ((lidos = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        for (i = 0; i < lidos; i++)
        {
            hash ^= buffer[i]; // operação XOR
        }
    }

    // volta o ponteiro para o início
    fseek(f, 0, SEEK_SET);

    return hash;
}


void error(const char *msg)
{
    perror(msg);
    exit(0);
}


void enviando_viaTCP(FILE *f, unsigned char hash, char *hostname, int portno)
{
    int sockfd, n;

    /* EXEMPLO DA ESTRUTURA - endereço do servidor
     * struct sockaddr_in
     * {
     * 	short sin_family; // deve ser AF_INET
     * 	u_short sin_port;
     * 	struct in_addr sin_addr; // contem o IP do host
     * 	char sin_zero[8]; // N?o usado, deve ser zero
     * 	};
    */
    struct sockaddr_in serv_addr;


    /* EXEMPLO DA ESTRUTURA que armazena informações do host
     *  struct hostent
     *  {
     *  	char *h_name; // nome oficial do host
     *  	char **h_aliases; // lista de aliases
     *  	int h_addrtype; // tipo de endereco do host
     *  	int h_length; // comprimento do endereco
     *  	char **h_addr_list; // lista de enderecos do servidor de nomes
     *  	# define h_addr h_addr_list[0] // endereco, para compatibilidade com vers?es anteriores
     *  }
    */
    struct hostent *server;

    // buffer para envio dos dados do arquivo
    unsigned char buffer[4096];

    /*
        socket(dominio_enderecamento, tipo_socket, protocolo)

        dominio_enderecamento:
            AF_INET  : Internet (IPv4)
            AF_UNIX  : comunicação local

        tipo_socket:
            SOCK_STREAM : TCP
            SOCK_DGRAM  : UDP

        protocolo:
            0 → escolha automática (normalmente TCP para STREAM)
    */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERRO ao abrir socket");

    /*
        gethostbyname:
        Recebe o hostname e retorna os dados do servidor
    */
    server = gethostbyname(hostname);

    if (server == NULL)
    {
        fprintf(stderr, "ERRO, host inexistente\n");
        exit(0);
    }

    // zera a struct
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    // copiando endereço IP retornado pelo DNS
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    // conversão da porta para padrão de rede (big-endian)
    serv_addr.sin_port = htons(portno);

    // estabelece conexão com o servidor
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERRO no connect");


    size_t lidos;

    while ((lidos = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        n = write(sockfd, buffer, lidos);
        printf("%u\n", n);

        if (n < 0)
            error("ERRO ao enviar arquivo");
    }



    // Hash é 1 byte -> enviar endereço + tamanho = 1
    n = write(sockfd, &hash, 1);

    if (n < 0)
        error("ERRO ao enviar hash");

    /*
        ===== RECEBENDO RESPOSTA =====
    */
    printf("RECEBENDO RESPOSTA \n");
    char resposta[256];
    bzero(resposta, 256);

    n = read(sockfd, resposta, 255);

    if (n < 0)
        error("ERRO ao ler resposta");

    printf("Servidor: %s\n", resposta);

    close(sockfd);
}

int main(int argc, char *argv[])
{
    /*
        argumentos:
        argv[1] : hostname
        argv[2] : porta
        argv[3] : arquivo
    */
    if (argc < 4)
    {
        fprintf(stderr, "Uso: %s hostname porta arquivo\n", argv[0]);
        exit(0);
    }

    FILE *f = fopen(argv[3],"rb");

    if (!f)
        error("ERRO ao abrir arquivo");

    unsigned char hash = calcular_hash(f);

    printf("Hash = %u\n", hash);

    enviando_viaTCP(f, hash, argv[1], atoi(argv[2]));

    fclose(f);

    return 0;
}
