#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // Biblioteca para LCD I2C
#include <DHT.h>
#include <RTClib.h>
#include <EEPROM.h>

#define DHTPIN 2       // Pino do sensor DHT11
#define DHTTYPE DHT11  // Tipo do sensor DHT
#define LDR_PIN A0     // Pino do sensor de luminosidade (LDR)
#define LED_VERMELHO 8 // Pino do LED vermelho (alerta crítico)
#define LED_AMARELO 7  // Pino do LED amarelo (alerta de atenção)
#define LED_VERDE 6    // Pino do LED verde (status normal)
#define BUZZER_PIN 9   // Pino do buzzer

#define TEMP_CRITICA_MIN    5
#define TEMP_CRITICA_MAX    40
#define TEMP_ATENCAO_MIN    10
#define TEMP_ATENCAO_MAX    35

#define UMID_CRITICA_MIN    20
#define UMID_CRITICA_MAX    90
#define UMID_ATENCAO_MIN    30
#define UMID_ATENCAO_MAX    80

#define LUZ_CRITICA_MAX     900  
#define LUZ_ATENCAO_MAX     700  // ajuste conforme seu sensor

#define ALARME_FREQ_ALTA    2000
#define ALARME_FREQ_BAIXA   1000
#define ALARME_CRITICO_DURACAO   500
#define ALARME_ATENCAO_DURACAO   1000

// Definições para EEPROM
#define EEPROM_SIGNATURE 0xAB  // Assinatura para verificar se a EEPROM foi inicializada
#define EEPROM_SIGNATURE_ADDR 0 // Endereço da assinatura na EEPROM
#define EEPROM_COUNT_ADDR 1     // Endereço do contador de logs na EEPROM
#define EEPROM_DATA_START 4     // Endereço inicial dos dados na EEPROM
#define MAX_LOGS 20             // Número máximo de logs armazenados

// Definições para o alarme sonoro
#define ALARME_FREQ_ALTA 2000   // Frequência alta do alarme (Hz)
#define ALARME_FREQ_BAIXA 1000  // Frequência baixa do alarme (Hz)
#define ALARME_ATENCAO_DURACAO 500  // Duração de cada tom do alarme de atenção (ms)
#define ALARME_CRITICO_DURACAO 300  // Duração de cada tom do alarme crítico (ms)

LiquidCrystal_I2C lcd(0x27, 16, 2);

DHT dht(DHTPIN, DHTTYPE);

RTC_DS1307 rtc;

// Variáveis para armazenar os valores dos sensores
float temperatura = 0.0;
float umidade = 0.0;
int luminosidade = 0;
int luminosidadeMapeada = 0;

// Variáveis para controle de tempo
unsigned long ultimaLeitura = 0;
unsigned long ultimoLog = 0;
unsigned long ultimaAtualizacaoLCD = 0;
unsigned long ultimoAlarme = 0;
int enderecoEEPROM = EEPROM_DATA_START;
int contadorLogs = 0;
bool mostrarLogo = true;
unsigned long tempoInicioLogo = 0;
bool estadoAlarme = false;
bool alarmeAtivo = false;
int nivelAlerta = 0; 

struct DadosLog {
  byte hora;
  byte minuto;
  float temperatura;
  float umidade;
  int luminosidade;
  byte nivelAlerta;
};
byte garrafa[8] = {
  B00100,
  B00100,
  B01110,
  B01110,
  B11111,
  B11111,
  B01110,
  B00000
};

byte uva[8] = {
  B01010,
  B11111,
  B11111,
  B11111,
  B11111,
  B01110,
  B00100,
  B00000
};

byte copo[8] = {
  B00000,
  B11110,
  B10010,
  B10010,
  B10010,
  B10010,
  B11111,
  B00000
};

byte termometro[8] = {
  B00100,
  B01010,
  B01010,
  B01010,
  B01110,
  B11111,
  B11111,
  B01110
};

