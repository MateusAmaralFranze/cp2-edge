🛰️ Sistema de Monitoramento Ambiental com Arduino Uno

📘 Sumário
Visão Geral

Objetivos do Projeto

Funcionamento Geral

Componentes e Materiais

Esquema de Montagem e Conexões

Definições de Alarme e Estados

Fluxo de Execução do Sistema

Estrutura de Dados e EEPROM

Detalhamento das Bibliotecas

Possíveis Melhorias e Expansões

Licença e Créditos

📌 Visão Geral
Este projeto trata-se de um sistema embarcado construído sobre a plataforma Arduino Uno, com o propósito de realizar a coleta, análise, alerta e persistência de dados ambientais em tempo real. Ideal para controle de microambientes como adegas, estufas, data centers ou áreas sensíveis à temperatura, umidade e iluminação.

🎯 Objetivos do Projeto
Monitorar em tempo real três parâmetros ambientais: temperatura, umidade e luminosidade.

Alertar visual e sonoramente condições fora dos padrões ideais.

Registrar automaticamente os eventos ambientais com carimbo de data/hora.

Manter dados persistentes em EEPROM mesmo após desligamentos.

Exibir dados em interface de usuário simples via display LCD 16x2.

⚙️ Funcionamento Geral
Coleta de Dados: sensores DHT22 (digital) e LDR (analógico).

Classificação de Estado:

Estado Normal

Estado de Atenção

Estado Crítico

Atuação:

LED verde para normalidade

LED amarelo e buzzer intermitente para atenção

LED vermelho e buzzer contínuo para estado crítico

Interface LCD: Exibe os valores e mensagens de status.

EEPROM:

Validação por assinatura (0xAB)

Registro com RTC_DS1307 de data/hora

Cada log contém o estado do ambiente e os valores dos sensores.

📦 Componentes e Materiais
Componente	Descrição Técnica	Pinos Arduino
Arduino Uno	Microcontrolador ATmega328P	-
DHT22	Sensor de Temperatura e Umidade (preciso)	D2 (DATA)
LDR	Sensor resistivo de luz + resistor de 10kΩ	A0 (entrada analógica)
LCD 16x2 I2C	Display com interface simplificada	SDA: A4, SCL: A5
RTC DS1307	Relógio de tempo real com cristal e bateria	SDA: A4, SCL: A5
LED Verde	Indicação de ambiente estável	D6
LED Amarelo	Indicação de estado de atenção	D7
LED Vermelho	Indicação de estado crítico	D8
Buzzer Piezoelétrico	Alarme sonoro (ativo)	D9 (PWM)
Protoboard, jumpers, etc.	Infraestrutura de montagem	-

🖼️ Esquema de Montagem e Conexões
O circuito é alimentado por 5V da porta USB. O barramento I2C é compartilhado entre o LCD e o RTC, sendo uma solução eficiente para economia de pinos. Os LEDs possuem resistores limitadores de corrente (220Ω ou 330Ω). O LDR está em divisor de tensão com resistor de 10kΩ.

Importante: o módulo RTC deve conter uma bateria de célula moeda (CR2032) para manter o tempo quando o Arduino é desligado.

🚨 Definições de Alarme e Estados
Os limites estão definidos por diretivas #define no código. Os três estados possíveis são:

Estado Normal (LED verde):
Temperatura entre 10 e 35 °C

Umidade entre 30% e 70%

Luminosidade abaixo de 800

Estado de Atenção (LED amarelo + alarme intermitente):
Temperatura: 35-40 °C ou 5-10 °C

Umidade: 20-30% ou 70-80%

Luminosidade: 800-900

Estado Crítico (LED vermelho + alarme contínuo):
Temperatura ≥ 40 °C ou ≤ 5 °C

Umidade ≥ 80% ou ≤ 20%

Luminosidade ≥ 900

🧭 Fluxo de Execução do Sistema
mermaid
Copiar
Editar
graph TD;
    A[Início] --> B[Setup de periféricos e EEPROM]
    B --> C[Loop principal]
    C --> D[Leitura de sensores]
    D --> E[Análise de estado ambiental]
    E --> F[Acionamento de LEDs e buzzer]
    E --> G[Exibição no LCD]
    E --> H[Registro em EEPROM se alerta]
    H --> C
🧠 Estrutura de Dados e EEPROM
Inicialização:
EEPROM recebe assinatura (0xAB) na primeira execução

Endereços são definidos para:

Assinatura

Contador de logs

Dados sequenciais

Log Salvo:
Cada evento salva na EEPROM:

Timestamp (RTC)

Temperatura, umidade e luminosidade

Estado ambiental (normal, atenção, crítico)

Exemplo:

yaml
Copiar
Editar
[23/05/2025 18:12:01] Estado: CRÍTICO | Temp: 42.0°C | Umid: 85% | Luz: 930
 Detalhamento das Bibliotecas
Biblioteca	Finalidade
LiquidCrystal_I2C	Interface com o display LCD via barramento I2C
DHT.h	Leitura dos sensores DHT22
RTClib.h	Controle e leitura do RTC (DS1307/DS3231)
EEPROM.h	Leitura e escrita de dados não voláteis
Wire.h	Comunicação I2C interna

🚀 Possíveis Melhorias e Expansões
Gravação em cartão SD com hora e leitura completa.

Envio para servidor online via módulo LoRa, Wi-Fi ou GSM.

Interface web com gráficos usando ESP8266/ESP32.

Controle remoto por Bluetooth ou app Android.

Reconfiguração dos limites via Serial ou botão físico.

Atualização automática de firmware (OTA).

