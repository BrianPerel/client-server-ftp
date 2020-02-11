/*
 * By: Brian Perel and Jon Petani
 * Computer Networking - 477 - HW2
 * server FTP program
 *
 * performed by server ftp program
 * The list of ftp commands: mkdir, user, pass, mkdir, rmdir, pwd, stat, help, quit, cd, ls, rm, send, recv
*/

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/*
 * data connection port and control connection port (2 ports)
 * for connecting to host (client). Both needed for tcp connections
 * for communications. Control port is used to pass control information and
 * data port is used to send and recieve files
 */
#define CONTROL_CONNECTION_FTP_PORT 4002
#define DATA_CONNECTION_FTP_PORT 4001


/* Error and OK constants codes with values, for if something goes wrong in program */
#define OK 0
#define ER_INVALID_HOST_NAME -1
#define ER_CREATE_SOCKET_FAILED -2
#define ER_BIND_FAILED -3
#define ER_CONNECT_FAILED -4
#define ER_SEND_FAILED -5
#define ER_RECEIVE_FAILED -6
#define ER_ACCEPT_FAILED -7


/*
 * Function prototypes
 * 1. svcInitServer = starts server, gives you required information to start
 * 2. sendMessage = server function to send reply messages to client side.
 * Function parameters: (s) the socket which is the channel you travel through to connect to client side,
 *  (msg) the message sent to client
 *
 * 3. receiveMessage = server function to receive transmission messages from client side,
 * buffer parameter allows for transmission of whitespace,
 * buffer size is the size of the buffer  (size of whitespace -> indents, spaces)
 */
int svcInitServer(int *s);
int sendMessage(int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);


/* List of all global variables (char arrays) */
char userCmd[1024];	/* user typed ftp command line received from client */
char cmd[1024];		/* ftp command (without argument) extracted from userCmd */
char argument[1024];	/* argument (without ftp command) extracted from userCmd */
char replyMsg[1024];  /* buffer to send reply message to client */
char temp[1024]; /* temp char array (string) to hold userCmd before broken into 2 parts */
int userVar; /* Hold a temp variable value */
int bytesread = 600; /* variable used to hold value of a file during file I/O in below cmds, since requested to transfer 100 bytes of a file at a time */
static FILE *fp; /* declare a global file pointer to use for reading/writing ls, pwd, send cmds */


/*
 * main
 *
 * 1. Function to listen for connection request from client
 * 2. Receive ftp command one at a time from client
 * 3. Process received command
 * 4. Send a reply message to the client after processing the command with staus of performing (completing) the command
 * 6. On receiving QUIT ftp command, send reply to client and then close all sockets
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	0			- Successful execution until QUIT command from client
 *	ER_ACCEPT_FAILED	- Accepting client connection request failed
 *	N			- Failed stauts, value of N depends on the command processed
 */
