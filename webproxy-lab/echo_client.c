#include "csapp.h"

// 표준입력 한 줄씩 서버에 보내고 응답을 그대로 출력하는 에코 클라이언트 진입점
// argc: 프로그램 이름을 포함한 명령행 인자 개수
// argv: argv[1]은 서버 호스트 이름, argv[2]는 숫자 포트 문자열
// 반환: 정상 종료 시 0

// lower-case open_clientfd로 연결 실패 코드를 직접 확인하고
// Rio_readinitb로 읽기 상태를 소켓에 초기화한 뒤
// Rio_writen과 Rio_readlineb로 요청과 응답을 반복하는 흐름
int main(int argc, char **argv)
{
    int clientfd; // 서버와 연결된 TCP 소켓 디스크립터

    char *host; // 서버 이름 또는 IP 문자열
    char *port; // AI_NUMERICSERV 전제를 만족하는 숫자 포트 문자열
    char buf[MAXLINE]; // 입력 한 줄과 응답 한 줄을 번갈아 담는 버퍼

    rio_t rio; // RIO 상태 구조체 변수 (목적: 서버 소켓에서 줄 단위 응답을 읽기 위함)

    /* [rio_t 상태]
     rio_fd: 읽을 대상 fd
     rio_cnt: 내부 버퍼에 아직 남아 있는 바이트 수
     rio_bufptr: 다음에 읽을 위치
     rio_buf[8192]: 내부 버퍼
    */

    if (argc != 3) { // 프로그램 이름 뒤에 host와 port 두 인자가 정확히 와야 함
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    // 명령행 인자를 연결 함수가 바로 쓰는 이름으로 보관
    host = argv[1];
    port = argv[2];

    clientfd = open_clientfd(host, port); // open_clientfd: 성공시 소켓의 fd 값, getaddrinfo 실패 시 -2, 그 밖의 연결 실패 시 -1 반환
    if (clientfd < 0) {
        fprintf(stderr, "연결 오류! %s:%s에 연결할 수 없습니다. \n오류 코드  %d\n\n", host, port, clientfd);
        exit(0);
    }

    // 소켓을 RIO 상태에 연결하고 내부 버퍼를 빈 상태로 초기화
    Rio_readinitb(&rio, clientfd);

    // 입력 한 줄을 읽어 전송하고 응답 한 줄을 받아 출력하는 반복
    while (fgets(buf, MAXLINE, stdin) != NULL) {
        // 부분 전송이나 EINTR(오류 코드)가 나와도 요청한 길이만큼 다시 쓰기 시도
        /* EINTR이란?
                유닉스 계열 운영체제에서 시스템 콜(System Call)이 실행되는 도중
                시그널(Signal)에 의해 방해를 받아 중단되었음을 알리는 오류 코드 */
        Rio_writen(clientfd, buf, strlen(buf)); //사용자가 작성한 한 줄을 서버로 전송

        // (maxlen-1)바이트까지만 읽고 끝에 NUL(\0)을 붙이는 줄 읽기
        // 이 예제는 반환값을 검사하지 않아 EOF나 긴 줄이 끊겨 들어온 경우를 별도 처리하지 않음
        Rio_readlineb(&rio, buf, MAXLINE); // 응답받은 값 수신
        
        Fputs(buf, stdout); // 수신한 한 줄 출력
    }

    Close(clientfd); // 소켓 연결 종료
    return 0;
}
