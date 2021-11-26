/*
 * By: Brian Perel and Jon Petani
 * Computer Networking - 477 - HW2
 *
 * Client FTP program
 *
 * connects to Unix Server
 *
 * Purpose of program / overall functionality: the clientftp.c program file is what starts the connection (using a socket) to the server file which contains the logic to check the 13 commands
 * You enter a command in this program in the main function, then the command is sent to the server-side program, evaluated, reply is sent
 * saying if command is ftp command or not
 *
 * Use ftp protocol: file transferring between hosts, network protocol
 *
 * This includes, the list of ftp commands processed by server ftp.
 * The list of ftp commands is gets, puts, mkdir, user, pass, mkdir, rmdir, pwd, stat, help, quit, cd, ls, rm, send, recv
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* The port that the client will be connecting to, the port (logical channel) identifies a type of network service*/
#define CONTROL_CONNECTION_FTP_PORT 4002
#define DATA_CONNECTION_FTP_PORT 4001

/* Error and OK codes with values */
#define OK 0
#define ER_INVALID_HOST_NAME -1
#define ER_CREATE_SOCKET_FAILED -2
#define ER_BIND_FAILED -3
#define ER_CONNECT_FAILED -4
#define ER_SEND_FAILED -5
#define ER_RECEIVE_FAILED -6

/* Function prototypes */
int clntConnect(char *serverName, int *s); /* connects you to the client */
int sendMessage(int s, char *msg, int msgSize); /* sends cmd and arg to server */
int receiveMessage(int s, char *buffer, int bufferSize, int *msgSize); /* receive reply from server */

char userCmd[1024]; /* user typed ftp command line read from keyboard */
char cmd[1024]; /* ftp command extracted from userCmd */
char argument[1024]; /* argument extracted from userCmd */
char replyMsg[1024]; /* buffer to receive reply message from server */
char temp[1024]; /* char array will hold a copy of the userCmd array for division purposes */

/*
 * main
 *
 * Function connects to the ftp server using clntConnect function.
 * Reads one ftp command in one line from the keyboard into userCmd array.
 * Sends the user command to the server.
 * Receive reply message from the server.
 * On receiving reply to QUIT ftp command from the server,
 * close the control connection socket and exit from main
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	OK	- Successful execution until QUIT command from client
 *	N	- Failed status, value of N depends on the function called or cmd processed
 */
