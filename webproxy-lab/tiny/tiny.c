/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 * GET 요청만 처리하는 반복형 교육용 서버 예제
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t* rp);
int parse_uri(char* uri, char* filename, char* cgiargs);
void serve_static(int fd, char* filename, int filesize);
void get_filetype(char* filename, char* filetype);
void serve_dynamic(int fd, char* filename, char* cgiargs);
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg);

/* 서버 소켓을 열고 들어오는 연결을 하나씩 순차 처리하는 진입점
 * argc: 프로그램 이름과 포트 문자열을 포함한 명령행 인자 개수
 * argv: argv[1]은 Open_listenfd에 넘길 숫자 포트 문자열
 * 반환: 정상 종료 경로 없음. 인자 오류 시 즉시 종료
 */
int main(int argc, char** argv) {
	int listenfd, connfd; // 대기 소켓과 현재 연결 소켓 디스크립터
	char hostname[MAXLINE], port[MAXLINE]; // 접속 클라이언트 주소를 문자열로 풀어쓴 결과
	socklen_t clientlen; // Accept에 넘길 주소 구조체 크기
	struct sockaddr_storage clientaddr; // IPv4와 IPv6를 모두 담을 수 있는 클라이언트 주소 버퍼

	/* Check command line args */
	if (argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	// 지정한 포트에서 연결을 받을 대기 소켓 생성
	listenfd = Open_listenfd(argv[1]);
	while (1) {
		// 반복형 서버라서 현재 연결을 끝낼 때까지 다음 요청은 기다림
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen);  
		Getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
		printf("Accepted connection from (%s, %s)\n", hostname, port);
		doit(connfd);	
		Close(connfd);
	}
}

/* 요청 한 건을 읽고 정적 파일 전송 또는 CGI 실행으로 응답을 마무리
 * fd: 현재 클라이언트와 연결된 소켓 디스크립터
 * 반환: 없음
 */
void doit(int fd) {
  int is_static; // 1이면 정적 파일, 0이면 CGI 프로그램 실행 대상
  struct stat sbuf; // 요청 대상 파일의 종류, 권한, 크기 정보
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; // 요청라인 원문과 메서드, URI, 버전 파싱 결과
  char filename[MAXLINE], cgiargs[MAXLINE]; // 로컬 파일 경로와 QUERY_STRING 값
  rio_t rio; // 줄 단위 요청 읽기를 위한 RIO 상태

  // 소켓을 내부 버퍼 기반 입력 스트림처럼 다루고 첫 요청라인을 읽기
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);

  printf("Request headers:\n");
  printf("%s", buf);

  // "GET /path HTTP/1.0" 형태의 요청라인을 세 토큰으로 분리
  // version은 파싱만 하고 현재 구현에서는 별도 분기 조건으로 쓰지 않음
  sscanf(buf, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET")) {
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }

  // 헤더 내용은 해석하지 않고 빈 줄이 나올 때까지 입력만 비움
  read_requesthdrs(&rio);

  // URI를 로컬 파일 경로로 바꾸고 정적/동적 요청 여부 판단
  is_static = parse_uri(uri, filename, cgiargs);
  
  // 실제 파일 메타데이터를 읽을 수 있어야 이후 분기 처리가 가능
  if(stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", "Tiny couldn’t find this file");
    return;
  }

  if (is_static) {
    // 정적 응답은 일반 파일이면서 소유자 읽기 권한이 있어야 함
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403","Forbidden", "Tiny couldn’t read the file");
      return;
    }
    serve_static(fd, filename, sbuf.st_size);
  }

  else {
	  // 현재 예제는 CGI도 실행 비트 대신 S_ISREG와 S_IRUSR만 확인하는 단순화된 검사 사용
	  if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
		  clienterror(fd, filename, "403", "Forbidden", "Tiny couldn’t run the CGI program");
		  return;
	  }
    serve_dynamic(fd, filename, cgiargs);
  }
}

/* 오류 상태 코드와 HTML 본문을 만들어 클라이언트에 즉시 전송
 * fd: 오류 응답을 보낼 연결 소켓 디스크립터
 * cause: 실패 원인으로 본문에 삽입할 문자열
 * errnum: HTTP 상태 코드 문자열
 * shortmsg: 상태 코드 옆에 붙일 짧은 오류 이름
 * longmsg: 본문에 넣을 상세 설명 앞부분
 * 반환: 없음
 */
