#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#define WINDOWS_INCLUDE
#include "../../../../../decompile/General/AltMods/OnlineCTR/global.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

struct SocketCtr
{
	SOCKET socket;

	char name[0xC];
	char characterID;
	char boolLoadSelf;
	char boolRaceSelf;
	char boolEndSelf;
};

struct SocketCtr CtrMain;
#define MAX_CLIENTS 8

unsigned char clientCount = 0;
int boolTakingConnections = 0;

// must match socket
int boolLoadAll = 0;
int boolRaceAll = 0;
int boolEndAll = 0;

struct SocketCtr CtrClient[MAX_CLIENTS];
fd_set master;

void ServerState_Boot()
{
	FD_ZERO(&master);
	FD_SET(CtrMain.socket, &master);

	for (int i = 0; i < clientCount; i++)
	{
		closesocket(CtrClient[i].socket);
		FD_CLR(CtrClient[i].socket, &master);
		memset(&CtrClient[i], 0, sizeof(struct SocketCtr));
	}

	clientCount = 0;
	boolLoadAll = 0;
	boolRaceAll = 0;
	boolEndAll = 0;
	printf("\nClientCount: 0\n");
	boolTakingConnections = 1;
}

void CheckNewClients()
{
	// I'd love to "accept" and then send a "reject"
	// message, but unfortunately select() sometimes
	// acts as blocking, will fix that later
	if (boolTakingConnections == 0)
		return;

	fd_set copy = master;

	// See who's talking to us
	int socketCount = select(0, &copy, 0, 0, 0);

	// Loop through all the current connections / potential connect
	for (int i = 0; i < socketCount; i++)
	{
		// Makes things easy for us doing this assignment
		SOCKET sock = copy.fd_array[i];

		// Is it an inbound communication?
		if (sock == CtrMain.socket)
		{
			// Accept a new connection
			SOCKET client = accept(CtrMain.socket, 0, 0);

			if (
					(clientCount == MAX_CLIENTS) ||
					(boolTakingConnections == 0)
				)
			{
				printf("Rejected\n");
				closesocket(client);
				continue;
			}

			// Add the new connection to the list of connected clients
			FD_SET(client, &master);

			// set socket to non-blocking
			unsigned long nonBlocking = 1;
			ioctlsocket(client, FIONBIO, &nonBlocking);

			// save client in array,
			// this is bad though, cause if someone disconnects and 
			// reconnects, they'll overwrite another socket in the array,
			// I'll fix it later
			CtrClient[clientCount].socket = client;
			clientCount++;

			// Send ClientID and clientCount back to all clients
			for (int j = 0; j < clientCount; j++)
			{
				struct SG_MessageClientStatus mw;
				mw.type = SG_NEWCLIENT;
				mw.size = sizeof(struct SG_MessageClientStatus);
				mw.clientID = j;
				mw.numClientsTotal = clientCount;
				send(CtrClient[j].socket, &mw, mw.size, 0);
			}
		}
	}
}

#if 0
void Disconnect(int i)
{
	// need to use closesocket, FD_CLR, and memset &CtrClient[i],
	// that's why this bugged out before, come back to it later

	clientCount--;
	printf("Disconnected %d, now %d remain\n", i, clientCount);
 
	for (int j = i; j < clientCount; j++)
	{
		memcpy(&CtrClient[j], &CtrClient[j+1], sizeof(struct SocketCtr));
	}

	// clear last
	memset(&CtrClient[clientCount], 0, sizeof(struct SocketCtr));

	// Send ClientID and clientCount back to all clients
	for (int j = 0; j < clientCount; j++)
	{
		struct SG_MessageClientStatus mw;
		mw.type = SG_DROPCLIENT;
		mw.size = sizeof(struct SG_MessageClientStatus);
		mw.clientID = i;
		mw.numClientsTotal = clientCount;
		send(CtrClient[j].socket, &mw, mw.size, 0);
	}
}
#endif

