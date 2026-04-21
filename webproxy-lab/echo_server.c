#include "csapp.h"

// 연결된 클라이언트 한 개와 줄 단위 에코를 처리하는 함수
// connfd: accept가 반환한 연결 전용 소켓 디스크립터
// 반환: 없음
// 부작용: 읽은 텍스트 줄을 같은 소켓으로 다시 전송
void echo (int connfd);

// 지정한 포트에 리스닝 소켓을 열고
// 연결을 순차적으로 받아 에코 처리하는 서버 진입점
// argc: 프로그램 이름을 포함한 명령행 인자 개수
// argv: argv[1]은 open_listenfd가 해석할 숫자 포트 문자열
// 종료: 인자 개수가 다르면 사용법을 출력하고 즉시 종료
int main (int argc, char **argv){
    int listenfd, connfd;
    // listenfd: 리스닝 소켓 fd
    // connfd: 새 연결마다 받아오는 소켓 fd

    // accept 호출 전 clientaddr 크기를 넣어 두고
    // accept가 실제로 기록한 주소 길이를 Getnameinfo에 그대로 넘길 때도 사용
    socklen_t clientlen;

    // IPv4와 IPv6를 모두 담을 수 있는 클라이언트 주소 저장소
    struct sockaddr_storage clientaddr;

    // 사람이 읽을 수 있는 호스트 이름과 포트 문자열 버퍼
    char client_hostname[MAXLINE], client_port[MAXLINE];
    
    // 프로그램 이름 뒤에 포트 인자 하나만 받음
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // open_listenfd는 실패 시 -2(getaddrinfo), -1(그 외) 같은 음수 코드를 반환
    listenfd = open_listenfd(argv[1]);
    if (listenfd < 0)
	    unix_error("Open_listenfd error");

    // 한 번에 하나의 연결만 처리하는 순차 서버
    while (1) {
        // accept에 넘길 주소 버퍼 크기를 매 반복마다 현재 구조체 크기로 재설정
        clientlen = sizeof(clientaddr);
        
        // 새 연결을 수락하고 상대 주소를 clientaddr에 채움
        connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
        if (connfd < 0)
            unix_error("Accept error");
        
        // Getnameinfo는 실패 시 에러 처리로 종료하므로
        // 여기까지 오면 사람이 읽을 수 있는 호스트와 포트 문자열 확보
        Getnameinfo((SA*)&clientaddr, clientlen, 
                    client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
        
        // 접속 로그를 남기고 해당 연결에 대한 에코 처리 수행
        printf("Connected to (%s, %s)\n", client_hostname, client_port);

        echo(connfd);

        // 현재 연결이 끝나면 연결 전용 소켓만 닫고 다음 접속 대기
        Close(connfd);
        
        // 연결 종료 알리기
        printf("Disconnected to (%s, %s)\n", client_hostname, client_port);
    }
}

// connfd에 연결된 클라이언트에서 한 줄씩 읽고
// 읽은 바이트 수만큼 같은 소켓으로 다시 써서 에코 수행
// connfd: 이미 연결이 성립한 소켓 fd
// 반환: 없음
// 종료: Rio_readlineb가 EOF에서 0을 반환하면 반복 종료
void echo (int connfd) {
    // 마지막으로 읽은 한 줄의 바이트 수
    size_t n;

    // 클라이언트에서 읽은 텍스트 한 줄 버퍼
    char buf[MAXLINE];

    // connfd용 버퍼드 읽기 상태
    rio_t rio;

    // Rio_readlineb를 쓰기 전에 connfd에 대한 RIO 상태 초기화
    rio_readinitb(&rio, connfd);

    // 줄 단위로 읽고 같은 길이만큼 그대로 전송
    while ((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
        // 수신 바이트 수를 확인하는 디버그 로그
        printf("server received %d bytes (\"%s\") \n", (int)n, buf);

        // 부분 쓰기를 내부에서 보정하며 읽은 길이 전체 전송
        Rio_writen(connfd, buf, n);
    }
}