void setup() {
  // Inicialização da comunicação serial
  Serial.begin(9600);
  Serial.println(F("Inicializando Sistema de Monitoramento Ambiental"));
  
  // Configuração dos pinos - IMPORTANTE: Configurar antes de usar
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERDE, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  
  // Teste inicial dos LEDs - Garantir que estão funcionando
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_AMARELO, HIGH);
  digitalWrite(LED_VERDE, HIGH);
  delay(1000);
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, LOW);
  delay(500);
  
  // Inicialização do LCD I2C
  lcd.init();                      // Inicializa o LCD
  lcd.backlight();                 // Liga a luz de fundo
  
  // Criar caracteres personalizados
  lcd.createChar(0, garrafa);
  lcd.createChar(1, uva);
  lcd.createChar(2, copo);
  lcd.createChar(3, termometro);
  
  // Inicialização do sensor DHT
  dht.begin();
  
  // Inicialização do RTC
  Wire.begin();
  if (!rtc.begin()) {
    Serial.println(F("RTC não encontrado!"));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Erro: RTC"));
    while (1);
  }
  
  // Configurar o RTC se não estiver funcionando
  if (!rtc.isrunning()) {
    Serial.println(F("RTC não está rodando! Ajustando para a hora da compilação..."));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Inicializar EEPROM se necessário
  inicializarEEPROM();
  
  // Mostrar logo animado da empresa
  mostrarLogoAnimado();
  tempoInicioLogo = millis();
  mostrarLogo = true;
  
  // Inicializar LEDs - Verde ligado por padrão (sistema funcionando)
  
  
  Serial.println(F("Sistema inicializado com sucesso!"));
}

void loop() {
  unsigned long tempoAtual = millis();
  
  // Mostrar logo por 3 segundos no início
  if (mostrarLogo && (tempoAtual - tempoInicioLogo < 3000)) {
    return;
  } else if (mostrarLogo) {
    mostrarLogo = false;
    lcd.clear();
  }
  
  // Leitura dos sensores a cada 2 segundos
  if (tempoAtual - ultimaLeitura >= 2000) {
    lerSensores();
    verificarAlertas();
    ultimaLeitura = tempoAtual;
  }
  
  // Atualização do LCD a cada 1 segundo
  if (tempoAtual - ultimaAtualizacaoLCD >= 1000) {
    atualizarLCD();
    ultimaAtualizacaoLCD = tempoAtual;
  }
  
  if (tempoAtual - ultimoLog >= 600000) {
    registrarLog();
    ultimoLog = tempoAtual;
  }
  
  // Controle do alarme sonoro
  if (alarmeAtivo) {
    controlarAlarme();
  }
  
  // Verificar se há comandos na porta serial
  verificarComandosSerial();
}

void lerSensores() {
  // Leitura do sensor DHT11
  float tempLeitura = dht.readTemperature();
  float umidLeitura = dht.readHumidity();
  
  // Verificar se as leituras são válidas
  if (!isnan(tempLeitura) && !isnan(umidLeitura)) {
    temperatura = tempLeitura;
    umidade = umidLeitura;
  } else {
    Serial.println(F("Falha na leitura do sensor DHT11!"));
  }
  
  // Leitura do sensor de luminosidade (LDR)
  luminosidade = analogRead(LDR_PIN);
  
  luminosidadeMapeada = map(luminosidade, 0, 1023, 100, 0);
  
  // Exibir valores no monitor serial
  Serial.print(F("Temperatura: "));
  Serial.print(temperatura);
  Serial.print(F(" °C, Umidade: "));
  Serial.print(umidade);
  Serial.print(F(" %, Luminosidade: "));
  Serial.print(luminosidade);
  Serial.print(F(" ("));
  
}

