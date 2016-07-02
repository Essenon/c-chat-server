/*
	名称:   聊天室服务器端程序 v1.0 ,TCP 协议 with socket2.2
	作者:   ZhangYiDa,使用 Windows 套接字事件模型创作
	主页:	http://www.aliensis.com / http://www.loogi.cn 
	编译：  Visual Studio with CL 14.0
	平台:   x86/x64 windows, 32 位编译
	注意：----------------------------------------------------
		1: 只可以编译成32位程序!编译器为CL.EXE
			VC6/VS均可.其它编译器未测试.
		2: 服务器最多只支持WSA_MAXIMUM_WAIT_EVENTS个客户端连接
			这个值在60左右,也就是说最多只支持60个客户端。
		3: 只支持文字聊天,所以未使用多线程发送和接收,
			自己可以在基础上开发可以带图片的
	-----------------------------------------------------
*/
/*
	服务器支持 用户昵称
	服务器支持 远程关闭 
	服务器数据连接未加密,毕竟这只是个模拟
	服务器收发数据的格式
	<描述>----------------------------------------------------
	接收数据:
		理论上所有接收到的数据都会被打包并发送到所有与服务器建立连接的客户端
		但是当用户发送 USER ZhangYiDa(也就是消息是以大写USER开头的)
		这个消息相当于一条指令,服务器接收到后会把发送这个消息的客户端昵称设置为
		ZhangYiDa(这用于设置用户昵称,不限次数),这条消息不会被发送
		如果服务器接收到SHUT指令,那么服务器将比较SHUT 指令后的AUTHKEY，如果正确,
		服务器将无条件关闭。
		例如服务器预设AuthKey = "TESTAUTHKEY16BYT"(必须为16个字节)那么客户端发送
		'SHUT TESTAUTHKEY16BYT'时，服务器将会关闭 ,如果AuthKey错误,客户端将会收到
	   ‘AUTHKEY INCORRECT'消息 
	发送数据:
		服务器向客户端发送的数据格式为,前六个字节固定为时：分：秒（字）
		如果有用户昵称,从偏移0x6开始为用户名（字节数视昵称长度）
		如果没有昵称,那么从偏移0x6开始的连续两个字节为0
		接下来是用户发送的消息,以两个0结尾
		例如昵称为Zhang的用户在时间16：32：12发送了一条消息hello!
		那么服务器发送的数据为(部分用16进制表示)
		0x0010 0x0020 0x000c + "Zhang" + 0x0000 + "hello!" + 0x0000
		共计约21个字节
*/
//winsock2.h需要比window.h提前定义
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#pragma comment(lib,"ws2_32.lib")

#define IS_DEBUG 1
//----函数列表---- 
//WSAEVENT _EventSelect(SOCKET sock, long event);
//初始化函数 
//EVESTACK* _InitQueneStruct();
//自定义的内存分配函数 
//LPVOID _Malloc(unsigned size);
//主要的聊天服务函数 
//int ServerMan(char*host, unsigned port);
//进程函数 
//int ThreadStart(struct TPARAM* param); 

#define _BACKLOG	  (0x9)      
#define	_MAX_NAME	  (0x30)    //名字最长为50个字节
#define _MAX_SOCKETS  (WSA_MAXIMUM_WAIT_EVENTS)
#define _MAX_EVENTS   (WSA_MAXIMUM_WAIT_EVENTS)
#define	_MAX_USERS	  (WSA_MAXIMUM_WAIT_EVENTS)
#define _MAX_RECV     (0x400)   //最多接收1024字节
#define _AUTHKEY_LEN (0x10)
#define _ZERO 		  (0x0)

typedef struct
{
	char user[_MAX_NAME];  //用户名
	unsigned host;         //用户IP
	unsigned port;         //用户端口
}USERINFO;

struct EVESTACK
{
	SOCKET    socStack[_MAX_SOCKETS]; //套接字队列
	WSAEVENT  eveStack[_MAX_EVENTS]; //事件队列
	USERINFO* usrStack[_MAX_USERS];   //用户队列
	unsigned  cEvents;                 //队列人数统计
};

