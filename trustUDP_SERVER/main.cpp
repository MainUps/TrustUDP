#include "..\trustUDP.h"
#include <map>

#define SERVERPORT 9000
#define BUFSIZE    512

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
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

	// 데이터 통신에 사용할 변수
	struct sockaddr_in clientaddr;
	int addrlen;
	char buf[BUFSIZE + 1];

	std::map<std::string, int>ack_map; //각 세션을 저장할 map

	// 클라이언트와 데이터 통신
	while (1) {
		// 데이터 받기
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr*)&clientaddr, &addrlen);

		// 체크섬 추출
		char received_checksum_str[3] = { buf[retval - 2], buf[retval - 1], '\0' };
		buf[retval - 2] = '\0';  // 체크섬을 제외한 나머지 데이터

		// 체크섬 계산
		unsigned int checksum = 0;
		for (int i = 0; i < strlen(buf); i++) {
			checksum += buf[i];
		}
		checksum = checksum & 0xFF;  // 체크섬을 1바이트 크기로 잘라냄

		// 체크섬을 문자열로 변환
		char checksum_str[3];
		sprintf(checksum_str, "%02X", checksum);

		// 체크섬 검증
		if (strcmp(checksum_str, received_checksum_str) != 0) {
			printf("[오류] 잘못된 체크섬입니다!\n");
			break;
		}
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("[UDP/%s:%d] %s", addr, ntohs(clientaddr.sin_port), buf);

		//ACK 받아서 검증
		int ack;
		sscanf(buf, "%d", &ack);
		std::string key = std::string(addr) + ":" + std::to_string(ntohs(clientaddr.sin_port));
		if (ack_map.find(key) == ack_map.end()) {
			ack_map[key] = -1;  // 처음 보는 클라이언트라면 ack_map에 
		}						// 항목을 추가하고 값을 -1로 설정
		if (ack_map[key] != ack - 1) {
			printf("[오류] 잘못된 ACK 값입니다!\n");
			break;
		}
		else {
			printf("  [ack number 일치]");
		}
		printf("     [체크섬 일치 : %s]\n", checksum_str);
		ack_map[key] = ack;

		// 데이터 보내기
		retval = sendto(sock, buf, retval, 0,
			(struct sockaddr*)&clientaddr, sizeof(clientaddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
	}
	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();

	return 0;
}