void verificarAlertas() {
  // Variáveis para controle de alertas
  bool alertaTempCritico = false;
  bool alertaUmidCritico = false;
  bool alertaLuzCritico = false;
  
  bool alertaTempAtencao = false;
  bool alertaUmidAtencao = false;
  bool alertaLuzAtencao = false;
  
  // Verificar temperatura - NÍVEL CRÍTICO
  if (temperatura < TEMP_CRITICA_MIN || temperatura > TEMP_CRITICA_MAX) {
    alertaTempCritico = true;
    Serial.print(F("ALERTA CRÍTICO "));
    Serial.println(temperatura);
  } 
  // Verificar temperatura - NÍVEL ATENÇÃO
  else if (temperatura < TEMP_ATENCAO_MIN || temperatura > TEMP_ATENCAO_MAX) {
    alertaTempAtencao = true;
    Serial.print(F("ALERTA ATENÇÃO "));
    Serial.println(temperatura);
  }
  
  // Verificar umidade - NÍVEL CRÍTICO
  if (umidade < UMID_CRITICA_MIN || umidade > UMID_CRITICA_MAX) {
    alertaUmidCritico = true;
    Serial.print(F("ALERTA CRÍTICO "));
    Serial.println(umidade);
  } 
  // Verificar umidade - NÍVEL ATENÇÃO
  else if (umidade < UMID_ATENCAO_MIN || umidade > UMID_ATENCAO_MAX) {
    alertaUmidAtencao = true;
    Serial.print(F("ALERTA ATENÇÃO "));
    Serial.println(umidade);
  }
  
  // Verificar luminosidade - NÍVEL CRÍTICO
  if (luminosidade > LUZ_CRITICA_MAX) {
    alertaLuzCritico = true;
    Serial.print(F("ALERTA CRÍTICO "));
    Serial.println(luminosidade);
  } 
  // Verificar luminosidade - NÍVEL ATENÇÃO
  else if (luminosidade > LUZ_ATENCAO_MAX) {
    alertaLuzAtencao = true;
    Serial.print(F("ALERTA ATENÇÃO "));
    Serial.println(luminosidade);
  }
  
  // Determinar o nível de alerta geral e acionar os LEDs diretamente
  if (alertaTempCritico || alertaUmidCritico || alertaLuzCritico) {
    // Nível crítico - LED vermelho
    digitalWrite(LED_VERMELHO, HIGH);
    digitalWrite(LED_AMARELO, LOW);
    digitalWrite(LED_VERDE, LOW);
    nivelAlerta = 2;
    alarmeAtivo = true;
    Serial.println(F("ALERTA CRÍTICO ATIVADO "));
  } 
  else if (alertaTempAtencao || alertaUmidAtencao || alertaLuzAtencao) {
    // Nível de atenção - LED amarelo
    digitalWrite(LED_VERMELHO, LOW);
    digitalWrite(LED_AMARELO, HIGH);
    digitalWrite(LED_VERDE, LOW);
    nivelAlerta = 1;
    alarmeAtivo = true;
    Serial.println(F("ALERTA ATENÇÃO ATIVADO "));
  } 
  else {
    // Normal - apenas LED verde
    digitalWrite(LED_VERMELHO, LOW);
    digitalWrite(LED_AMARELO, LOW);
    digitalWrite(LED_VERDE, HIGH);
    nivelAlerta = 0;
    alarmeAtivo = false;
    noTone(BUZZER_PIN);
    Serial.println(F("SISTEMA NORMAL "));
  }
}

void controlarAlarme() {
  unsigned long tempoAtual = millis();
  int duracaoAlarme = (nivelAlerta == 2) ? ALARME_CRITICO_DURACAO : ALARME_ATENCAO_DURACAO;
  
  // Alternar entre tons a cada duracaoAlarme milissegundos
  if (tempoAtual - ultimoAlarme >= duracaoAlarme) {
    ultimoAlarme = tempoAtual;
    
    if (estadoAlarme) {
      // Tom alto
      tone(BUZZER_PIN, ALARME_FREQ_ALTA);
    } else {
      // Tom baixo
      tone(BUZZER_PIN, ALARME_FREQ_BAIXA);
    }
    
    // Inverter estado para próxima vez
    estadoAlarme = !estadoAlarme;
  }
}

void atualizarLCD() {
  // Obter hora atual
  DateTime agora = rtc.now();
  
  // Primeira linha: Temperatura e Umidade
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("T:"));
  lcd.print(temperatura, 1);
  lcd.print(F("C  "));
  lcd.print(F("U:"));
  lcd.print(umidade, 1);
  
  // Segunda linha: Luminosidade e Hora
  lcd.setCursor(0, 1);
  lcd.print(F("L:"));
  lcd.print(luminosidade);
  lcd.print(F("   "));

  
  
  // Formatar hora (HH:MM)
  if (agora.hour() < 10) lcd.print(F("0"));
  lcd.print(agora.hour());
  lcd.print(F(":"));
  if (agora.minute() < 10) lcd.print(F("0"));
  lcd.print(agora.minute());
  
  // Se houver alarme ativo, piscar o backlight do LCD
  if (alarmeAtivo && estadoAlarme) {
    lcd.noBacklight();
  } else {
    lcd.backlight();
  }
}

void inicializarEEPROM() {
  // Verificar se a EEPROM já foi inicializada
  byte assinatura = EEPROM.read(EEPROM_SIGNATURE_ADDR);
  
  if (assinatura != EEPROM_SIGNATURE) {
    // EEPROM não inicializada, fazer inicialização
    Serial.println(F("Inicializando EEPROM..."));
    
    // Escrever assinatura
    EEPROM.write(EEPROM_SIGNATURE_ADDR, EEPROM_SIGNATURE);
    
    // Zerar contador de logs
    EEPROM.write(EEPROM_COUNT_ADDR, 0);
    EEPROM.write(EEPROM_COUNT_ADDR + 1, 0);
    
    contadorLogs = 0;
    enderecoEEPROM = EEPROM_DATA_START;
  } else {
    // EEPROM já inicializada, ler contador de logs
    contadorLogs = EEPROM.read(EEPROM_COUNT_ADDR) | (EEPROM.read(EEPROM_COUNT_ADDR + 1) << 8);
    
    Serial.print(F("EEPROM já inicializada. Logs armazenados: "));
    Serial.println(contadorLogs);
    
    // Calcular endereço para próximo log
    enderecoEEPROM = EEPROM_DATA_START + (contadorLogs % MAX_LOGS) * sizeof(DadosLog);
  }
}

void registrarLog() {
  // Obter hora atual
  DateTime agora = rtc.now();
  
  // Criar estrutura de dados para o log
  DadosLog dados;
  dados.hora = agora.hour();
  dados.minuto = agora.minute();
  dados.temperatura = temperatura;
  dados.umidade = umidade;
  dados.luminosidade = luminosidadeMapeada;
  dados.nivelAlerta = nivelAlerta;
  
  // Calcular endereço para o log (sistema circular)
  enderecoEEPROM = EEPROM_DATA_START + (contadorLogs % MAX_LOGS) * sizeof(DadosLog);
  
  // Escrever dados na EEPROM
  EEPROM.put(enderecoEEPROM, dados);
  
  // Incrementar contador de logs
  contadorLogs++;
  
  // Atualizar contador na EEPROM
  EEPROM.write(EEPROM_COUNT_ADDR, contadorLogs & 0xFF);
  EEPROM.write(EEPROM_COUNT_ADDR + 1, (contadorLogs >> 8) & 0xFF);
  
  Serial.print(F("Log #"));
  Serial.print(contadorLogs);
  Serial.print(F(" registrado às "));
  Serial.print(agora.hour());
  Serial.print(F(":"));
  Serial.print(agora.minute());
  Serial.print(F(" - Nível de alerta: "));
  
  switch (nivelAlerta) {
    case 0:
      Serial.println(F("Normal"));
      break;
    case 1:
      Serial.println(F("Atenção"));
      break;
    case 2:
      Serial.println(F("CRÍTICO"));
      break;
  }
}

void mostrarLogoAnimado() {
  // Animação de entrada
  for (int i = 0; i < 16; i++) {
    lcd.clear();
    lcd.setCursor(i, 0);
    lcd.write(0); // Garrafa
    lcd.setCursor(15-i, 1);
    lcd.write(2); // Copo
    delay(100);
  }
  
  delay(500);
  lcd.clear();
  
  // Animação pulsante
  for (int j = 0; j < 3; j++) {
    // Mostrar logo grande
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.write(0); // Garrafa
    lcd.setCursor(5, 0);
    lcd.print(F("WELCOME"));
    lcd.setCursor(12, 0);
    lcd.write(1); // Uva
    
    lcd.setCursor(2, 1);
    lcd.print(F("COREX"));
    lcd.setCursor(13, 1);
    lcd.write(2); // Copo
    delay(700);
    
    // Mostrar logo pequeno
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.write(0); // Garrafa
    lcd.setCursor(6, 0);
    lcd.print(F("-___-"));
    lcd.setCursor(11, 0);
    lcd.write(1); // Uva
    
    lcd.setCursor(3, 1);
    lcd.print(F("COREX"));
    lcd.setCursor(12, 1);
    lcd.write(2); // Copo
    delay(700);
  }
  
  // Transição para o sistema de monitoramento
  for (int i = 0; i < 16; i++) {
    lcd.clear();
    
    // Rolar o logo para a esquerda
    if (i < 16) {
      lcd.setCursor(3-i, 0);
      lcd.write(0); // Garrafa
      lcd.setCursor(6-i, 0);
      lcd.print(F("--"));
      lcd.setCursor(12-i, 0);
      lcd.write(1); // Uva
      
      lcd.setCursor(2-i, 1);
      lcd.print(F("COREX"));
      lcd.setCursor(13-i, 1);
      lcd.write(2); // Copo
    }
    
    // Rolar o texto de monitoramento da direita
    if (i > 0) {
      lcd.setCursor(16-i, 0);
      lcd.print(F("SISTEMA DE"));
      lcd.setCursor(16-i, 1);
      lcd.print(F("MONITORAMENTO"));
    }
    
    delay(150);
  }
  
  // Mostrar mensagem final
  delay(500);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F(" MONITORAMENTO "));
  lcd.setCursor(0, 1);
  lcd.write(3); // Termômetro
  lcd.print(F(" INICIANDO... "));
  lcd.write(3); // Termômetro
  
  // Efeito de carregamento
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, 0);
    lcd.write(255); // Bloco cheio
    delay(100);
  }
  
  delay(500);
}