int main(int argc, char *argv[]) {

	/* List of local varibale */
	int msgSize;        /* Size of msg received in octets (bytes) */
	int listenSocket;   /* listening server ftp socket for client connect request */
	int ccSocket;        /* Control connection socket - to be used in all client communication, to transfer control information between hosts */
	int dcSocket;	     /* declare data connection socket - to be used to transfer files between hosts */
	int status;        /* store the end status of the strcmp cmd result value (0, or other value). 0 represents successful execution, while any other value represents no match in comparison */
	char buffer[100]; 			 /* Max amount of bytes that you send, bytes to be read for a file */
	bool userCheck = false;  /* check whether user has entered username or not */
	bool passCheck = false;  /* check whether user has entered password or not */

	/*
	 * NOTE: without \n at the end of format string in printf,
         * UNIX will buffer (not flush)
	 * output to display and you will not see it on monitor.
	*/
	printf("Started execution of server ftp\n");

	 /* initialize (start) server ftp */
	printf("Initialize ftp server\n");

	/* Storing value of listenSocket from function into status, to start server */
	status=svcInitServer(&listenSocket);
	if(status != 0)
	{
		printf("Exiting server ftp due to svcInitServer returned error\n");
		exit(status);
	}

	/* loading text, identifies step we are at: which is between svcInitServer connection and ccSocket accept */
	printf("ftp server is waiting to accept connection\n");

	/* wait until connection request comes from client ftp */
	ccSocket = accept(listenSocket, NULL, NULL);

	/* indicates that accept function has finished and ended */
	printf("Came out of accept() function \n");

	/* if value received from accept function is not 0, then error exists */
	if(ccSocket < 0)
	{
		perror("cannot accept connection:");
		printf("Server ftp is terminating after closing listen socket.\n");
		close(listenSocket);  /* close listen socket */
		return (ER_ACCEPT_FAILED);  /* error exists, since value of ccSocket received from accept is not 0, meaning failure */
	}

	printf("Connected to client, calling receiveMsg to get ftp cmd from client \n");

	/*
	 * Receive and process ftp commands from client until quit command.
     * On receiving quit command, send reply to client and
     * then close the control connection socket "ccSocket".
     */
	do {
	    /* Receive client ftp commands until status of connection is less than 0, which would indicate failure */
 	    status=receiveMessage(ccSocket, userCmd, sizeof(userCmd), &msgSize);
	    if(status < 0)
	    {
					printf("Receive message failed. Closing control connection\n");
					printf("Server ftp is terminating.\n");
					break;
	    }

			/*
			 * Separate command and argument from userCmd, in order to be able to check the cmd entered alone from argument
			 * tokenization process = copy userCmd into temp variable, then break userCmd input into cmd and argument, the buffer is the space entered between cmd and arg
			 * everything before buffer (space) goes into cmd and everything after goes into argument
			 */
			strcpy(temp, userCmd);
			char *cmd = strtok(temp, " ");
			char *argument = strtok(NULL, " ");

			/* array of char pointers in an initializer list notation. Store ftp usernames and ftp passwords in 2 seperate arrays for the user and pass cmds */
			char *users[20] = {"abc", "xyz", "Brian", "John", "Amy", "Emily", "Michelle", "Jon", "Ava", "Isabella", "Alexandra", "Mia", "Scarface", "Terminator", "Dr. Java", "Tango", "Ryder", "Jack", "\0"};
			char *pass[20] = {"bananas", "monkeys", "friday", "book", "framingham1", "password", "20passwords", "Car", "Computer", "Phone", "BestPassword", "1password", "commander", "keys", "Calisthenics", "Dr. Dre", "The1letterPassword", "MyPassword", "\0"};

			/* variables to hold length of arrays above, use these 2 for loop continuation condition of for loops */
			int usersLength = sizeof(users) / sizeof(users[0]);
			int passLength = sizeof(pass) / sizeof(pass[0]);

			/*
			 * boolean var used to check if cmd is valid or not, if cmd is valid var gets assigned true.
			 * If by end of program var never switches from false, then print invalid ftp cmd
			 */
			bool cmdCheck = false;

            /*
 			 * Starting Homework#2 program to process all ftp commands
 			 * See Homework#2 for list of ftp commands to implement.
 			 */

				/*
				 * This block has the user login with useranme. Test for cmd to be user, then compare argument of user cmd to user array elements,
				 * search for username in user array. If argument is found in user array, reply success.
				 * If not, reply with invalid ftp username. cmd test 1, example: 'user Brian'
				 * Brian Perel implemented this command
				 */
				if(strcmp(cmd, "user") == 0) {
					cmdCheck = true;
					int x;
					for(x = 0; x < usersLength-2; x++) {
						if(strcmp(argument, users[x]) == 0) {
							strcpy(replyMsg, "ftp Username correct\n200 cmd OK\n");
							userVar = x;
							userCheck = true;
							break;
						}
					}
					if(userCheck == false || (strcmp(argument, "") == 0)) {
						strcpy(replyMsg, "Invalid ftp username\nUsername not found\n");
						printf("Invalid ftp username\nUsername not found\n");
					}
				}


			   /*
				* This block has the user login with password. Test for cmd to be pass, then compare argument of user cmd to pass array location userVar (which is the location of the username entered and found),
				* search for password in array, must match the password at that location in array to complete a pair match
				* cmd test 2, example 'pass monkeys'
				* Brian Perel implemented this command
                */
				else if(strcmp(cmd, "pass") == 0) { /* If the cmd alone is equal to 'pass' enter conditional statement */
					cmdCheck = true;
					if(userCheck == false) {
						strcpy(replyMsg, "Please enter an ftp username before entering a password\n"); /* return prompt message to client */
					}
					else if(strcmp(argument, pass[userVar]) == 0) {
                        strcpy(replyMsg, "Password correct\nLogin successful\n200 cmd OK\n");
                        passCheck = true;
					}
					else if(passCheck == false) {
						strcpy(replyMsg, "Invalid password for the user\nLogin failed. Please enter username and password.\n");
					}
				}


				/*
				 * This block tests the quit cmd, if quit is cmd then send replyMsg
				 * and then program terminates. cmd test 3, example 'quit'
				 * Brian Perel implemented this command
				 */
				else if((strcmp(cmd, "quit") == 0) || (strcmp(cmd, "bye") == 0)) {
						userCheck, passCheck, cmdCheck = true;
						strcpy(replyMsg, "200 cmd OK\nServer is closing\n");
				}


				/*
				 * This block tests the help cmd, if help is entered
				 * then send replyMsg with the description of all cmds together in 1 reply message
				 * cmd test 4, example 'help' or '?' -> mean the samething
				 * Jon Petani implemented this command
				 */
				else if((strcmp(cmd, "help") == 0) || (strcmp(cmd, "?") == 0)) {
						if((strcmp(userCmd, "help") == 0) || (strcmp(cmd, "?") == 0)) {
							userCheck, passCheck, cmdCheck = true;
							strcpy(replyMsg, "200 cmd OK\n"
							"user: Enter username and check against system. Must include a username as argument.\n"
							"pass: Enter password and check against system. Must include a password as argument.\n"
							"help: View all commands.\n"
							"mkdir: make directories. Must include a directory-name as argument.\n"
							"rmdir: delete directories. Must include a directory-name as arguement.\n"
							"cd: change working directories. Must include a directory-name as argument.\n"
							"rm: delete file\n. Must include a directory-name as argument.\n"
							"ls: show directory contents.\n"
							"pwd: view absolute directory path.\n"
							"send: send file to client. Must include a filename as argument.\n"
							"recv: get file from client. Must include a filename as argument.\n"
							"quit: exit program.\n"
							"stat: display file or filesystem status. Must include '-f /__' as argument.\n"
							"help: display information about all commands.\n");
						}
						else {
							strcpy(replyMsg, "500 invalid syntax.\nCommand Failed.\n");
						}
				}


				/*
				 * This block tests for the stat cmd, stat returns the status of
				 * the connection to the server or the general status of the server
				 * cmd test 5, example 'stat -f /export/home/perel/HW2-101519'
				 * Jon Petani implemented this command
				 */
				else if(strcmp(cmd, "stat") == 0) {
					cmdCheck = true;
					status = system(userCmd);
					if(status == 0) {
						strcpy(replyMsg, "200 cmd OK\n transfer mode is ascii\n");
					}
					else {
						strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
					}
				}


				/*
				 * This statement will check if user has logged in or not.
				 * If user has not logged in they won't be able to test the below cmds
				 * userCheck and passCheck are boolean variables to verify that user entered both parts of login process
				 */
				if((userCheck == true) && (passCheck == true)) {

						   /* This block tests for the mkdir cmd, check cmd, then do a system call to perform action
							* and send replymsg to client if successful
							* cmd test 6, example 'ls', 'mkdir abc', 'ls'
							* Brian Perel implemented this command
							*/
							if(strcmp(cmd, "mkdir") == 0) {
								cmdCheck = true;
								status = system(userCmd);
								if(status == 0) {
									strcpy(replyMsg, "200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


                           /* This block tests the rmdir cmd, check cmd, then do a system call to perform action
							* and send replymsg to client if successful
							* cmd test 7, example 'ls', 'rmdir abc', 'ls'
							* Jon Petani implemented this command
							*/
							else if(strcmp(cmd, "rmdir") == 0) {
								cmdCheck = true;
								status = system(userCmd);
								if(status == 0) {
									strcpy(replyMsg, "200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


							else if(strcmp(cmd, "dir") == 0) {
								cmdCheck = true;
							  status = system("dir > diroutput.txt");
								if((strcmp(userCmd, "dir") == 0) && status == 0) {
									fp = fopen("diroutput.txt", "r");
									bytesread = fread(replyMsg, 1, 1024, fp);
									replyMsg[bytesread] = '\0';	// store null terminator
									remove("diroutput.txt");
									fclose(fp);
									strcat(replyMsg, "\n200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


							/*
							 * This block tests the cd cmd, check cmd, then do a chdir call to perform actions (move to different dir)
							 * and send replymsg to client if successful
							 * use chdir() which performs a system call to change current working directory
							 * cmd test 8, example 'pwd', 'cd ..', 'pwd'
							 * Jon Petani implemented this command
							 */
							else if(strcmp(cmd, "cd") == 0) {
								cmdCheck = true;
								status = chdir(argument);
								if(status == 0) {
									strcpy(replyMsg, "200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


							/*
							 * This block tests the rm cmd, check cmd, then do a system call to perform action
							 * and send replymsg to client if successful
							 * cmd test 9, example 'ls', (use touch to create a file before running this ftp program), 'rm file1', 'ls'
							 * Brian Perel implemented this command
							 */
							else if(strcmp(cmd, "rm") == 0) {
								cmdCheck = true;
								status = system(userCmd);
								if(status == 0) {
									strcpy(replyMsg, "200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}

							/*
							 * mv cmd can be used to rename or move files
							 * rename (mv) = 'mv old-filename new-filename'
							 * move (mv) = 'mv filename destination-directory'
							 */
							else if(strcmp(cmd, "mv") == 0) {
								cmdCheck = true;
								status = system(userCmd);
								if(status == 0) {
									strcpy(replyMsg, "200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


							/*
							 * This block tests the pwd cmd, check cmd, then do a system call to perform action
							 * in which, pwd > pwdoutput.txt stores the content of pwd into the txt file
							 * open the txt file and read the file content to terminal while sending replymsg back to client
							 * remove the txt file once operation is finished (content is read to replyMsg and sent)
							 * cmd test 10, example 'pwd'
							 * Jon Petani implemented this command
							 */
							else if(strcmp(cmd, "pwd") == 0) {
								cmdCheck = true;
							  status = system("pwd > pwdoutput.txt");
								if((strcmp(userCmd, "pwd") == 0) && status == 0) {
									fp = fopen("pwdoutput.txt", "r");
									bytesread = fread(replyMsg, 1, 1024, fp);
									replyMsg[bytesread] = '\0';	// store null terminator
									remove("pwdoutput.txt");
									fclose(fp);
									strcat(replyMsg, "\n200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


							/*
							 * This block tests the ls cmd, check cmd, then do a system call to perform action
							 * in which ls > lsoutput.txt stores the content of ls into the txt file
							 * open the txt file and read the content to terminal while sending replymsg back to client
							 * remove the txt file once operation is finished (content is read to replyMsg and sent)
							 * cmd test 11, 'ls'
							 * Jon Petani implemented this command
							 */
							else if(strcmp(cmd, "ls") == 0) {
								cmdCheck = true;
								status = system("ls > lsoutput.txt");
								if((strcmp(userCmd, "ls") == 0) && status == 0) {
									fp = fopen("lsoutput.txt", "r");
									bytesread = fread(replyMsg, 1, 1024, fp);
									replyMsg[bytesread] = '\0';	// store null terminator
									remove("lsoutput.txt"); // remove the txt file after content is read
									fclose(fp);
									strcat(replyMsg, "\n200 cmd OK\n");
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


							/*
							 * This block tests the send cmd, check cmd, then establish connection with client, recieve the file as an argument
							 * Open file and write to the terminal screen. Receive information from client to store information for each line
							 * use msgSize to keep track of size of message which tracks size of file, once message size reaches 0 then the file is empty
							 * cmd test 12, file should be created before execution, example touch names.txt, 'ls', 'send filename' (cmd sends a file from clientftp to serverftp), 'ls'
							 * Brian Perel implemented this command
							 */
							else if((strcmp(cmd, "send") == 0) || (strcmp(cmd, "put") == 0)) {
								cmdCheck = true;
								if((strcmp(userCmd, "send") != 0) || (strcmp(userCmd, "put") != 0)) { /* If you enter send cmd with an argument proceed, otherwise (else) there is no argument so the cmd cannot be executed. In that case print invalid syntax since cmd is correct but syntax is not */
									status = clntConnect("10.3.200.17", &dcSocket);
									if(status != 0) {
										printf("Couldn't create data connection\n");
										strcpy(replyMsg, "Couldn't create data connection\n");
									}
								  else {
										printf("Successfully Created data connection\n");
										fp = fopen(argument, "w+");
										do {
												status = receiveMessage(dcSocket, buffer, sizeof(buffer), &msgSize);
												fwrite(buffer, 1, msgSize, fp);
										} while((msgSize > 0) && (status == 0));
										strcpy(replyMsg, "200 cmd OK\n");
										fclose(fp);
										close(dcSocket);
									}
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
							}


							/*
							 * This block tests the recv cmd, check recv, connect to client, open the file passed as argument,
							 * while not end of line read the content of the file and send to client as a message
							 * cmd test 13, example 'ls', 'recv filename' (server picks out received content and brings it back to client), 'ls'
							 * Brian Perel implemented this command
							 */
							else if((strcmp(cmd, "recv") == 0) || (strcmp(cmd, "get") == 0)) {
								cmdCheck = true;
								if((strcmp(userCmd, "recv") != 0) || (strcmp(userCmd, "get") != 0)) {  /* If you enter recv cmd with an argument proceed, otherwise (else) there is no argument so the cmd cannot be executed. In that case print invalid syntax since cmd is correct but syntax is not */
									status = clntConnect("10.3.200.17", &dcSocket);
									if(status != 0) {
										printf("Couldn't create data connection\n");
										strcpy(replyMsg, "Couldn't create data connection\n");
									}
								  else {
										printf("Successfully created data connection\n");
										strcpy(replyMsg, "Successfully created data connection\n");
										fp = fopen(argument, "r+");
										while(!feof(fp)) {
											bytesread = fread(buffer, 1, 100, fp);
											printf("Number of bytes read: %d\n", bytesread);
											sendMessage(dcSocket, buffer, bytesread);
							   		}
										strcpy(replyMsg, "200 cmd OK\n");
										fclose(fp);
										close(dcSocket);
									}
								}
								else {
									strcpy(replyMsg, "500 invalid syntax\nCommand Failed\n");
								}
						 }

					} /* close (userCheck==true and passCheck==true) if statement */

					if(userCheck == true && passCheck == true && cmdCheck == false) {
							strcpy(replyMsg, "Invalid FTP cmd\n");
					}

					else if((userCheck == false) && (passCheck == false) && (strcmp(cmd, "help") != 0) && (strcmp(cmd, "?") != 0) && (strcmp(cmd, "stat") != 0) && (strcmp(cmd, "user") != 0)) {
							strcpy(replyMsg, "Please enter username and password before testing cmds\n");
					}

	    /*
 	     * ftp server sends only one reply message to the client for
	     * each command received in this implementation.
	     */
	    status=sendMessage(ccSocket,replyMsg,strlen(replyMsg) + 1);	/* Added 1 to include NULL character in */
				/* the reply string strlen does not count NULL character */
	    if(status < 0)
	    {
				break;
	    }
	} while((strcmp(userCmd, "quit") != 0) && (strcmp(userCmd, "bye") != 0));

	printf("Closing control connection socket.\n");
	close (ccSocket);  /* Close client control connection socket */

	printf("Closing listen socket.\n");
	close(listenSocket);  /*close listen socket */

	printf("Exiting from server ftp main\n");

	return (status);
}

int clntConnect (
	char *serverName, /* server IP address in dot notation (input) */
	int *s 		  /* control connection socket number (output) */
	)
{
	int sock;	/* local variable to keep socket number */
	struct sockaddr_in clientAddress;  	/* local client IP address */
	struct sockaddr_in serverAddress;	/* server IP address */
	struct hostent *serverIPstructure;	/* host entry having server IP address in binary */


	/* Get IP address os server in binary from server name (IP in dot natation) */
	if((serverIPstructure = gethostbyname(serverName)) == NULL)
	{
		printf("%s is unknown server. \n", serverName);
		return (ER_INVALID_HOST_NAME);  /* error return */
	}

	/* Create control connection socket */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("cannot create socket ");
		return (ER_CREATE_SOCKET_FAILED);	/* error return */
	}

	/* initialize client address structure memory to zero */
	memset((char *) &clientAddress, 0, sizeof(clientAddress));

	/* Set local client IP address, and port in the address structure */
	clientAddress.sin_family = AF_INET;	/* Internet protocol family */
	clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY is 0, which means */
						 /* let the system fill client IP address */
	clientAddress.sin_port = 0;  /* With port set to 0, system will allocate a free port */
			  /* from 1024 to (64K -1) */

	/* Associate the socket with local client IP address and port */
	if(bind(sock,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}


	/* Initialize serverAddress memory to 0 */
	memset((char *) &serverAddress, 0, sizeof(serverAddress));

	/* Set ftp server ftp address in serverAddress */
	serverAddress.sin_family = AF_INET;
	memcpy((char *) &serverAddress.sin_addr, serverIPstructure->h_addr,
			serverIPstructure->h_length);
	serverAddress.sin_port = htons(DATA_CONNECTION_FTP_PORT);

	/* Connect to the server */
	if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("Cannot connect to server ");
		close (sock); 	/* close the control connection socket */
		return(ER_CONNECT_FAILED);  	/* error return */
	}


	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /* successful return */
}  // end of clntConnect() */


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

int svcInitServer (int *s) {	/*Listen socket number returned from this function */

	int sock;
	struct sockaddr_in svcAddr;
	int qlen;

	/*create a socket endpoint */
	if( (sock=socket(AF_INET, SOCK_STREAM,0)) <0)
	{
		perror("cannot create socket");
		return(ER_CREATE_SOCKET_FAILED);
	}

	/*initialize memory of svcAddr structure to zero. */
	memset((char *)&svcAddr,0, sizeof(svcAddr));

	/* initialize svcAddr to have server IP address and server listen port#. */
	svcAddr.sin_family = AF_INET;
	svcAddr.sin_addr.s_addr=htonl(INADDR_ANY);  /* server IP address */
	svcAddr.sin_port=htons(CONTROL_CONNECTION_FTP_PORT);    /* server listen port # */

	/* bind (associate) the listen socket number with server IP and port#.
	 * bind is a socket interface function
	 */
	if(bind(sock,(struct sockaddr *)&svcAddr,sizeof(svcAddr))<0)
	{
		perror("cannot bind");
		close(sock);
		return(ER_BIND_FAILED);	/* bind failed */
	}

	/*
	 * Set listen queue length to 1 outstanding connection request.
	 * This allows 1 outstanding connect request from client to wait
	 * while processing current connection request, which takes time.
	 * It prevents connection request to fail and client to think server is down
	 * when in fact server is running and busy processing connection request.
	 */
	qlen=1;


	/*
	 * Listen for connection request to come from client ftp.
	 * This is a non-blocking socket interface function call,
	 * meaning, server ftp execution does not block by the 'listen' funcgtion call.
	 * Call returns right away so that server can do whatever it wants.
	 * The TCP transport layer will continuously listen for request and
	 * accept it on behalf of server ftp when the connection requests comes.
	 */

	listen(sock,qlen);  /* socket interface function call */

	/* Store listen socket number to be returned in output parameter 's' */
	*s=sock;

	return(OK); /*successful return */
}


/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character arrary containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */

int sendMessage(
	int    s,	/* socket to be used to send msg to client */
	char   *msg, 	/* buffer having the message data */
	int    msgSize 	/* size of the message/data in bytes */
	)
{
	int i;


	/* Print the message to be sent byte by byte as character */
	for(i=0; i<msgSize; i++)
	{
		printf("%c",msg[i]);
	}
	printf("\n");

	if((send(s, msg, msgSize, 0)) < 0) /* socket interface call to transmit */
	{
		perror("unable to send ");
		return(ER_SEND_FAILED);
	}

	return(OK); /* successful send */
}


/*
 * receiveMessage
 *
 * Function to receive message from client ftp
 *
 * Parameters
 * s		- Socket to be used to receive msg from client (input)
 * buffer  	- Pointer to character arrary to store received msg (input/output)
 * bufferSize	- Maximum size of the array, "buffer" in octent/byte (input)
 *		    This is the maximum number of bytes that will be stored in buffer
 * msgSize	- Actual # of bytes received and stored in buffer in octet/byes (output)
 *
 * Return status
 *	OK			- Msg successfully received
 *	R_RECEIVE_FAILED	- Receiving msg failed
 */


int receiveMessage (
	int s, 		/* socket */
	char *buffer, 	/* buffer to store received msg */
	int bufferSize, /* how large the buffer is in octet */
	int *msgSize 	/* size of the received msg in octet */
	)
{
	int i;

	*msgSize=recv(s,buffer,bufferSize,0); /* socket interface call to receive msg */

	if(*msgSize<0)
	{
		perror("unable to receive");
		return(ER_RECEIVE_FAILED);
	}

	/* Print the received msg byte by byte */
	for(i=0;i<*msgSize;i++)
	{
		printf("%c", buffer[i]);
	}
	printf("\n");

	return (OK);
}
