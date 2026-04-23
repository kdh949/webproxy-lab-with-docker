/*
 * adder.c - a minimal CGI program that adds two numbers together
 * Tiny가 실행하는 교육용 CGI 예제
 */
/* $begin adder */
#include "../csapp.h"

/* QUERY_STRING에서 두 수를 꺼내 합을 계산하고 HTML 응답을 표준출력으로 생성
 * 전제: 호출한 웹 서버가 표준출력을 클라이언트 소켓에 연결한 상태
 * 전제: QUERY_STRING이 '&'로 나뉜 두 토큰이며 각 토큰이 '=' 뒤 숫자를 포함한 상태
 * 반환: 정상 종료 시 0
 */
int main(void) {
	char *buf, *p; // getenv가 돌려준 QUERY_STRING 버퍼와 두 인자를 가르는 '&' 위치
	char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE]; // 분리한 두 토큰과 최종 HTML 본문 버퍼
	int n1 = 0, n2 = 0; // 숫자로 변환한 두 피연산자. 파싱 전 기본값은 0

	// QUERY_STRING을 '&' 기준으로 둘로 나눔
	// 각 토큰에서는 '=' 뒤 숫자 부분만 정수로 변환
	if ((buf = getenv("QUERY_STRING")) != NULL) {
		p = strchr(buf, '&');
		// 환경 변수 문자열 안에서 '&'를 널 문자로 덮어써 왼쪽 토큰의 끝으로 사용
		*p = '\0';
		strcpy(arg1, buf);
		strcpy(arg2, p + 1);
		n1 = atoi(strchr(arg1, '=') + 1);
		n2 = atoi(strchr(arg2, '=') + 1);
	}

	// 같은 버퍼 끝에 계속 이어 붙여 브라우저가 표시할 본문 생성
	sprintf(content, "QUERY_STRING=%s\r\n<p>", buf);
	sprintf(content + strlen(content), "Welcome to add.com: ");
	sprintf(content + strlen(content), "THE Internet addition portal.\r\n<p>");
	sprintf(content + strlen(content), "The answer is: %d + %d = %d\r\n<p>", n1,
			n2, n1 + n2);
	sprintf(content + strlen(content), "Thanks for visiting!\r\n");

	// CGI 프로그램이 응답 헤더 일부를 직접 출력해야 함
	// 브라우저가 본문을 해석할 수 있도록 Content-Type과 길이를 먼저 전송
	printf("Content-type: text/html\r\n");
	printf("Content-length: %d\r\n", (int)strlen(content));
	// 헤더와 본문 사이는 빈 줄 한 줄로 구분
	printf("\r\n");
	printf("%s", content);
	// 표준출력 버퍼에 남은 데이터를 종료 전에 강제로 밀어냄
	fflush(stdout);

	exit(0);
}
/* $end adder */