void exibirLogs() {
  int totalLogs = min(contadorLogs, MAX_LOGS);
  
  Serial.println(F("=== LOGS ARMAZENADOS ==="));
  Serial.print(F("Total de logs: "));
  Serial.println(totalLogs);
  
  for (int i = 0; i < totalLogs; i++) {
    // Calcular endereço do log
    int endereco;
    
    if (contadorLogs <= MAX_LOGS) {
      // Ainda não completou o buffer circular
      endereco = EEPROM_DATA_START + i * sizeof(DadosLog);
    } else {
      // Buffer circular completo, começar do mais antigo
      int indiceInicial = contadorLogs % MAX_LOGS;
      int indice = (indiceInicial + i) % MAX_LOGS;
      endereco = EEPROM_DATA_START + indice * sizeof(DadosLog);
    }
    
    // Ler dados da EEPROM
    DadosLog dados;
    EEPROM.get(endereco, dados);
    
    // Exibir dados
    Serial.print(F("Log #"));
    Serial.print(i + 1);
    Serial.print(F(": "));
    
    // Formatar hora
    if (dados.hora < 10) Serial.print(F("0"));
    Serial.print(dados.hora);
    Serial.print(F(":"));
    if (dados.minuto < 10) Serial.print(F("0"));
    Serial.print(dados.minuto);
    
    // Exibir dados dos sensores
    Serial.print(F(", Temp: "));
    Serial.print(dados.temperatura);
    Serial.print(F("°C, Umid: "));
    Serial.print(dados.umidade);
    Serial.print(F("%, Luz: "));
    Serial.print(dados.luminosidade);
    Serial.print(F("/10"));
    
    // Exibir nível de alerta
    Serial.print(F(", Alerta: "));
    switch (dados.nivelAlerta) {
      case 0:
        Serial.print(F("Normal"));
        break;
      case 1:
        Serial.print(F("Atenção"));
        break;
      case 2:
        Serial.print(F("CRÍTICO"));
        break;
    }
    
    Serial.println();
  }
  
  Serial.println(F("======================="));
}

void limparEEPROM() {
  Serial.println(F("Limpando EEPROM..."));
  
  // Resetar assinatura e contador
  EEPROM.write(EEPROM_SIGNATURE_ADDR, 0);
  EEPROM.write(EEPROM_COUNT_ADDR, 0);
  EEPROM.write(EEPROM_COUNT_ADDR + 1, 0);
  
  // Reinicializar EEPROM
  inicializarEEPROM();
  
  Serial.println(F("EEPROM limpa com sucesso!"));
}

void verificarComandosSerial() {
  if (Serial.available() > 0) {
    char comando = Serial.read();
    
    switch (comando) {
      case 'l': // Exibir logs
      case 'L':
        exibirLogs();
        break;
        
      case 'c': // Limpar EEPROM
      case 'C':
        limparEEPROM();
        break;
        
      case 'r': // Registrar log manualmente
      case 'R':
        registrarLog();
        Serial.println(F("Log registrado manualmente"));
        break;
        
      case 'i': // Exibir informações do sistema
      case 'I':
        exibirInformacoesSistema();
        break;
        
      case 'a': // Testar alarme
      case 'A':
        testarAlarme();
        break;
        
      case 's': // Silenciar alarme
      case 'S':
        silenciarAlarme();
        break;
        
      case 'o': // Mostrar logo novamente
      case 'O':
        mostrarLogoAnimado();
        break;
        
      case 't': // Testar níveis de alerta
      case 'T':
        testarNiveisAlerta();
        break;
        
      case 'd': // Forçar valores de debug
      case 'D':
        forcarValoresDebug();
        break;
    }
    
    // Limpar buffer serial
    while (Serial.available() > 0) {
      Serial.read();
    }
  }
}

void forcarValoresDebug() {
  Serial.println(F("Forçando valores para teste de alertas..."));
  
  // Perguntar qual teste realizar
  Serial.println(F("Escolha o teste:"));
  Serial.println(F("1 - Temperatura alta (alerta crítico)"));
  Serial.println(F("2 - Umidade alta (alerta crítico)"));
  Serial.println(F("3 - Luminosidade alta (alerta crítico)"));
  Serial.println(F("4 - Temperatura média (alerta atenção)"));
  Serial.println(F("5 - Umidade média (alerta atenção)"));
  Serial.println(F("6 - Luminosidade média (alerta atenção)"));
  Serial.println(F("7 - Valores normais"));
  
  // Aguardar escolha
  while (!Serial.available()) {
    delay(100);
  }
  
  char escolha = Serial.read();
  
  switch (escolha) {
    case '1':
      temperatura = TEMP_CRITICA_MAX + 5;
      umidade = 65.0;
      luminosidade = 500;
      Serial.print(F("Temperatura forçada para: "));
      Serial.println(temperatura);
      break;
      
    case '2':
      temperatura = 15.0;
      umidade = UMID_CRITICA_MAX + 10;
      luminosidade = 500;
      Serial.print(F("Umidade forçada para: "));
      Serial.println(umidade);
      break;
      
    case '3':
      temperatura = 15.0;
      umidade = 65.0;
      luminosidade = LUZ_CRITICA_MAX + 100;
      Serial.print(F("Luminosidade forçada para: "));
      Serial.println(luminosidade);
      break;
      
    case '4':
      temperatura = TEMP_ATENCAO_MAX + 1;
      umidade = 65.0;
      luminosidade = 500;
      Serial.print(F("Temperatura forçada para: "));
      Serial.println(temperatura);
      break;
      
    case '5':
      temperatura = 15.0;
      umidade = UMID_ATENCAO_MAX + 1;
      luminosidade = 500;
      Serial.print(F("Umidade forçada para: "));
      Serial.println(umidade);
      break;
      
    case '6':
      temperatura = 15.0;
      umidade = 65.0;
      luminosidade = LUZ_ATENCAO_MAX + 1;
      Serial.print(F("Luminosidade forçada para: "));
      Serial.println(luminosidade);
      break;
      
    case '7':
      temperatura = 15.0;
      umidade = 65.0;
      luminosidade = 500;
      Serial.println(F("Valores normais restaurados"));
      break;
      
    default:
      Serial.println(F("Escolha inválida"));
      return;
  }
  
  // Atualizar LCD e verificar alertas
  atualizarLCD();
  verificarAlertas();
  
  Serial.println(F("Valores forçados aplicados. Verificando alertas..."));
}

void testarAlarme() {
  Serial.println(F("Testando alarme..."));
  
  // Testar nível de atenção
  Serial.println(F("Testando alarme de ATENÇÃO..."));
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, HIGH);
  digitalWrite(LED_VERDE, LOW);
  nivelAlerta = 1;
  alarmeAtivo = true;
  delay(3000);
  
  // Testar nível crítico
  Serial.println(F("Testando alarme CRÍTICO..."));
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, LOW);
  nivelAlerta = 2;
  alarmeAtivo = true;
  delay(3000);
  
  // Voltar ao normal
  alarmeAtivo = false;
  nivelAlerta = 0;
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, HIGH);
  noTone(BUZZER_PIN);
  Serial.println(F("Teste de alarme concluído"));
}

void testarNiveisAlerta() {
  Serial.println(F("Testando níveis de alerta..."));
  
  // Testar nível normal
  Serial.println(F("Nível NORMAL"));
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, HIGH);
  nivelAlerta = 0;
  delay(2000);
  
  // Testar nível de atenção
  Serial.println(F("Nível de ATENÇÃO"));
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, HIGH);
  digitalWrite(LED_VERDE, LOW);
  nivelAlerta = 1;
  delay(2000);
  
  // Testar nível crítico
  Serial.println(F("Nível CRÍTICO"));
  digitalWrite(LED_VERMELHO, HIGH);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, LOW);
  nivelAlerta = 2;
  delay(2000);
  
  // Voltar ao normal
  digitalWrite(LED_VERMELHO, LOW);
  digitalWrite(LED_AMARELO, LOW);
  digitalWrite(LED_VERDE, HIGH);
  nivelAlerta = 0;
  Serial.println(F("Teste de níveis concluído"));
}