void ParseMessage(int i)
{
	char recvBufFull[0x100];
	memset(recvBufFull, 0xFF, 0x100);

	// if send() happens 100 times, it all gets picked up
	// in one recv() call, so only call recv one time
	int numBytes = recv(CtrClient[i].socket, recvBufFull, 0x100, 0);

	if (numBytes == -1)
	{
		int err = WSAGetLastError();

		// This happens due to nonblock, ignore it
		if (err != WSAEWOULDBLOCK)
		{
			// client closed
			if ((err == WSAENOTCONN) || (err == WSAECONNRESET))
			{
				//Disconnect(i);

				// reboot
				ServerState_Boot();
			}

			else
			{
				printf("%d\n", err);
			}

			return;
		}
	}

	// parse every message coming in
	for (int offset = 0; offset < numBytes; /**/)
	{
		struct CG_Header* recvBuf = &recvBufFull[offset];
		//printf("%d %d %d %d\n", numBytes, offset, recvBuf->size, recvBuf->type);
		offset += recvBuf->size;

		char sgBuffer[16];
		memset(sgBuffer, 0, 16);

		int sendAll = 1;

		// switch will compile into a jmp table, no funcPtrs needed
		switch (((struct CG_Header*)recvBuf)->type)
		{
			case CG_NAME:
			{
				struct SG_MessageName* s = &sgBuffer[0];
				struct CG_MessageName* r = recvBuf;

				// save new name
				memcpy(&CtrClient[i].name[0], &r->name[0], 12);
				printf("%d: %s\n", i, r->name);

				s->type = SG_NAME;
				s->size = sizeof(struct SG_MessageName);

				// send all OTHER names to THIS client
				for (int j = 0; j < 8; j++)
				{
					if (
						// skip empty sockets, skip self
						(CtrClient[j].socket != 0) &&
						(i != j)
						)
					{
						s->clientID = j;
						memcpy(&s->name[0], &CtrClient[j].name[0], 12);

						// clieint[i] gets 8 messages,
						// dont send 1 message to all [j]
						send(CtrClient[i].socket, s, s->size, 0);
					}
				}

				// send THIS name to all OTHER clients
				s->type = SG_NAME;
				s->size = sizeof(struct SG_MessageName);
				s->clientID = i;
				memcpy(&s->name[0], &CtrClient[i].name[0], 12);
				break;
			}

			case CG_TRACK:
			{
				// clients can only connect during track selection,
				// once the Client Gives CG_TRACK to server, close it
				boolTakingConnections = 0;

				struct SG_MessageTrack* s = &sgBuffer[0];
				struct CG_MessageTrack* r = recvBuf;

				s->type = SG_TRACK;
				s->size = sizeof(struct CG_MessageTrack);
				s->trackID = r->trackID;
				break;
			}

			case CG_CHARACTER:
			{
				struct SG_MessageCharacter* s = &sgBuffer[0];
				struct CG_MessageCharacter* r = recvBuf;

				s->type = SG_CHARACTER;
				s->size = sizeof(struct SG_MessageCharacter);
				s->clientID = i;
				s->characterID = r->characterID;
				s->boolLockedIn = r->boolLockedIn;

				CtrClient[i].characterID = s->characterID;
				CtrClient[i].boolLoadSelf = s->boolLockedIn;
				break;
			}

			case CG_STARTRACE:
			{
				printf("Ready to race: %d\n", i);
				CtrClient[i].boolRaceSelf = 1;
				sendAll = 0;
				break;
			}

			case CG_RACEINPUT:
			{
				struct SG_MessageRaceInput* s = &sgBuffer[0];
				struct CG_MessageRaceInput* r = recvBuf;

				s->type = SG_RACEINPUT;
				s->size = sizeof(struct SG_MessageRaceInput);
				s->clientID = i;
				s->buttonHold = r->buttonHold;
				break;
			}

			case CG_RACEPOS:
			{
				struct SG_MessageRacePos* s = &sgBuffer[0];
				struct CG_MessageRacePos* r = recvBuf;

				s->type = SG_RACEPOS;
				s->size = sizeof(struct SG_MessageRacePos);
				s->clientID = i;
				memcpy(&s->posX[0], &r->posX[0], 9);
				break;
			}

			case CG_RACEROT:
			{
				struct SG_MessageRaceRot* s = &sgBuffer[0];
				struct CG_MessageRaceRot* r = recvBuf;

				s->type = SG_RACEROT;
				s->size = sizeof(struct SG_MessageRaceRot);
				s->clientID = i;
				s->kartRot1 = r->kartRot1;
				s->kartRot2 = r->kartRot2;
				break;
			}

			default:
			{
				break;
			}
		}

		if (sendAll)
		{
			struct SG_Header* s = &sgBuffer[0];

			// send a message all other clients
			for (int j = 0; j < 8; j++)
			{
				if (
					// skip empty sockets, skip self
					(CtrClient[j].socket != 0) &&
					(i != j)
					)
				{
					send(CtrClient[j].socket, s, s->size, 0);
				}
			}
		}
	}
}