void clienterror(int fd, char* cause, char* errnum, char* shortmsg, char* longmsg) {
  char buf[MAXLINE], body[MAXBUF]; // 응답 헤더 한 줄과 HTML 오류 본문 버퍼

  // 브라우저가 그대로 렌더링할 간단한 HTML 오류 페이지 조립
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // 상태줄과 필수 헤더를 먼저 보내고 이어서 본문 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

/* 요청라인 뒤에 이어지는 헤더를 빈 줄이 나올 때까지 읽음
 * 헤더 값은 의미 해석 없이 소비하고 현재 구현의 요청 처리 분기에는 쓰지 않음
 * rp: 이미 소켓과 연결된 RIO 입력 상태
 * 반환: 없음
 */
void read_requesthdrs(rio_t* rp) {
	char buf[MAXLINE]; // 헤더 한 줄씩 읽어 담는 임시 버퍼

	// HTTP 헤더 끝은 빈 줄 한 줄로 표시되므로 "\r\n"까지 반복
	Rio_readlineb(rp, buf, MAXLINE);
	while (strcmp(buf, "\r\n")) {
		Rio_readlineb(rp, buf, MAXLINE);
		printf("%s", buf);
	}
	return;
}

/* URI를 파일 경로와 CGI 인자로 분해하고 정적/동적 요청 여부를 반환
 * uri: 요청 URI 문자열. 동적 요청이면 '?' 위치를 '\0'로 바꿔 경로와 인자를 분리
 * filename: 현재 작업 디렉터리 기준 로컬 파일 경로 출력 버퍼
 * cgiargs: CGI 프로그램에 QUERY_STRING으로 넘길 문자열 출력 버퍼
 * 반환: 정적 콘텐츠면 1, 동적 콘텐츠면 0
 */
int parse_uri(char* uri, char* filename, char* cgiargs) {
  char* ptr; // 동적 URI에서 쿼리 문자열이 시작되는 '?' 위치

  // cgi-bin이 없으면 정적 파일 요청으로 보고 로컬 경로만 구성
  if (!strstr(uri, "cgi-bin")) {
	  strcpy(cgiargs, "");
	  strcpy(filename, ".");
	  strcat(filename, uri);

	  // 디렉터리 요청은 기본 문서 home.html로 연결
	  if (uri[strlen(uri) - 1] == '/') {
		  strcat(filename, "home.html");
	  }

    return 1;
  }

  // 동적 요청은 '?' 뒤 문자열을 QUERY_STRING으로 떼어내고 앞부분만 실행 파일 경로로 사용
  ptr = strchr(uri, '?');
  if (ptr != NULL) {
	  strcpy(cgiargs, ptr + 1);
	  // execve에 넘길 경로 문자열이 끝나도록 '?' 자리에 널 문자를 기록
	  *ptr = '\0';
  } else {
	  strcpy(cgiargs, "");
  }

  strcpy(filename, ".");
  strcat(filename, uri);
  return 0;
}

/* 정적 파일에 대한 HTTP 응답 헤더를 만들고 파일 내용을 그대로 전송
 * fd: 응답을 보낼 연결 소켓 디스크립터
 * filename: 전송할 로컬 파일 경로
 * filesize: stat으로 얻은 파일 크기 바이트 수
 * 반환: 없음
 */
void serve_static(int fd, char* filename, int filesize) {
  int srcfd; // 정적 파일을 읽기 전용으로 연 파일 디스크립터
  char* srcp; // mmap이 돌려준 파일 내용 시작 주소
  char filetype[MAXLINE], buf[MAXLINE]; // MIME 타입 문자열과 응답 헤더 버퍼
  int len = 0; // buf에 현재까지 채운 헤더 길이

  get_filetype(filename, filetype);

  // 헤더 한 줄씩 이어 붙이며 최종 Content-Length와 Content-Type 기록
  len += snprintf(buf+len, MAXBUF-len, "HTTP/1.0 200 OK\r\n");
  len += snprintf(buf+len, MAXBUF-len, "Server: Tiny Web Server\r\n");
  len += snprintf(buf+len, MAXBUF-len, "Connection: close\r\n");
  len += snprintf(buf+len, MAXBUF-len, "Content-length: %d\r\n", filesize);
  len += snprintf(buf+len, MAXBUF-len, "Content-type: %s\r\n\r\n", filetype);

  Rio_writen(fd, buf, len);

  // 파일을 메모리에 매핑해 한 번의 쓰기 호출로 본문 전송
  srcfd = Open(filename, O_RDONLY, 0);
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  Close(srcfd);

  Rio_writen(fd, srcp, filesize);
  Munmap(srcp, filesize);
}

/* CGI 프로그램을 자식 프로세스로 실행해 동적 응답 본문을 생성
 * fd: 응답을 보내는 연결 소켓 디스크립터
 * filename: execve로 실행할 CGI 프로그램 경로
 * cgiargs: QUERY_STRING 환경 변수에 넣을 인자 문자열
 * 반환: 없음
 */
void serve_dynamic(int fd, char* filename, char* cgiargs) {
	char buf[MAXLINE], *emptylist[] = {NULL}; // 상태줄 헤더 버퍼와 빈 argv 목록
	int len = 0; // buf에 기록한 헤더 길이

	// CGI 프로그램이 나머지 헤더와 본문을 직접 출력함
	// 여기서는 상태줄과 서버 헤더만 먼저 전송
	len += snprintf(buf + len, MAXLINE - len, "HTTP/1.0 200 OK\r\n");
	len += snprintf(buf + len, MAXLINE - len, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, len);

	if (Fork() == 0) {
		// CGI 프로그램이 getenv("QUERY_STRING")로 읽을 값을 자식 환경에 기록
		setenv("QUERY_STRING", cgiargs, 1);
		// CGI 표준출력을 소켓으로 연결해 printf 결과가 바로 클라이언트로 가도록 구성
		Dup2(fd, STDOUT_FILENO);
		Execve(filename, emptylist, environ);
	}

	// 반복형 서버라서 자식 CGI가 끝날 때까지 부모가 대기
	Wait(NULL);
}

/* 파일 이름 접미사를 보고 응답 Content-Type 문자열을 결정
 * filename: MIME 타입을 추론할 로컬 파일 경로
 * filetype: 선택한 MIME 타입 문자열 출력 버퍼
 * 반환: 없음
 */
void get_filetype(char* filename, char* filetype) {
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else
		strcpy(filetype, "text/plain");
}