void silenciarAlarme() {
  if (alarmeAtivo) {
    Serial.println(F("Alarme silenciado temporariamente"));
    alarmeAtivo = false;
    noTone(BUZZER_PIN);
    delay(30000); // Silenciar por 30 segundos
    Serial.println(F("Monitoramento de alarme reativado"));
  } else {
    Serial.println(F("Não há alarme ativo para silenciar"));
  }
}

void exibirInformacoesSistema() {
  Serial.println(F("=== INFORMAÇÕES DO SISTEMA ==="));
  Serial.print(F("Versão: 1.2\n"));
  Serial.print(F("Logs armazenados: "));
  Serial.print(contadorLogs);
  Serial.print(F(" (máximo: "));
  Serial.print(MAX_LOGS);
  Serial.println(F(")"));
  
  Serial.print(F("Memória EEPROM total: "));
  Serial.print(EEPROM.length());
  Serial.println(F(" bytes"));
  
  Serial.print(F("Memória EEPROM utilizada: "));
  Serial.print(min(contadorLogs, MAX_LOGS) * sizeof(DadosLog) + EEPROM_DATA_START);
  Serial.println(F(" bytes"));
  
  Serial.println(F("Limites de alerta - NÍVEL DE ATENÇÃO:"));
  Serial.print(F("- Temperatura: "));
  Serial.print(TEMP_ATENCAO_MIN);
  Serial.print(F("°C a "));
  Serial.print(TEMP_ATENCAO_MAX);
  Serial.println(F("°C"));
  
  Serial.print(F("- Umidade: "));
  Serial.print(UMID_ATENCAO_MIN);
  Serial.print(F("% a "));
  Serial.print(UMID_ATENCAO_MAX);
  Serial.println(F("%"));
  
  Serial.print(F("- Luminosidade máxima: "));
  Serial.println(LUZ_ATENCAO_MAX);
  
  Serial.println(F("Limites de alerta - NÍVEL CRÍTICO:"));
  Serial.print(F("- Temperatura: "));
  Serial.print(TEMP_CRITICA_MIN);
  Serial.print(F("°C a "));
  Serial.print(TEMP_CRITICA_MAX);
  Serial.println(F("°C"));
  
  Serial.print(F("- Umidade: "));
  Serial.print(UMID_CRITICA_MIN);
  Serial.print(F("% a "));
  Serial.print(UMID_CRITICA_MAX);
  Serial.println(F("%"));
  
  Serial.print(F("- Luminosidade máxima: "));
  Serial.println(LUZ_CRITICA_MAX);
  
  // Exibir data e hora atual
  DateTime agora = rtc.now();
  Serial.print(F("Data e hora atual: "));
  Serial.print(agora.year());
  Serial.print(F("/"));
  Serial.print(agora.month());
  Serial.print(F("/"));
  Serial.print(agora.day());
  Serial.print(F(" "));
  Serial.print(agora.hour());
  Serial.print(F(":"));
  Serial.print(agora.minute());
  Serial.print(F(":"));
  Serial.println(agora.second());
  
  // Exibir estado atual dos LEDs
  Serial.println(F("Estado atual dos LEDs:"));
  Serial.print(F("- LED Verde: "));
  Serial.println(digitalRead(LED_VERDE) ? F("LIGADO") : F("DESLIGADO"));
  Serial.print(F("- LED Amarelo: "));
  Serial.println(digitalRead(LED_AMARELO) ? F("LIGADO") : F("DESLIGADO"));
  Serial.print(F("- LED Vermelho: "));
  Serial.println(digitalRead(LED_VERMELHO) ? F("LIGADO") : F("DESLIGADO"));
  
  Serial.print(F("Status do alarme: "));
  if (alarmeAtivo) {
    Serial.print(F("ATIVO - Nível "));
    Serial.println(nivelAlerta == 2 ? F("CRÍTICO") : F("de ATENÇÃO"));
  } else {
    Serial.println(F("Inativo"));
  }
  
  Serial.println(F("============================="));
}