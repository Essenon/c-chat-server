/*
	����:   �����ҷ������˳��� v1.0 ,TCP Э�� with socket2.2
	����:   ZhangYiDa,ʹ�� Windows �׽����¼�ģ�ʹ���
	��ҳ:	http://www.aliensis.com / http://www.loogi.cn 
	���룺  Visual Studio with CL 14.0
	ƽ̨:   x86/x64 windows, 32 λ����
	ע�⣺----------------------------------------------------
		1: ֻ���Ա����32λ����!������ΪCL.EXE
			VC6/VS����.����������δ����.
		2: ���������ֻ֧��WSA_MAXIMUM_WAIT_EVENTS���ͻ�������
			���ֵ��60����,Ҳ����˵���ֻ֧��60���ͻ��ˡ�
		3: ֻ֧����������,����δʹ�ö��̷߳��ͺͽ���,
			�Լ������ڻ����Ͽ������Դ�ͼƬ��
	-----------------------------------------------------
*/
/*
	������֧�� �û��ǳ�
	������֧�� Զ�̹ر� 
	��������������δ����,�Ͼ���ֻ�Ǹ�ģ��
	�������շ����ݵĸ�ʽ
	<����>----------------------------------------------------
	��������:
		���������н��յ������ݶ��ᱻ��������͵�������������������ӵĿͻ���
		���ǵ��û����� USER ZhangYiDa(Ҳ������Ϣ���Դ�дUSER��ͷ��)
		�����Ϣ�൱��һ��ָ��,���������յ����ѷ��������Ϣ�Ŀͻ����ǳ�����Ϊ
		ZhangYiDa(�����������û��ǳ�,���޴���),������Ϣ���ᱻ����
		������������յ�SHUTָ��,��ô���������Ƚ�SHUT ָ����AUTHKEY�������ȷ,
		���������������رա�
		���������Ԥ��AuthKey = "TESTAUTHKEY16BYT"(����Ϊ16���ֽ�)��ô�ͻ��˷���
		'SHUT TESTAUTHKEY16BYT'ʱ������������ر� ,���AuthKey����,�ͻ��˽����յ�
	   ��AUTHKEY INCORRECT'��Ϣ 
	��������:
		��������ͻ��˷��͵����ݸ�ʽΪ,ǰ�����ֽڹ̶�Ϊʱ���֣��루�֣�
		������û��ǳ�,��ƫ��0x6��ʼΪ�û������ֽ������ǳƳ��ȣ�
		���û���ǳ�,��ô��ƫ��0x6��ʼ�����������ֽ�Ϊ0
		���������û����͵���Ϣ,������0��β
		�����ǳ�ΪZhang���û���ʱ��16��32��12������һ����Ϣhello!
		��ô���������͵�����Ϊ(������16���Ʊ�ʾ)
		0x0010 0x0020 0x000c + "Zhang" + 0x0000 + "hello!" + 0x0000
		����Լ21���ֽ�
*/
//winsock2.h��Ҫ��window.h��ǰ����
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <malloc.h>
#pragma comment(lib,"ws2_32.lib")

#define IS_DEBUG 1
//----�����б�---- 
//WSAEVENT _EventSelect(SOCKET sock, long event);
//��ʼ������ 
//EVESTACK* _InitQueneStruct();
//�Զ�����ڴ���亯�� 
//LPVOID _Malloc(unsigned size);
//��Ҫ����������� 
//int ServerMan(char*host, unsigned port);
//���̺��� 
//int ThreadStart(struct TPARAM* param); 

#define _BACKLOG	  (0x9)      
#define	_MAX_NAME	  (0x30)    //�����Ϊ50���ֽ�
#define _MAX_SOCKETS  (WSA_MAXIMUM_WAIT_EVENTS)
#define _MAX_EVENTS   (WSA_MAXIMUM_WAIT_EVENTS)
#define	_MAX_USERS	  (WSA_MAXIMUM_WAIT_EVENTS)
#define _MAX_RECV     (0x400)   //������1024�ֽ�
#define _AUTHKEY_LEN (0x10)
#define _ZERO 		  (0x0)