int main(int argc, char *argv[]) {

	int ccSocket; /* Control connection socket - to be used in all client communication */
	int dcSocket; /* data connection socket - to be used to transfer files between hosts*/
	int msgSize; /* size of the reply message received from the server */
	int status; /* Variable to hold status of strcmp, stores integer. If variable has 0, then it can indicate success to program, else not */
	int listenSocket; /* holds socket connection info */
	int ccPort; /* Store port number */
	int bytesread = 600; /* store number of bytes while reading file in send and recv cmds */
	char buffer[100]; /* amount of bytes of a file */
	FILE *fp; /* create file pointer to work with files in program */
	bool userCheck = false; /* boolean variable to check if user has entered username, set to false until user enters correct ftp username */
	bool passCheck = false; /* boolean variable to check if user has entered password, set to false until user enters correct ftp password */

	/*
	 * NOTE: without \n at the end of format string in printf,
	 * UNIX will buffer (not flush)
	 * output to display and you will not see it on monitor.
	 */
	printf("Started execution of client ftp\n");

	/* listen for data connection */

	status = clntConnect("10.3.200.17", &ccSocket); /* open cc Socket, connect to socket */

	/* If status value is not 0 then couldn't connect -> program terminates */
	if (status != 0) {
		printf("Connection to server failed, exiting main.\n");
		return (status);
	}

	status = svcInitServer(&listenSocket); /* listen for data connection, connect to server */

	if (status != 0) { /* Condition for successful connection to server */
		return (status);
	}

	/*
	 * Read an ftp command with argument, if any, in one line from user into userCmd.
	 * Copy ftp command part into ftpCmd and the argument into arg array.
	 * Send the line read (both ftp cmd part and the argument part) in userCmd to server.
	 * Receive reply message from the server.
	 * until quit command is typed by the user.
	 */

	/*
	 * do while loop, enter an ftp command and argument, separate cmd and argument.
	 * Then send command and argument to server-side
	 * repeat loop until user enters quit
	 */

	do {
		printf("my ftp> "); /* prompt user to enter cmd here */
		gets(userCmd); /* take the cmd input here */

		/*
		 * seperate cmd/args using strtok() (tokenize)
		 * copy userCmd (string containing cmd and argument) into temp array so that original array is not touched
		 * break cmd into cmd and argument (by space). So everything before space (delimeter) will be assigned to cmd array and everything after would go into argument array
		 */
		strcpy(temp, userCmd);
		char *cmd = strtok(temp, " ");
		char *argument = strtok(NULL, " ");

		/* send the userCmd to the server using sendMessage() which uses socket */
		status = sendMessage(ccSocket, userCmd, strlen(userCmd) + 1);
		if (status != OK) {
			break;
		}

		if (strcmp(cmd, "user") == 0) {
			if (strcmp(userCmd, "user") != 0) { /* if no argument is provided, then unsuccessful try, since user needs argument - username */
				userCheck = true;
			}
		}

		else if (strcmp(cmd, "pass") == 0) {
			if (userCheck == false) {
			} else if (strcmp(userCmd, "pass") != 0) { /* if no argument is provided, then unsuccessful try, since pass needs argument */
				passCheck = true;
			}
		}

		/*
		 * if user has entered username and password, only then can we implement or receive anything for send or recieve cmds
		 * part of validation for send and recv cmds step
		 */
		if (userCheck == true && passCheck == true) {

			/*
			 * Function Purpose: send file from client to server
			 * check cmd, perform data connection via socket to server
			 * open file with read mode, while file pointer is not null and not end of file, read into bytesread (put content in txt file)
			 * then open the txt file on client side
			 * dcSocket is only used for send and recv cmds
			 * dcSocket uses accept to wait and accept data connection from server
			 * open file to send data
			 * repeat until no file pointer
			 * bytesread stores the number of bytes read
			 * finally close the file and close the data connection
			 */
			if ((strcmp(cmd, "send") == 0) || (strcmp(cmd, "put") == 0)) {
				if ((strcmp(userCmd, "send") != 0)
						|| (strcmp(userCmd, "put") != 0)) { /* If you enter send cmd with an argument proceed, otherwise (else) there is no argument so the cmd cannot be executed. In that case print invalid syntax since cmd is correct but syntax is not */
					dcSocket = accept(listenSocket, NULL, NULL); /* use accept() to listen for connection via socket */
					fp = fopen("my_quotes_cs.txt", "r+"); /* open the file from client side */
					if (fp != NULL) { /* validate that file pointer is not null, file is not empty */
						printf("File opened\n");

						while (!feof(fp)) { /* loop while not end of line */
							bytesread = fread(buffer, 1, 100, fp); /* read 1 line at a time, reading 100 bytes from file a time */
							printf("Number of bytes read: %d\n", bytesread);
							sendMessage(dcSocket, buffer, bytesread); /* send data to server-side */
						}
						fclose(fp);
						close(dcSocket);
					} else {
						printf("fp open failed\n");
						strcpy(replyMsg, "Error! file open failed\n");
					}
				} else {
					printf("500 invalid syntax\nCommand Failed\n");
				}
			}

			/*
			 * Check recv cmd, connect to server, open file and receive message from server, then write that content into file
			 * then close file pointer and dcSocket
			 * On client side create and open file, receive message (using receiveMessage() which uses socket), and then write to that file
			 */
			else if ((strcmp(cmd, "recv") == 0) || (strcmp(cmd, "get") == 0)) {
				if ((strcmp(userCmd, "recv") != 0)
						|| (strcmp(userCmd, "get") != 0)) { /* If you enter recv cmd with an argument proceed, otherwise (else) there is no argument so the cmd cannot be executed. In that case print invalid syntax since cmd is correct but syntax is not */
					dcSocket = accept(listenSocket, NULL, NULL); /* establish the data connection using socket / listen for socket */
					fp = fopen("movie_stars_sc.txt", "w+"); // open the original file on client side
					if (fp != NULL) {
						printf("File opened\n");

						/* while message size > 0 receive message and write to the newly created txt file on client side */
						do {
							status = receiveMessage(dcSocket, buffer,
									sizeof(buffer), &msgSize);
							fwrite(buffer, 1, msgSize, fp);
						} while ((msgSize > 0) && (status == 0));

						fclose(fp);
						close(dcSocket);
					} else {
						printf("fp open failed\n");
						strcpy(replyMsg, "Error! file open failed\n");
					}
				} else {
					printf("500 invalid syntax\nCommand Failed\n");
				}
			}

		}

		status = receiveMessage(ccSocket, replyMsg, sizeof(replyMsg), &msgSize);
		if (status != OK) {
			break;
		}
	} while ((strcmp(userCmd, "quit") != 0) && (strcmp(userCmd, "bye") != 0));

	printf("Closing control connection\n");
	close(ccSocket); /* close control connection socket */
	close(listenSocket); /* close data connection socket */
	printf("Exiting client main \n");

	return (status);
}

