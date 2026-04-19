# Proxy Lab 프록시 빌드용 Makefile
# 기본 `make`는 `proxy`를 만들고 `make handin`은 제출용 tar를 만듦

# C 소스 빌드에 쓰는 기본 컴파일러
# `gcc`는 GNU C 컴파일러 실행 파일
CC = gcc

# 컴파일 단계 공통 옵션
# `-g`는 디버거용 심볼 정보를 넣고 `-Wall`은 경고를 넓게 켜는 옵션
CFLAGS = -g -Wall

# 이 Makefile에서 링크 명령 끝에 붙는 추가 옵션 묶음
# `-lpthread`는 `pthread` 라이브러리를 연결하는 옵션
LDFLAGS = -lpthread

# 기본 타깃
# 이름 없이 `make`를 실행하면 `proxy`를 생성
all: proxy

# `csapp.c`를 오브젝트 파일로 컴파일
# `csapp.h` 변경도 다시 빌드에 반영
csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

# `proxy.c`를 오브젝트 파일로 컴파일
# `csapp.h`의 공용 선언 변경도 다시 빌드에 반영
proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

# 두 오브젝트를 묶어 `proxy` 실행 파일 생성
# `-o proxy`는 출력 이름 고정, `$(LDFLAGS)`는 링크 명령 끝에 붙는 추가 옵션
proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

# 제출용 tar 파일 생성 규칙
# 먼저 `make clean`으로 빌드 부산물을 지움
# 그다음 상위 디렉터리에서 `proxylab-handout` 디렉터리를 tar로 묶음
# `$(USER)`는 make가 읽는 `USER` 환경 변수값
# `tar cvf`의 `c`, `v`, `f`는 생성, 자세한 출력, 파일명 지정 의미
# `--exclude`는 보조 파일과 점으로 시작하는 이름 패턴을 제외하도록 tar에 전달
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

# 빌드 산출물과 압축 파일, 에디터 백업 파일을 한 번에 지우는 정리 규칙
# `*~`, `*.o`, `*.tar`, `*.zip`, `*.gzip`, `*.bzip`, `*.gz`는 와일드카드 패턴
# `proxy`는 실행 파일, `core`는 실행 중 남을 수 있는 덤프 파일
clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz
