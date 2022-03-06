#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include <time.h>
#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

#define ADDRESS     "tcp://localhost:1883"
#define QOS         1
#define TIMEOUT     10000L
#define TAM 50

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

char topic_control[50]= "control_"; 
char client_id[12];
char msg_status[TAM];
char msg_conexao[TAM];

void set_online(MQTTAsync client);


MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

void menu(); //menu de opções

void criar_grupo(void * client){ // Criar grupo (não completa)
	
	char * group_name;
	printf("Insira o nome do grupo");
	fgets(group_name,10,stdin);
	MQTTAsync_sendMessage(client, "online", &pubmsg, &opts);
}

void * send_message(void *client){ //enviar mensagem(falta adicionar o tópico qual referente será enviada)

	char * message;
	printf("Mensagem:\n");
	scanf("%s",message);

	pubmsg.payload = message;
	pubmsg.payloadlen = strlen(message);

	MQTTAsync_sendMessage(client, "online", &pubmsg, &opts); //envia a mensagem
}

char aceitar_contato(char * msg){ //função para aceitar o recebimento de msg
	char res;
	printf("Deseja aceitar mensagens deste contato?");
	scanf("%c",&res);
	return res;
}


void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	if (cause)
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


int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
	char * user;
	char res;
	int i=0,j=0;


    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   	message: %.*s\n", message->payloadlen, (char*)message->payload);

	if(strstr(message->payload,"está online")){ //verifica se é mensagem de status
		printf("Contato online encontrado!\n");
	}

	if(strstr(message->payload,"deseja enviar mensagem")){ //recebeu solicitação de bate papo
		printf("Usuário deseja conectar encontrado!!\n");
		res = aceitar_contato(message->payload);
		if(res=='s') printf("conectado!");
	}

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Disconnect failed, rc %d\n", response->code);
	disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Subscribe failed, rc %d\n", response->code);
	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response->code);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	//MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
//	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	int rc;

	printf("Successful connection\n");
	
	/*printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, client_id, QOS);*/


	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;
	/*if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}*/

	MQTTAsync_subscribe(client, "client1_control", QOS, &opts);
	MQTTAsync_subscribe(client, "online", QOS, &opts);

	set_online(client);


}

void set_online(MQTTAsync client){

	strcpy(msg_status,client_id);//insere o id (obtido pelo timestamp) na nova string
	strcat(msg_status," está online\n"); // concatena as strings
	//printf("%s",msg_status); // teste da mensagem id do usuario + está online

	pubmsg.payload = msg_status; //seta a mensagem a ser enviada para a string mg_status
	pubmsg.payloadlen = strlen(msg_status);	//seta o tamanho da mensagem
	MQTTAsync_sendMessage(client, "online", &pubmsg, &opts); //envia a mensagem

	pubmsg.payload = "123 deseja enviar mensagem"; //seta a mensagem a ser enviada para a string mg_status
	pubmsg.payloadlen = strlen(pubmsg.payload);	//seta o tamanho da mensagem

	MQTTAsync_sendMessage(client, "online", &pubmsg, &opts); //envia a mensagem

}


int main(int argc, char* argv[])
{
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	int rc;
	int ch;

	if ((rc = MQTTAsync_create(&client, ADDRESS, client_id, MQTTCLIENT_PERSISTENCE_NONE, NULL))
			!= MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto exit;
	}

	if ((rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to set callbacks, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	opts.context = client;
	pubmsg.qos = QOS;
	pubmsg.retained = 0;


	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	int timestamp = (int)time(NULL);
	sprintf(client_id, "%d", timestamp); //converter int para string
	strcat(topic_control,client_id); //gerando tópico de controle
	
	printf("Olá usuário, seu id é %s\n",client_id);
	//printf("you control topic is %s\n",topic_control);
	
	//send_message(client);
	while (!subscribed && !finished)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	if (finished)
		goto exit;

	do 
	{
		ch = getchar();
	} while (ch!='Q' && ch != 'q');

	disc_opts.onSuccess = onDisconnect;
	disc_opts.onFailure = onDisconnectFailure;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}
 while (!disc_finished)
 	{
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif
 	}

destroy_exit:
	MQTTAsync_destroy(&client);
exit:
 	return rc;
}