/*
 * svcInitServer
 *
 * Function to create a socket and to listen for connection request from client
 *    using the created listen socket.
 *
 * Parameters
 * s		- Socket to listen for connection request (output)
 *
 * Return status
 *	OK			- Successfully created listen socket and listening
 *	ER_CREATE_SOCKET_FAILED	- socket creation failed
 */
int svcInitServer(int *s) {

	int sock, qlen;
	struct sockaddr_in svcAddr;

	/* create a socket endpoint that check for errors, less than 0 value indicates error hit */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("cannot create socket");
		return (ER_CREATE_SOCKET_FAILED);
	}

	/* initialize memory of svcAddr structure to zero. */
	memset((char*) &svcAddr, 0, sizeof(svcAddr));

	/* initialize svcAddr to have server IP address and server listen port#. */
	svcAddr.sin_family = AF_INET;
	svcAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* server IP address */
	svcAddr.sin_port = htons(DATA_CONNECTION_FTP_PORT); /* server listen port # */

	/* bind (associate) the listen socket number with server IP and port#. Bind is a socket interface function */
	if (bind(sock, (struct sockaddr*) &svcAddr, sizeof(svcAddr)) < 0) {
		perror("cannot bind");
		close(sock);
		return (ER_BIND_FAILED); /* bind failed */
	}

	/*
	 * Set listen queue length to 1 outstanding connection request.
	 * This allows 1 outstanding connect request from client to wait
	 * while processing current connection request, which takes time.
	 * It prevents connection request to fail and client to think server is down
	 * when in fact server is running and busy processing connection request.
	 */
	qlen = 1;

	/*
	 * Listen for connection request to come from client ftp.
	 * This is a non-blocking socket interface function call,
	 * meaning, server ftp execution does not block by the 'listen' funcgtion call.
	 * Call returns right away so that server can do whatever it wants.
	 * The TCP transport layer will continuously listen for request and
	 * accept it on behalf of server ftp when the connection requests comes.
	 */

	listen(sock, qlen); /* socket interface function call */

	/* Store listen socket number to be returned in output parameter 's' */
	*s = sock;

	return (OK); /*successful return */
}

/*
 * clntConnect
 *
 * Function to create a socket, bind local client IP address and port to the socket
 * and connect to the server
 *
 * Parameters
 * serverName	- IP address of server in dot notation (input)
 * s		- Control connection socket number (output)
 *
 * Return status
 *	OK			- Successfully connected to the server
 *	ER_INVALID_HOST_NAME	- Invalid server name
 *	ER_CREATE_SOCKET_FAILED	- Cannot create socket
 *	ER_BIND_FAILED		- bind failed
 *	ER_CONNECT_FAILED	- connect failed
 */
int clntConnect(char *serverName, int *s) {

	int sock; /* local variable to keep socket number */

	struct sockaddr_in clientAddress; /* local client IP address */
	struct sockaddr_in serverAddress; /* server IP address */
	struct hostent *serverIPstructure; /* host entry having server IP address in binary */

	/* Get IP address os server in binary from server name (IP in dot natation) */
	if ((serverIPstructure = gethostbyname(serverName)) == NULL) {
		printf("%s is unknown server. \n", serverName);
		return (ER_INVALID_HOST_NAME); /* error return */
	}

	/* Create control connection socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("cannot create socket ");
		return (ER_CREATE_SOCKET_FAILED); /* error return */
	}

	/* initialize client address structure memory to zero */
	memset((char*) &clientAddress, 0, sizeof(clientAddress));

	/* Set local client IP address, and port in the address structure */
	clientAddress.sin_family = AF_INET; /* Internet protocol family */
	clientAddress.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY is 0, which means let the system fill client IP address */
	clientAddress.sin_port = 0; /* With port set to 0, system will allocate a free port from 1024 to (64K -1) */

	/* Associate the socket with local client IP address and port */
	if (bind(sock, (struct sockaddr*) &clientAddress, sizeof(clientAddress)) < 0) {
		perror("cannot bind");
		close(sock);
		return (ER_BIND_FAILED); /* bind failed */
	}

	/* Initialize serverAddress memory to 0 */
	memset((char*) &serverAddress, 0, sizeof(serverAddress));

	/* Set ftp server ftp address in serverAddress */
	serverAddress.sin_family = AF_INET;
	memcpy((char*) &serverAddress.sin_addr, serverIPstructure->h_addr,
			serverIPstructure->h_length);
	serverAddress.sin_port = htons(CONTROL_CONNECTION_FTP_PORT);

	/* Connect to the server */
	if (connect(sock, (struct sockaddr*) &serverAddress, sizeof(serverAddress))
			< 0) {
		perror("Cannot connect to server");
		close(sock); /* close the control connection socket */
		return (ER_CONNECT_FAILED); /* error return */
	}

	/* Store listen socket number to be returned in output parameter 's' */
	*s = sock;

	return (OK); /* successful return */
}

/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character array buffer containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */
int sendMessage(int s, char *msg, int msgSize) {

	/* Print the message to be sent byte by byte as character */
	for (int i = 0; i < msgSize; i++) {
		printf("%c", msg[i]);
	}

	printf("\n");

	/* socket interface call to transmit */
	if ((send(s, msg, msgSize, 0)) < 0) {
		perror("unable to send ");
		return (ER_SEND_FAILED);
	}

	return (OK); /* successful send */
}

/*
 * receiveMessage
 *
 * Function to receive message from client ftp
 *
 * Parameters
 * s		- Socket to be used to receive msg from client (input)
 * buffer  	- Pointer to character array buffer to store received msg (input/output)
 * bufferSize	- Maximum size of the array, "buffer" in octent/byte (input)
 *		    This is the maximum number of bytes that will be stored in buffer
 * msgSize	- Actual # of bytes received and stored in buffer in octet/byes (output)
 *
 * Return status
 *	OK			- Msg successfully received
 *	ER_RECEIVE_FAILED	- Receiving msg failed
 */
int receiveMessage(int s, char *buffer, int bufferSize, int *msgSize) {

	*msgSize = recv(s, buffer, bufferSize, 0); /* socket interface call to receive msg */

	if (*msgSize < 0) {
		perror("unable to receive");
		return (ER_RECEIVE_FAILED);
	}

	/* Print the received msg byte by byte */
	for (int i = 0; i < *msgSize; i++) {
		printf("%c", buffer[i]);
	}

	printf("\n");
	return (OK);
}

/*
 * clntExtractReplyCode
 *
 * Function to extract the three digit reply code
 * from the server reply message received.
 * It is assumed that the reply message string is of the following format
 *      ddd  text
 * where ddd is the three digit reply code followed by or or more space.
 *
 * Parameters
 *	buffer	  - Pointer to an array containing the reply message (input)
 *	replyCode - reply code number (output)
 *
 * Return status
 *	OK	- Successful (returns always success code
 */
int clntExtractReplyCode(char *buffer, int *replyCode) {
	/* extract the codefrom the server reply message */
	sscanf(buffer, "%d", replyCode);
	return (OK);
}
