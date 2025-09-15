/*
 * Sistema de Elevação com Descida Controlada - NEMA 23 + TB6600
 * 
 * Funcionamento:
 * 1. Motor sobe por tempo determinado com velocidade controlada
 * 2. Pausa no topo com motor travado
 * 3. Motor desce com velocidade controlada (direção oposta)
 * 4. Repete o ciclo
 */

// ==================== PINOS DO ESP32 ====================
const int STEP_PIN = 25;  // Pulsos para o TB6600
const int DIR_PIN = 26;   // Direção do motor
const int ENA_PIN = 27;   // Habilita/desabilita motor

// ==================== VELOCIDADES ====================
const int VELOCIDADE_SUBIDA = 4000;     // Microssegundos entre pulsos (subida)
const int VELOCIDADE_DESCIDA = 6000;    // Microssegundos entre pulsos (descida) - mais lenta para segurança
const int VELOCIDADE_INICIAL = 4000;    // Partida suave (mais lenta)

// ==================== DIREÇÕES ====================
const int SUBIR = HIGH;    // Ajuste se necessário
const int DESCER = LOW;    // Direção oposta à subida

// ==================== TEMPOS EM MILISSEGUNDOS ====================
const unsigned long TEMPO_SUBIDA = 10000;     // 10 segundos subindo
const unsigned long TEMPO_PAUSA = 5000;       // 5 segundos parado no topo
const unsigned long TEMPO_DESCIDA = 12000;    // 12 segundos descendo (mais tempo pois é mais lento)

// ==================== CONTROLE DE ESTADOS ====================
enum Estado {
  SUBINDO,
  PARADO_NO_TOPO,
  DESCENDO,        // Mudamos de QUEDA_LIVRE para DESCENDO
  PARADO_EM_BAIXO, // Novo estado: pausa embaixo antes de reiniciar
  REINICIANDO
};

Estado estadoAtual = SUBINDO;
unsigned long tempoInicio = 0;

void setup() {
  // Inicia comunicação serial para monitoramento
  Serial.begin(115200);
  Serial.println("Sistema de Elevação com Descida Controlada Iniciado");
  
  // Configura os pinos como saída
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  
  // Configura estado inicial: motor habilitado e subindo
  digitalWrite(ENA_PIN, LOW);    // Motor ligado (LOW = habilitado no TB6600)
  digitalWrite(DIR_PIN, SUBIR);  // Direção para subir
  digitalWrite(STEP_PIN, LOW);   // Pulso em estado baixo
  
  // Marca o tempo de início
  tempoInicio = millis();
  estadoAtual = SUBINDO;
  
  Serial.println("Iniciando subida controlada...");
}

void loop() {
  unsigned long tempoAtual = millis();
  unsigned long tempoDecorrido = tempoAtual - tempoInicio;
  
  switch(estadoAtual) {
    
    case SUBINDO:
      // Gera pulsos continuamente para subir com velocidade controlada
      gerarPulso(VELOCIDADE_SUBIDA);
      
      // Verifica se terminou o tempo de subida
      if(tempoDecorrido >= TEMPO_SUBIDA) {
        pararMotor();
        estadoAtual = PARADO_NO_TOPO;
        tempoInicio = tempoAtual;
        Serial.println("Parado no topo - motor travado");
      }
      break;
      
    case PARADO_NO_TOPO:
      // Motor permanece travado (sem gerar pulsos, mas energizado)
      // Apenas aguarda o tempo de pausa
      
      if(tempoDecorrido >= TEMPO_PAUSA) {
        // Prepara para descida: muda direção mas mantém motor habilitado
        prepararDescida();
        estadoAtual = DESCENDO;
        tempoInicio = tempoAtual;
        Serial.println("Iniciando descida controlada");
      }
      break;
      
    case DESCENDO:
      // Motor agora desce com velocidade controlada (direção oposta)
      gerarPulso(VELOCIDADE_DESCIDA);
      
      // Verifica se terminou o tempo de descida
      if(tempoDecorrido >= TEMPO_DESCIDA) {
        pararMotor();
        estadoAtual = PARADO_EM_BAIXO;
        tempoInicio = tempoAtual;
        Serial.println("Chegou embaixo - motor travado");
      }
      break;
      
    case PARADO_EM_BAIXO:
      // Nova pausa embaixo antes de reiniciar o ciclo
      // Isso dá tempo para o sistema se estabilizar
      
      if(tempoDecorrido >= 2000) { // 2 segundos de pausa embaixo
        estadoAtual = REINICIANDO;
        tempoInicio = tempoAtual;
        Serial.println("Preparando novo ciclo...");
      }
      break;
      
    case REINICIANDO:
      // Reabilita motor e configura para nova subida
      reiniciarCiclo();
      estadoAtual = SUBINDO;
      tempoInicio = tempoAtual;
      Serial.println("Novo ciclo - subindo novamente");
      break;
  }
  
  // Verifica se há comandos do usuário
  verificarComandos();
}