struct RECVDATA
{
	char data[_MAX_RECV]; //发送的消息
	char user[_MAX_NAME]; //发送者昵称
	unsigned brcv;        //接收的数据大小（字节计）
};

struct TPARAM
{   //进程参数结构 
	unsigned port;
	char*    host;
	unsigned flag;
	//16字节SHUT指令AuthKEY 
	char AuthKey[_AUTHKEY_LEN];
};



WSAEVENT _EventSelect(SOCKET sock, long event)
{	//参数sock,是需设置事件的socket,event是需设置的事件 
	WSAEVENT ev;
	//创建一个事件模型
	if ((ev = WSACreateEvent()) != WSA_INVALID_EVENT)
	{	//为事件模型添加需要监控的事件
		if (WSAEventSelect(sock, ev, event) != SOCKET_ERROR)
		{	//成功返回 事件ID
			return ev;
		}
	} //失败返回NULL
	return NULL;
}
LPVOID _Malloc(unsigned size)
{
	HANDLE hHeap;
	hHeap = GetProcessHeap();
	return HeapAlloc(hHeap, HEAP_ZERO_MEMORY | \
					HEAP_NO_SERIALIZE, size);
}
void _Free(HANDLE hm1,HANDLE hm2)
{	
	HANDLE hHeap;
	hHeap = GetProcessHeap();
	HeapFree(hHeap,HEAP_ZERO_MEMORY|HEAP_NO_SERIALIZE,hm1);
	HeapFree(hHeap,HEAP_ZERO_MEMORY|HEAP_NO_SERIALIZE,hm2);
}
LPVOID _InitQueneStruct()
{
	struct EVESTACK *sta;
	unsigned  Index, lcptr;
	//分配主结构内存
	sta = (EVESTACK*)_Malloc(sizeof(EVESTACK));
	if (sta != NULL)
	{	//分配内含结构内存
		lcptr = (unsigned)_Malloc(sizeof(USERINFO)*_MAX_USERS);
		if (lcptr != NULL)
		{	//切割分配的内存块,省得分配_MAX_USERS个内存块
			Index = 0;
			while (Index != _MAX_USERS)
			{
				sta->usrStack[Index++] = (USERINFO*)lcptr;
				lcptr = lcptr + sizeof(USERINFO);
			}
			sta->cEvents = 0;
			return  sta;
		}
	}
	return  NULL;
}

