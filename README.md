# IoT-link

EcoSmart Container - Sistema Inteligente de Gestión de Residuos (Primera versión)

Un contenedor IoT que combina hardware y software para incentivar el reciclaje responsable y optimizar la gestión de residuos. Incluye una interfaz web en React, firmware para ESP8266 y un sistema de recompensas por depósito de basura.

## 1.- Características Principales
Monitoreo en Tiempo Real : Nivel de basura, temperatura, humedad, batería y detección de fuego
Control Automático : Apertura/cierre de tapa mediante motor paso a paso al presionar un botón
Sistema de Recompensas : Tokens por depósito de basura 
Alertas Inteligentes : Notificaciones por contenedor lleno (>85%), detección de fuego o batería baja (<20%)
Integración IoT : Comunicación WiFi entre ESP8266 y servidor local para actualización de datos cada 10 segundos

## 2.- Componentes del Sistema
###  Hardware
1.-**ESP8266 (NodeMCU) : Controlador central con conectividad WiFi**

- <img width="650" height="400" alt="image" src="https://github.com/user-attachments/assets/198b368a-b96c-4781-9f78-30c658077bd6" />
#### Sensores:

- HC-SR04: Medición de nivel de basura
<img width="350" height="150" alt="image" src="https://github.com/user-attachments/assets/b2918082-72a5-481e-8319-8629b48f3816" />
  
- DHT11: Temperatura y humedad
<img width="380" height="150" alt="image" src="https://github.com/user-attachments/assets/e896298c-63bf-4194-bf46-54d4e9df2986" />

- Sensor de llama: Detección de incendios
<img width="380" height="150" alt="image" src="https://github.com/user-attachments/assets/1d913738-0b07-442b-8b7a-6f78324cb8e2" />

- Sensor IR: Confirmación de depósito de basura
<img width="380" height="150" alt="image" src="https://github.com/user-attachments/assets/fb717131-8468-49dc-aa9e-fd52a94fa335" />

- ADC (A0): Monitoreo de nivel de batería: Se usa un simple divisor resistivo para tener una salida de hasta un 1V cuando Vbat=12.1V
<img width="380" height="150" alt="image" src="https://github.com/user-attachments/assets/6ded521b-0cc4-4285-b039-c2eab0452e97" />

- Motor paso a paso: Control de tapa (512 pasos para abrir/cerrar)
<img width="380" height="150" alt="image" src="https://github.com/user-attachments/assets/828d5c59-3525-4aa2-8f58-4b99ab91a4f5" />

- Botón físico: Activación manual de la tapa
<img src="https://github.com/user-attachments/assets/ba71f89e-5dd0-402f-a7eb-82f297728329" alt="image" width="200">

- ESP32-24S028 como interfaz para el usuario 
<img src="https://github.com/user-attachments/assets/c0fa416c-1a86-4f38-bcc4-5e670555a82a" alt="image" width="320">

- Sistema de alimentación y carga de batería
<img width="400" height="200" alt="image" src="https://github.com/user-attachments/assets/a8e28b12-d470-401b-bbdd-923e2be59ed3" />


### Software
- **Interfaz Web (React): Dashboard visual con gráficos y alertas**
<img width="400" height="180" alt="image" src="https://github.com/user-attachments/assets/78cb2d2c-9e51-4c53-a8a0-2dc2963fedc7" />

- **Backend (Node.js/Express): Servidor local para almacenamiento y envío de datos**
<img width="350" height="160" alt="image" src="https://github.com/user-attachments/assets/3dfae9bc-86f1-4ab6-805d-35187ba373e5" />

- **Firmware (Arduino): Lógica de sensores, motor y comunicación WiFi. LOGS SERIALES**
<img width="450" height="120" alt="image" src="https://github.com/user-attachments/assets/4078482b-0d9e-4c06-80a4-c110a549ea17" />


## 3.- Funcionamiento 
El ESP8266 lee sensores y controla el motor paso a paso
Los datos se envían a un servidor local *"mi servidor local (http://192.168.43.42:3000/data)"*

La interfaz web en React obtiene los datos y actualiza la UI cada 5 segundos

Los usuarios ganan tokens al depositar basura (detectado por el sensor IR)

La interfaz de usuario recibe datos para mostrarlos, donde se podra acceder a tres menús pequeños .

## 4.- Contribuciones
Si tienes alguna duda o consulta no dudes en hacerlo. Reporta errores, solicita mejoras o propón nuevas funcionalidades en las issues del repositorio.