typedef struct
{
	char user[_MAX_NAME];  //�û���
	unsigned host;         //�û�IP
	unsigned port;         //�û��˿�
}USERINFO;

struct EVESTACK
{
	SOCKET    socStack[_MAX_SOCKETS]; //�׽��ֶ���
	WSAEVENT  eveStack[_MAX_EVENTS]; //�¼�����
	USERINFO* usrStack[_MAX_USERS];   //�û�����
	unsigned  cEvents;                 //��������ͳ��
};

struct RECVDATA
{
	char data[_MAX_RECV]; //���͵���Ϣ
	char user[_MAX_NAME]; //�������ǳ�
	unsigned brcv;        //���յ����ݴ�С���ֽڼƣ�
};

struct TPARAM
{   //���̲����ṹ 
	unsigned port;
	char*    host;
	unsigned flag;
	//16�ֽ�SHUTָ��AuthKEY 
	char AuthKey[_AUTHKEY_LEN];
};



WSAEVENT _EventSelect(SOCKET sock, long event)
{	//����sock,���������¼���socket,event�������õ��¼� 
	WSAEVENT ev;
	//����һ���¼�ģ��
	if ((ev = WSACreateEvent()) != WSA_INVALID_EVENT)
	{	//Ϊ�¼�ģ�������Ҫ��ص��¼�
		if (WSAEventSelect(sock, ev, event) != SOCKET_ERROR)
		{	//�ɹ����� �¼�ID
			return ev;
		}
	} //ʧ�ܷ���NULL
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
	//�������ṹ�ڴ�
	sta = (EVESTACK*)_Malloc(sizeof(EVESTACK));
	if (sta != NULL)
	{	//�����ں��ṹ�ڴ�
		lcptr = (unsigned)_Malloc(sizeof(USERINFO)*_MAX_USERS);
		if (lcptr != NULL)
		{	//�и������ڴ��,ʡ�÷���_MAX_USERS���ڴ��
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
{	//host ָ�������IP�ĵ�ַ,portΪ�˿�
	struct EVESTACK *sta;
	struct RECVDATA  rcv;
	WSADATA          wsa;
	sockaddr_in      sin;
	SOCKET 		    sock;
	WORD	    wVersion;
	register unsigned Index;
	//�׽��ְ汾2
	wVersion = 0x0202;
	WSAStartup(wVersion, &wsa);
	#if IS_DEBUG
	printf("\n>>ִ��WSAStartup() LastError = %d",GetLastError());
	#endif
	//���socket�ṹ
	sin.sin_family = AF_INET;
	sin.sin_port   = htons(param->port);
	sin.sin_addr.S_un.S_addr = inet_addr(param->host);
	//�����׽���
	sock = socket(AF_INET, SOCK_STREAM, 0x0);
	#if IS_DEBUG
	printf("\n>>ִ��socket() LastError = %d",GetLastError());
	#endif
	if (sock != INVALID_SOCKET)
	{	//�󶨵�ַ�Ͷ˿�
		if (bind(sock, (sockaddr*)&sin, sizeof(sockaddr_in)) != SOCKET_ERROR)
		{	//��ʼ�����˿�
			#if IS_DEBUG
			printf("\n>>ִ��bind() LastError = %d",GetLastError());
			#endif
			listen(sock, _BACKLOG);
			/*
				���䲢��ʼ��sta�ṹ��sta->eveStack[0]
				sta->usrStack[0] ʼ��Ϊ�� 
			*/
			sta = (struct EVESTACK*)_InitQueneStruct();
			//��������Ҫ��ע���ǿͻ�������FD_ACCEPT
			//�����ǽ������׽�����ӵ�����
			Index = sta->cEvents++; 
			sta->eveStack[Index] = _EventSelect(sock, FD_ACCEPT | FD_CLOSE);
			sta->socStack[Index] = sock;
			//����������ѭ��
			while ( true )
			{
				//�¼�ID��event id�� 
				unsigned evid;
				//�ṹsockaddr��size(sinlen for accept)
				int sinlen = sizeof(sockaddr_in);
				WSANETWORKEVENTS wsne;
				evid = WSAWaitForMultipleEvents(sta->cEvents, \
									sta->eveStack, FALSE, WSA_INFINITE, FALSE);
				if (evid <= WSA_WAIT_EVENT_0 + _MAX_EVENTS)
				{
					WSAEnumNetworkEvents(sta->socStack[evid], sta->eveStack[evid], &wsne);
					if (wsne.lNetworkEvents == FD_ACCEPT)
					{	//��Ϊ�¼�ģ���������������,�����������������ô����Ľ����ᱻ���� 
						if (sta->cEvents != WSA_MAXIMUM_WAIT_EVENTS)
						{
							sock = accept(sta->socStack[evid], (sockaddr*)&sin, (int*)&sinlen);
							if (sock != INVALID_SOCKET)
							{
								//û�жԷ���ֵ��飬һ��Ҳ���������
								//��Ҫ����������ô��ָ��,��ȷ�ֲ���������ʡ�ö���
								Index = sta->cEvents++; 
								sta->socStack[Index] = sock;
								sta->eveStack[Index] = _EventSelect(sock, FD_READ | FD_WRITE | FD_CLOSE);
								sta->usrStack[Index]->port = sin.sin_port;
								sta->usrStack[Index]->host = sin.sin_addr.S_un.S_addr;
								#if IS_DEBUG
								printf("\n�ͻ���IP = %X �˿� = %X ������.��ǰ�ͻ�������=%d.", \
														sin.sin_addr.S_un.S_addr, \
														sin.sin_port,\
														sta->cEvents-0x1);
								#endif
								
							}
						}
					}
					//�û�������Ϣ
					else if (wsne.lNetworkEvents == FD_READ)
					{
						rcv.brcv = recv(sta->socStack[evid], rcv.data, _MAX_RECV - 0x1, 0x0);
						//����û����͵������Ƿ�Ϊ USER XXX��ʽ
						if (*(unsigned*)rcv.data == 0x52455355)
						{	//������USER������û�����Ϊָ�������֣���USER ZhangYiDa����Ѹ��׽������û���Ӧ������
							memcpy(sta->usrStack[evid]->user, rcv.data + 0x5, rcv.brcv - 0x5);
							#if IS_DEBUG
							printf("\n�ͻ���IP = %X �˿� = %X ����Ϊ %s.",\
											sta->usrStack[evid]->host,\
											sta->usrStack[evid]->port,\
											sta->usrStack[evid]->user); 
							#endif 
						}
						else if(*(unsigned*)rcv.data == 0x54554853)
						{	//����һ���򵥵�Զ�̹ر�ָ��,������������յ�SHUTָ�� 
							//��ô,���򽫼��SHUTָ��Ĳ���AuthKey,���AuthKey��ȷ,��ô���������ر� 
							if(!memcmp(rcv.data + 0x5,param->AuthKey,_AUTHKEY_LEN))
							{
								#if IS_DEBUG
								printf("\n�յ�SHUTָ��,AUTHKEY=%s ,����������˳�.",param->AuthKey);
								#endif
								for(Index = 0;Index!=sta->cEvents;Index++)
								{	//�ڹر�ǰ��ͻ��˷���close��Ϣ 
									char * cloMsg = "SERVER CLOSED";
									#define  CLOS_MSG_LEN  0xd
									send(sta->socStack[Index], cloMsg, CLOS_MSG_LEN, 0x0);
									WSACloseEvent(sta->eveStack[Index]);
									closesocket(sta->socStack[Index]);
								}
								_Free(sta,sta->usrStack[0x0]);
								return WSACleanup();
							}
							//���Ѹÿͻ���AUTHKEY����ȷ 
							#define  AUTH_MSG_LEN  0x11
							char * incAuthKey = "AUTHKEY INCORRECT";
							send(sta->socStack[evid],incAuthKey,AUTH_MSG_LEN,0x0);
						}
						else 
						{
							/*
							���������Ҫ����װ��Ϣ,�����͵�������������������ӵĿͻ���
							####    �����и����©��   ####
							*/
							char buffer[_MAX_RECV];   //���ͻ�����
							char *base = buffer;    //�����Ҫ���������ֽ���
							unsigned bsend, Index, unameLen;
							SYSTEMTIME time;          //ʱ��ṹ,��Ҫʹ�� ʱ,��,��
							
							GetSystemTime(&time);
							//���ʱ����6���ֽڣ��ֱ�Ϊ ʱ �� �� ���� 
							((short*)base) [0x0]  =  time.wHour;
							((short*)base) [0x1]  =  time.wMinute;
							((short*)base) [0x2]  =  time.wSecond;
							((short*)base) [0x3]  =  time.wMilliseconds;
							//����û������ֽ�����һ��,������0��β��
							unameLen = strlen(sta->usrStack[evid]->user);
							base = (char*)memcpy(base + 0x8, sta->usrStack[evid]->user, unameLen);
							base = (char*)((unsigned)base + unameLen);
							((short*)base) [0x0]  =  _ZERO; 
							//�����Ϣ���ֽ�����һ��,������0��β��
							base = (char*)memcpy(base + 0x2, rcv.data, rcv.brcv);
							base = (char*)((unsigned)base + rcv.brcv);
							((short*)base) [0x0]  =  _ZERO;
							//������Ҫ���͵��ֽ���,����+0x2����Ϊ�������������ֽ�ҲҪ���ȥ
							bsend = (unsigned)base - (unsigned)buffer + 0x2;
							//ѭ����ÿ���ͻ��˷�����Ϣ
							Index = sta->cEvents;
							while (Index >= 0x1)
							{
								send(sta->socStack[Index--], buffer, bsend, 0x0);
							}
						}
					}
					//�û��˳���¼
					else if (wsne.lNetworkEvents == FD_CLOSE)
					{
						//�ر��¼�
						#if IS_DEBUG
						printf("\n�û�%s�˳� �׽��� = %X ", \
										sta->usrStack[evid]->user,\
										sta->socStack[evid]);
						#endif
						WSACloseEvent(sta->eveStack[evid]);
						//���ö�Ӧevid���¼�,�׽���,�û�����
						//evid != sta->cEvents���ܻ����Խ�����
						while (evid++ > sta->cEvents)
						{
							sta->eveStack[evid-0x1] = sta->eveStack[evid];
							sta->socStack[evid-0x1] = sta->socStack[evid];
							sta->usrStack[evid-0x1] = sta->usrStack[evid];
						}
						sta->cEvents--;
					}
					else if (wsne.lNetworkEvents == FD_WRITE)
					{	//���û�����,���û�����һ����Ϣ��ʾ���ӳɹ�
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
{   //������ʼ���� 
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
	char *ipaddr = "117.84.87.96";
	char *AuthKey = "TESTAUTHKEY16BYT";
	unsigned port = 9988;
	
	HANDLE hProcess;
	struct TPARAM param;
	param.host 		= 	ipaddr;
	param.port 		= 	port;
	param.flag 		= 	0x0;
	memcpy(param.AuthKey,AuthKey,_AUTHKEY_LEN);
	
	hProcess = StartThread(&param);
	printf("�����������Ѿ����� ID = %d", (unsigned)hProcess);
	getchar();
	return 0;
}