int ServerMan(struct TPARAM*param)
{	//host 指向服务器IP的地址,port为端口
	struct EVESTACK *sta;
	struct RECVDATA  rcv;
	WSADATA          wsa;
	sockaddr_in      sin;
	SOCKET 		    sock;
	WORD	    wVersion;
	register unsigned Index;
	//套接字版本2
	wVersion = 0x0202;
	WSAStartup(wVersion, &wsa);
	#if IS_DEBUG
	printf("\n>>执行WSAStartup() LastError = %d",GetLastError());
	#endif
	//填充socket结构
	sin.sin_family = AF_INET;
	sin.sin_port   = htons(param->port);
	sin.sin_addr.S_un.S_addr = inet_addr(param->host);
	//分配套接字
	sock = socket(AF_INET, SOCK_STREAM, 0x0);
	#if IS_DEBUG
	printf("\n>>执行socket() LastError = %d",GetLastError());
	#endif
	if (sock != INVALID_SOCKET)
	{	//绑定地址和端口
		if (bind(sock, (sockaddr*)&sin, sizeof(sockaddr_in)) != SOCKET_ERROR)
		{	//开始监听端口
			#if IS_DEBUG
			printf("\n>>执行bind() LastError = %d",GetLastError());
			#endif
			listen(sock, _BACKLOG);
			/*
				分配并初始化sta结构，sta->eveStack[0]
				sta->usrStack[0] 始终为空 
			*/
			sta = (struct EVESTACK*)_InitQueneStruct();
			//服务器主要关注的是客户端连接FD_ACCEPT
			//下面是将服务套接字添加到队列
			Index = sta->cEvents++; 
			sta->eveStack[Index] = _EventSelect(sock, FD_ACCEPT | FD_CLOSE);
			sta->socStack[Index] = sock;
			//下面进入服务循环
			while ( true )
			{
				//事件ID（event id） 
				unsigned evid;
				//结构sockaddr的size(sinlen for accept)
				int sinlen = sizeof(sockaddr_in);
				WSANETWORKEVENTS wsne;
				evid = WSAWaitForMultipleEvents(sta->cEvents, \
									sta->eveStack, FALSE, WSA_INFINITE, FALSE);
				if (evid <= WSA_WAIT_EVENT_0 + _MAX_EVENTS)
				{
					WSAEnumNetworkEvents(sta->socStack[evid], sta->eveStack[evid], &wsne);
					if (wsne.lNetworkEvents == FD_ACCEPT)
					{	//因为事件模型是有最大容量的,所以如果队列满了那么后面的将不会被接受 
						if (sta->cEvents != WSA_MAXIMUM_WAIT_EVENTS)
						{
							sock = accept(sta->socStack[evid], (sockaddr*)&sin, (int*)&sinlen);
							if (sock != INVALID_SOCKET)
							{
								//没有对返回值检查，一般也不会出问题
								//不要怪我用了这么多指针,的确局部变量可以省好多事
								Index = sta->cEvents++; 
								sta->socStack[Index] = sock;
								sta->eveStack[Index] = _EventSelect(sock, FD_READ | FD_WRITE | FD_CLOSE);
								sta->usrStack[Index]->port = sin.sin_port;
								sta->usrStack[Index]->host = sin.sin_addr.S_un.S_addr;
								#if IS_DEBUG
								printf("\n客户端IP = %X 端口 = %X 已连接.当前客户端数量=%d.", \
														sin.sin_addr.S_un.S_addr, \
														sin.sin_port,\
														sta->cEvents-0x1);
								#endif
								
							}
						}
					}
					//用户发送消息
					else if (wsne.lNetworkEvents == FD_READ)
					{
						rcv.brcv = recv(sta->socStack[evid], rcv.data, _MAX_RECV - 0x1, 0x0);
						//检测用户发送的数据是否为 USER XXX格式
						if (*(unsigned*)rcv.data == 0x52455355)
						{	//将发送USER命令的用户命名为指令后的名字（如USER ZhangYiDa将会把该套接字与用户对应起来）
							memcpy(sta->usrStack[evid]->user, rcv.data + 0x5, rcv.brcv - 0x5);
							#if IS_DEBUG
							printf("\n客户端IP = %X 端口 = %X 改名为 %s.",\
											sta->usrStack[evid]->host,\
											sta->usrStack[evid]->port,\
											sta->usrStack[evid]->user); 
							#endif 
						}
						else if(*(unsigned*)rcv.data == 0x54554853)
						{	//这是一个简单的远程关闭指令,如果服务器接收到SHUT指令 
							//那么,程序将检查SHUT指令的参数AuthKey,如果AuthKey正确,那么服务器将关闭 
							if(!memcmp(rcv.data + 0x5,param->AuthKey,_AUTHKEY_LEN))
							{
								#if IS_DEBUG
								printf("\n收到SHUT指令,AUTHKEY=%s ,服务进程已退出.",param->AuthKey);
								#endif
								for(Index = 0;Index!=sta->cEvents;Index++)
								{	//在关闭前向客户端发送close消息 
									char * cloMsg = "SERVER CLOSED";
									#define  CLOS_MSG_LEN  0xd
									send(sta->socStack[Index], cloMsg, CLOS_MSG_LEN, 0x0);
									WSACloseEvent(sta->eveStack[Index]);
									closesocket(sta->socStack[Index]);
								}
								_Free(sta,sta->usrStack[0x0]);
								return WSACleanup();
							}
							//提醒该客户端AUTHKEY不正确 
							#define  AUTH_MSG_LEN  0x11
							char * incAuthKey = "AUTHKEY INCORRECT";
							send(sta->socStack[evid],incAuthKey,AUTH_MSG_LEN,0x0);
						}
						else 
						{
							/*
							这个部分主要是组装消息,并发送到所有与服务器建立连接的客户端
							####    这里有个溢出漏洞   ####
							*/
							char buffer[_MAX_RECV];   //发送缓冲区
							char *base = buffer;    //这个主要是来计算字节数
							unsigned bsend, Index, unameLen;
							SYSTEMTIME time;          //时间结构,主要使用 时,分,秒
							
							GetSystemTime(&time);
							//填充时间域（6个字节）分别为 时 分 秒 毫秒 
							((short*)base) [0x0]  =  time.wHour;
							((short*)base) [0x1]  =  time.wMinute;
							((short*)base) [0x2]  =  time.wSecond;
							((short*)base) [0x3]  =  time.wMilliseconds;
							//填充用户名域（字节数不一定,以两个0结尾）
							unameLen = strlen(sta->usrStack[evid]->user);
							base = (char*)memcpy(base + 0x8, sta->usrStack[evid]->user, unameLen);
							base = (char*)((unsigned)base + unameLen);
							((short*)base) [0x0]  =  _ZERO; 
							//填充信息域（字节数不一定,以两个0结尾）
							base = (char*)memcpy(base + 0x2, rcv.data, rcv.brcv);
							base = (char*)((unsigned)base + rcv.brcv);
							((short*)base) [0x0]  =  _ZERO;
							//计算需要发送的字节数,后面+0x2是因为上面这句的两个字节也要算进去
							bsend = (unsigned)base - (unsigned)buffer + 0x2;
							//循环向每个客户端发送消息
							Index = sta->cEvents;
							while (Index >= 0x1)
							{
								send(sta->socStack[Index--], buffer, bsend, 0x0);
							}
						}
					}
					//用户退出登录
					else if (wsne.lNetworkEvents == FD_CLOSE)
					{
						//关闭事件
						#if IS_DEBUG
						printf("\n用户%s退出 套接字 = %X ", \
										sta->usrStack[evid]->user,\
										sta->socStack[evid]);
						#endif
						WSACloseEvent(sta->eveStack[evid]);
						//将该对应evid的事件,套接字,用户覆盖
						//evid != sta->cEvents可能会出现越界访问
						while (evid++ > sta->cEvents)
						{
							sta->eveStack[evid-0x1] = sta->eveStack[evid];
							sta->socStack[evid-0x1] = sta->socStack[evid];
							sta->usrStack[evid-0x1] = sta->usrStack[evid];
						}
						sta->cEvents--;
					}
					else if (wsne.lNetworkEvents == FD_WRITE)
					{	//当用户连接,向用户发送一条消息提示连接成功
						char* accMsg = "CONNECTED";
						#define  CNNT_MSG_LEN  0x9
						send(sta->socStack[evid], accMsg, CNNT_MSG_LEN, 0x0);
					}
				}
			}
		}
	}
	return WSACleanup();
}


HANDLE StartThread(struct TPARAM*param)
{   //进程起始函数 
	static HANDLE hProcess;
	if (!param->flag)
	{	
		static TPARAM* tParam = (TPARAM*)_Malloc(sizeof(TPARAM));
		memcpy(tParam,param,sizeof(TPARAM)); 
		hProcess = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)\
					ServerMan, tParam, 0x0, NULL);
		param->flag = (hProcess==NULL)?0x0:0x1;
	}
	return hProcess;
}

int main(void)
{
	char *ipaddr = "172.0.0.1";
	char *AuthKey = "TESTAUTHKEY16BYT";
	unsigned port = 9988;
	
	HANDLE hProcess;
	struct TPARAM param;
	param.host 		= 	ipaddr;
	param.port 		= 	port;
	param.flag 		= 	0x0;
	memcpy(param.AuthKey,AuthKey,_AUTHKEY_LEN);
	
	hProcess = StartThread(&param);
	printf("服务主进程已经创建 ID = %d", (unsigned)hProcess);
	getchar();
	return 0;
}