// ==================== FUNÇÕES DO MOTOR ====================

void gerarPulso(int intervalo_micros) {
  // Cria um pulso para fazer o motor dar um passo
  // Funciona igual para subida e descida, só muda a direção configurada
  digitalWrite(STEP_PIN, HIGH);
  delayMicroseconds(intervalo_micros / 2);
  digitalWrite(STEP_PIN, LOW);
  delayMicroseconds(intervalo_micros / 2);
}

void pararMotor() {
  // Para o motor mas mantém energizado (travado)
  // Útil nas pausas no topo e embaixo
  digitalWrite(STEP_PIN, LOW);
  // ENA_PIN permanece LOW = motor continua habilitado e travado
  Serial.println("Motor parado mas travado (energizado)");
}

void prepararDescida() {
  // Configura o motor para descida controlada
  // Muda apenas a direção, mantém motor habilitado
  digitalWrite(DIR_PIN, DESCER);   // Inverte direção para descer
  digitalWrite(STEP_PIN, LOW);     // Estado inicial do pulso
  // Motor permanece habilitado (ENA_PIN continua LOW)
  
  delay(100);  // Pequena pausa para o driver processar a mudança de direção
  Serial.println("Direção configurada para DESCIDA");
}

void reiniciarCiclo() {
  // Prepara motor para novo ciclo de subida
  digitalWrite(ENA_PIN, LOW);     // Garante que motor está ligado
  digitalWrite(DIR_PIN, SUBIR);   // Volta direção de subida
  digitalWrite(STEP_PIN, LOW);    // Estado inicial do pulso
  
  delay(500);  // Pausa para estabilização
  Serial.println("Sistema reiniciado - pronto para subir");
}

// ==================== COMANDOS VIA SERIAL ====================

void verificarComandos() {
  if(Serial.available()) {
    String comando = Serial.readString();
    comando.trim();
    
    if(comando == "STOP") {
      // Para tudo imediatamente e trava motor
      pararMotor();
      Serial.println("PARADA DE EMERGÊNCIA - Motor travado!");
      while(true) {
        delay(1000);  // Loop infinito - precisa resetar para continuar
      }
    }
    else if(comando == "STATUS") {
      // Mostra estado atual do sistema
      Serial.print("Estado atual: ");
      Serial.println(obterNomeEstado());
      Serial.print("Motor habilitado: ");
      Serial.println(digitalRead(ENA_PIN) == LOW ? "SIM" : "NÃO");
      Serial.print("Direção: ");
      Serial.println(digitalRead(DIR_PIN) == SUBIR ? "SUBIR" : "DESCER");
      Serial.print("Tempo no estado atual: ");
      Serial.print(millis() - tempoInicio);
      Serial.println(" ms");
    }
    else if(comando == "VELOCIDADES") {
      // Mostra as velocidades configuradas
      Serial.println("=== VELOCIDADES CONFIGURADAS ===");
      Serial.print("Subida: ");
      Serial.print(VELOCIDADE_SUBIDA);
      Serial.println(" microssegundos/pulso");
      Serial.print("Descida: ");
      Serial.print(VELOCIDADE_DESCIDA);
      Serial.println(" microssegundos/pulso");
      Serial.println("(Menor valor = mais rápido)");
    }
  }
}

const char* obterNomeEstado() {
  // Converte o estado atual para texto legível
  switch(estadoAtual) {
    case SUBINDO: return "SUBINDO";
    case PARADO_NO_TOPO: return "PARADO NO TOPO";
    case DESCENDO: return "DESCENDO";           // Novo nome do estado
    case PARADO_EM_BAIXO: return "PARADO EM BAIXO"; // Novo estado
    case REINICIANDO: return "REINICIANDO";
    default: return "DESCONHECIDO";
  }
}