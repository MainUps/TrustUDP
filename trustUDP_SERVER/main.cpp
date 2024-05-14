#include "..\trustUDP.h"
#include <map>

#define SERVERPORT 9000
#define BUFSIZE    512

int main(int argc, char* argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// ������ ��ſ� ����� ����
	struct sockaddr_in clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	std::map<std::string, int>ack_map; //�� ������ ������ map

	// Ŭ���̾�Ʈ�� ������ ���
	while (1) {
		// ������ �ޱ�
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr*)&clientaddr, &addrlen);

		// üũ�� ����
		char received_checksum_str[3] = { buf[retval - 2], buf[retval - 1], '\0' };
		buf[retval - 2] = '\0';  // üũ���� ������ ������ ������

		// üũ�� ���
		unsigned int checksum = 0;
		for (int i = 0; i < strlen(buf); i++) {
			checksum += buf[i];
		}
		checksum = checksum & 0xFF;  // üũ���� 1����Ʈ ũ��� �߶�

		// üũ���� ���ڿ��� ��ȯ
		char checksum_str[3];
		sprintf(checksum_str, "%02X", checksum);

		// üũ�� ����
		if (strcmp(checksum_str, received_checksum_str) != 0) {
			printf("[����] �߸��� üũ���Դϴ�!\n");
			break;
		}
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// ���� ������ ���
		buf[retval] = '\0';
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("[UDP/%s:%d] %s", addr, ntohs(clientaddr.sin_port), buf);

		//ACK �޾Ƽ� ����
		int ack;
		sscanf(buf, "%d", &ack);
		std::string key = std::string(addr) + ":" + std::to_string(ntohs(clientaddr.sin_port));
		if (ack_map.find(key) == ack_map.end()) {
			ack_map[key] = -1;  // ó�� ���� Ŭ���̾�Ʈ��� ack_map�� 
		}						// �׸��� �߰��ϰ� ���� -1�� ����
		if (ack_map[key] != ack - 1) {
			printf("[����] �߸��� ACK ���Դϴ�!\n");
			break;
		}
		else {
			printf("  [ack number ��ġ]");
		}
		printf("     [üũ�� ��ġ : %s]\n", checksum_str);
		ack_map[key] = ack;

		// ������ ������
		retval = sendto(sock, buf, retval, 0,
			(struct sockaddr*)&clientaddr, sizeof(clientaddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
	}
	// ���� �ݱ�
	closesocket(sock);

	// ���� ����
	WSACleanup();

	return 0;
}