void ServerState_Tick()
{
	CheckNewClients();

	// check messages in sockets
	for (int i = 0; i < 8; i++)
	{
		if (CtrClient[i].socket != 0)
		{
			ParseMessage(i);
		}
	}

	// This must be here,
	// otherwise dropping a client wont start the race,
	// while all other drivers are ready to start

	if (clientCount == 0) return;

	if (!boolLoadAll)
	{
		boolLoadAll = 1;
		for (int j = 0; j < clientCount; j++)
			if (CtrClient[j].boolLoadSelf == 0)
				boolLoadAll = 0;

		if (boolLoadAll)
		{
			printf("Start Loading\n");

			struct SG_Header sg;
			sg.type = SG_STARTLOADING;
			sg.size = sizeof(struct SG_Header);

			// send a message to the client
			for (int j = 0; j < clientCount; j++)
				send(CtrClient[j].socket, &sg, sg.size, 0);
		}
	}

	if (!boolRaceAll)
	{
		boolRaceAll = 1;
		for (int j = 0; j < clientCount; j++)
			if (CtrClient[j].boolRaceSelf == 0)
				boolRaceAll = 0;

		if (boolRaceAll)
		{
			printf("Start Race\n");

			struct SG_Header sg;
			sg.type = SG_STARTRACE;
			sg.size = sizeof(struct SG_Header);

			// send a message to the client
			for (int j = 0; j < clientCount; j++)
				send(CtrClient[j].socket, &sg, sg.size, 0);
		}
	}

	if (!boolEndAll)
	{
		boolEndAll = 1;
		for (int j = 0; j < clientCount; j++)
			if (CtrClient[j].boolEndSelf == 0)
				boolEndAll = 0;

#if 0
		if (boolEndAll)
		{
			printf("End Race\n");

			struct SG_Header sg;
			sg.type = SG_ENDRACE;
			sg.size = sizeof(struct SG_Header);

			// send a message to the client
			for (int j = 0; j < clientCount; j++)
				send(CtrClient[j].socket, &sg, sg.size, 0);
		}
#endif
	}
}

int main()
{
	printf(__DATE__);
	printf("\n");
	printf(__TIME__);
	printf("\n\n");

	HWND console = GetConsoleWindow();
	RECT r;
	GetWindowRect(console, &r); //stores the console's current dimensions
	MoveWindow(console, r.left, r.top, 480, 240 + 35, TRUE);

	WSADATA wsaData;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		system("pause");
	}

	// TCP, port 1234 (call of duty uses 27000), 
	// accept from any (INADDR_ANY) address
	struct sockaddr_in socketIn;
	socketIn.sin_family = AF_INET;
	socketIn.sin_port = htons(1234);
	socketIn.sin_addr.S_un.S_addr = INADDR_ANY;

	CtrMain.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(CtrMain.socket, (struct sockaddr*)&socketIn, sizeof(socketIn));

	int flag = 1;
	setsockopt(CtrMain.socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));

	listen(CtrMain.socket, SOMAXCONN);
	printf("NodeServer ready on port 1234\n\n");

	// set LISTENING socket to non-blocking
	unsigned long nonBlocking = 1;
	iResult = ioctlsocket(CtrMain.socket, FIONBIO, &nonBlocking);

	ServerState_Boot();

	while (1)
	{
		ServerState_Tick();
	}
}