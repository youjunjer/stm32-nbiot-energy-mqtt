# 06_nbiot_mqtt_hello

Auto run sequence at boot:
1. ATI
2. AT+CEREG?
3. AT+EDNS="mqttgo.io"
4. AT+EMQNEW=218.161.43.149,1883,60000,1024
5. AT+EMQCON=0,3.1,"6545451212",60000,0,0,"",""
6. AT+EMQPUB=0,yourTopic,0,0,0,10,68656c6c6f
7. AT+EMQDISCON=0

Open COM13 @115200 to view step results.
