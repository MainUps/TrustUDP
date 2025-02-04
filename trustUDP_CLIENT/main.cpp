#include "..\trustUDP.h"

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

int main(int argc, char* argv[])
{
	int retval;

	// 명령행 인수가 있으면 IP 주소로 사용
	if (argc > 1) SERVERIP = argv[1];

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// 소켓 주소 구조체 초기화
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	// 데이터 통신에 사용할 변수
	struct sockaddr_in peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	int len;
	int ack = 0; //ack신호


	// 서버와 데이터 통신
	while (1) {
		// 데이터 입력
		printf("\n[보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;


		char temp[BUFSIZE + 1];
		sprintf(temp, "%d %s", ack, buf);
		strcpy(buf, temp);


		// 체크섬 계산
		unsigned int checksum = 0;
		for (int i = 0; i < strlen(buf); i++) {
			checksum += buf[i];
		}
		checksum = checksum & 0xFF;  // 체크섬을 1바이트 크기로 잘라냄

		// 체크섬을 문자열로 변환
		char checksum_str[3];
		sprintf(checksum_str, "%02X", checksum);

		// 체크섬을 데이터 뒤에 추가
		strcat(buf, checksum_str);

		printf("체크섬: %s\n", checksum_str); // 체크섬 출력

		retval = sendto(sock, buf, (int)strlen(buf), 0,
			(struct sockaddr*)&serveraddr, sizeof(serveraddr));

		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
		printf("ack number : %d\n", ack);


		// 데이터 받기
		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr*)&peeraddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 송신자의 주소 체크
		if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
			printf("[오류] 잘못된 데이터입니다!\n");
			break;
		}

		//ACK 확인과 증가(++)
		int received_ack;
		sscanf(buf, "%d", &received_ack);
		if (received_ack != ack) {
			printf("[오류] 잘못된 ACK입니다!\n");
			break;
		}
		ack++;

	}

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}
