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
int timestamp_online[TAM];
int posicao_lista = 0;
char topic_control[TAM]= "control_"; 
char client_id[12];
char msg_status[TAM];
char msg_conexao[TAM];

void set_online(MQTTAsync client);



MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

//void menu(); //menu de opções

char *split_string(char *msg){
	char str[TAM];
	strcpy(str, msg);

    char* temp = 0;
    char** result = 0;
    unsigned int tamanho = 0;

    temp = strtok(str, " ");
    
    if (temp) {
        result = malloc( (tamanho + 1) * sizeof(char**));
        result[tamanho++] = temp;
    }

    while ( (temp = strtok(0, " ")) != 0 ) {
  		result = realloc(result, (tamanho + 1) * sizeof(char**));
        result[tamanho++] = temp;
    }

	

	char *split = result[0];

    if (tamanho > 0){
		free(result);
	}

	return split;
}

void criar_grupo(void * client){ // Criar grupo (não completa)
	
	char * group_name;
	printf("Insira o nome do grupo");
	fgets(group_name,10,stdin);
	strcat(group_name,"_GROUP");
	MQTTAsync_sendMessage(client, "GROUP", &pubmsg, &opts);
}

void * send_message(void *client){ //enviar mensagem(falta adicionar o tópico qual referente será enviada)

	char message[TAM];
	char topic_control_other[TAM] = "control_";
	char tsl_string[TAM];

	printf("\nUsuários online: \n");
	for (int i = 0; i < TAM; i++){
		if(timestamp_online[i] != 0 )
			printf("%d -> %d\n", i+1, timestamp_online[i]);
	}

	printf("Selecione o usuário para abrir o chat: ");
	int opc = 0;

	scanf("%d", &opc);
	
	opc--;

	//printf("opc; %d\n", opc);

	sprintf(tsl_string, "%d", timestamp_online[opc]); //converter int para string
	strcat(topic_control_other, tsl_string);

	pubmsg.payload = strcat(client_id," deseja enviar mensagem");
	pubmsg.payloadlen = strlen(pubmsg.payload);
	printf("%s",topic_control_other);
	MQTTAsync_sendMessage(client, topic_control_other, &pubmsg, &opts); //envia a mensagem

	printf("topico outro = %s\n", topic_control_other);

	printf("Mensagem: ");
	getchar();
	fgets(message, TAM, stdin);
	
	pubmsg.payload = message;
	pubmsg.payloadlen = strlen(message);
	MQTTAsync_sendMessage(client, topic_control_other, &pubmsg, &opts); //envia a mensagem
}

char aceitar_contato(char * msg, void *context){ //função para aceitar o recebimento de msg
	char res;
	char topic_chat[TAM];
	char s_idclient[TAM];
	char topic_control_other[TAM];
	char msg_resposta[TAM];
	
	printf("Deseja aceitar mensagens deste contato? S ou N");
	scanf("%c",&res);

	if (res == 'S' || res == 's'){
		printf("Contato aceito\n");

		char *split = split_string(msg);
	
		sprintf(s_idclient, "%d", timestamp_online[0]);

		strcpy(topic_chat, split);
		strcat(topic_chat, "_");
		strcat(topic_chat, s_idclient);
		strcat(topic_chat, "_Chat");

		printf("topic_chat: %s\n", topic_chat);
		
		strcpy(topic_control_other, "control_");
		strcat(topic_control_other, split);

		printf("control mlk: %s\n", topic_control_other);
		
		strcpy(msg_resposta, topic_chat);
		strcat(msg_resposta, " aceito conexão");
		
		pubmsg.payload = msg_resposta; //seta a mensagem a ser enviada para a string mg_status
		pubmsg.payloadlen = strlen(msg_resposta);	//seta o tamanho da mensagem
		MQTTAsync_sendMessage(context, topic_control_other, &pubmsg, &opts); //envia a mensagem
		MQTTAsync_subscribe(context, topic_chat, QOS, &opts);
	}

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

	if(strstr(message->payload,"está online")){ //verifica se é mensagem de status (VERIFICAR VALIDAÇÃO POSTERIORMENTE
		printf("Contato online encontrado!\n");
	}else if(strstr(message->payload," deseja enviar mensagem")){ //recebeu solicitação de bate papo
		printf("Usuário deseja conectar-se!!\n");
		res = aceitar_contato(message->payload, context);
		//if(res=='s') printf("conectado!");
	}else if(strstr(message->payload,"MR: ")) {
		printf("Message arrived\n");
		printf("     topic: %s\n", topicName);
		printf("   	message: %.*s\n", message->payloadlen, (char*)message->payload);
	}else if(strstr(message->payload," aceito conexão")) {
		printf("metade ja foi\n");
		printf("%.*s\n", message->payloadlen, (char*)message->payload);
		
		char *topico_chat = split_string((char*)message->payload);
		printf("topico_chat = %s\n", topico_chat);
		MQTTAsync_subscribe(context, "1646875206_1646875207_Chat", QOS, &opts);		//ver caractere final, deve ta bugado

	}else{
		timestamp_online[posicao_lista] = atoi(message->payload);
		//printf("id encontrado: %d\n", timestamp_online[posicao_lista]);
		posicao_lista++;
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

	//MQTTAsync_subscribe(client, "client1_control", QOS, &opts);
	MQTTAsync_subscribe(client, "online", QOS, &opts);
	//printf("topic control = %s", topic_control);
	MQTTAsync_subscribe(client, topic_control, QOS, &opts);

	set_online(client);


}

void set_online(MQTTAsync client){

	strcpy(msg_status,client_id);//insere o id (obtido pelo timestamp) na nova string
	strcat(msg_status," está online\n"); // concatena as strings
	//printf("%s",msg_status); // teste da mensagem id do usuario + está online

	pubmsg.payload = msg_status; //seta a mensagem a ser enviada para a string mg_status
	pubmsg.payloadlen = strlen(msg_status);	//seta o tamanho da mensagem
	MQTTAsync_sendMessage(client, "online", &pubmsg, &opts); //envia a mensagem

	pubmsg.payload = client_id; //seta a mensagem a ser enviada para a string mg_status
	pubmsg.payloadlen = strlen(pubmsg.payload);	//seta o tamanho da mensagem
	MQTTAsync_sendMessage(client, "online", &pubmsg, &opts); //envia a mensagem

}


int main(int argc, char* argv[])
{
	

	for (int i = 0; i < TAM; i++){
		timestamp_online[i] = 0;
	}
	
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
	printf("you control topic is %s\n",topic_control);

	//send_message(client);
	while (!subscribed && !finished)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	if (finished)
		goto exit;

	char opcao[10];
	int opc = 0;

	do 
	{
		printf("\n1 - Enviar mensagem\n");
		printf("2 - Listar usuários online\n");
		opc = 0;
		fgets(opcao, 10, stdin);

		opc = atoi(opcao);

		switch (opc){
			case 1:
				send_message(client);
				break;
			case 2:
				printf("\nUsuários online: \n");
				for (int i = 0; i < TAM; i++){
					if(timestamp_online[i] != 0 )
						printf("%d -> %d\n", i+1, timestamp_online[i]);
				}

				printf("\n----------------------\n");
				
				break;
		}
		
	} while (strcmp(opcao, "q")!=0);

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
