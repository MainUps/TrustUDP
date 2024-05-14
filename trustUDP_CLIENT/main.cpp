#include "..\trustUDP.h"

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

int main(int argc, char* argv[])
{
	int retval;

	// ����� �μ��� ������ IP �ּҷ� ���
	if (argc > 1) SERVERIP = argv[1];

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// ���� �ּ� ����ü �ʱ�ȭ
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	// ������ ��ſ� ����� ����
	struct sockaddr_in peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;
	int ack = 0; //ack��ȣ


	// ������ ������ ���
	while (1) {
		// ������ �Է�
		printf("\n[���� ������] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' ���� ����
		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;


		char temp[BUFSIZE + 1];
		sprintf(temp, "%d %s", ack, buf);
		strcpy(buf, temp);


		// üũ�� ���
		unsigned int checksum = 0;
		for (int i = 0; i < strlen(buf); i++) {
			checksum += buf[i];
		}
		checksum = checksum & 0xFF;  // üũ���� 1����Ʈ ũ��� �߶�

		// üũ���� ���ڿ��� ��ȯ
		char checksum_str[3];
		sprintf(checksum_str, "%02X", checksum);

		// üũ���� ������ �ڿ� �߰�
		strcat(buf, checksum_str);

		printf("üũ��: %s\n", checksum_str); // üũ�� ���

		retval = sendto(sock, buf, (int)strlen(buf), 0,
			(struct sockaddr*)&serveraddr, sizeof(serveraddr));

		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
		printf("ack number : %d\n", ack);


		// ������ �ޱ�
		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// �۽����� �ּ� üũ
		if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
			printf("[����] �߸��� �������Դϴ�!\n");
			break;
		}

		//ACK Ȯ�ΰ� ����(++)
		int received_ack;
		sscanf(buf, "%d", &received_ack);
		if (received_ack != ack) {
			printf("[����] �߸��� ACK�Դϴ�!\n");
			break;
		}
		ack++;

	}

	// ���� �ݱ�
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}
