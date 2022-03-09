/*******************************************************************************
 * Copyright (c) 2012, 2020 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include <time.h>
#include <pthread.h>
#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif
#define GNU_SOURCE
#define ADDRESS     "tcp://localhost:1883"
//#define CLIENTID    "client2 "
#define GLOBAL_TOPIC "online"
#define QOS         1
#define TIMEOUT     10000L
#define TAM 50

pthread_t threads[3];

MQTTAsync client;
MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
int rc;


int finished = 0;

char topic_control[50]= "control_"; 
char client_id[12];


MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m);



void *received(){
	
	int count = 0;

	printf("Executando thread...\n");
	//	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

	MQTTAsync_subscribe(client, "online", QOS, &opts);

	while(1){
		//printf("teste %d\n",count);

		//count++;
	}
}


void menu(){
	
	int opc;

	printf("1-Listar\n2-Mensagens\n3-sair\n");
	scanf("%d",&opc);

	switch (opc){
		case 1:
			
			break;
		
		case 2:
			pthread_create(&(threads[1]), NULL,received,NULL);
			pthread_join(threads[1],NULL);
		case 3:
			exit(0) ;
		
		default:
			break;
	}
}

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
 		finished = 1;
	}
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Disconnect failed\n");
	finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	finished = 1;
}

void onSendFailure(void* context, MQTTAsync_failureData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	printf("Message send failed token %d error code %d\n", response->token, response->code);
	//opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = client;
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}

void onSend(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	printf("Message with token value %d delivery confirmed\n", response->token);
	//opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = client;
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;
	char msg_status[TAM];
	char msg_conexao[TAM];
	
	printf("Successful connection\n");
	opts.onSuccess = onSend;
	opts.onFailure = onSendFailure;
	opts.context = client;
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	/*if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}*/

	
	strcpy(msg_status,client_id);
	strcat(msg_status," está online");

	//printf("%s\n",msg_status);
	pubmsg.payload = msg_status;
	pubmsg.payloadlen = strlen(msg_status);	

	MQTTAsync_sendMessage(client, "online", &pubmsg, &opts);

	strcpy(msg_conexao,client_id);
	strcat(msg_conexao," deseja enviar mensagem");


	pubmsg.payload = msg_conexao;
	pubmsg.payloadlen =strlen(msg_conexao); 
	MQTTAsync_sendMessage(client, "client1_control", &pubmsg, &opts);

	MQTTAsync_subscribe(client, "online", QOS, &opts);//envio teste
	printf("teste chegou até aquiiii hoje....\n");                                                     
	
}

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m)
{
	/* not expecting any messages */
	printf("teste entrou aqui");
	return 1;
}

int main(int argc, char* argv[])
{
	pthread_create(&(threads[0]),NULL,received,NULL);
	int timestamp = (int)time(NULL);
	sprintf(client_id, "%d", timestamp); // converter int para string
	strcat(topic_control,client_id);


	//printf("topic_control name : %s \n",topic_control);
	if ((rc = MQTTAsync_create(&client, ADDRESS,client_id , MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	if ((rc = MQTTAsync_setCallbacks(client, client, connlost, messageArrived, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to set callback, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	printf("Your ID is %s \n",client_id);
	//menu();

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;

	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	while (!finished)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	MQTTAsync_destroy(&client);
 	return rc;
}
  
