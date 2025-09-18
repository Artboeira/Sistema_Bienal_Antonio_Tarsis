/*
 * SISTEMA DE PERCUSSÃO AUTOMÁTICA - Penduricalho de Carvão
 * ESP32 + NEMA 23 + Driver TB6600
 * 
 * Funcionamento:
 * 1. Penduricalho SOBE lentamente (15 segundos)
 * 2. Fica ERGUIDO na posição (1 minuto)
 * 3. CAI em queda livre simulada (~2 segundos) 
 * 4. BATE no tambor e fica parado (30 segundos)
 * 5. Repete o ciclo automaticamente
 */

// ==================== PINOS DO ESP32 ====================
const int STEP_PIN = 25;  // Pulsos para o TB6600
const int DIR_PIN = 26;   // Direção do motor
const int ENA_PIN = 27;   // Habilita/desabilita motor

// ==================== VELOCIDADES OTIMIZADAS PARA PERCUSSÃO ====================
const int VELOCIDADE_SUBIDA = 12000;   // LENTA: 12ms entre pulsos (subida suave)
const int VELOCIDADE_DESCIDA = 200;   // RÁPIDA: 0.2ms entre pulsos (queda livre simulada)

// ==================== DIREÇÕES ====================
const int SUBIR = HIGH;    // Ajuste conforme seu setup
const int DESCER = LOW;    // Direção oposta à subida

// ==================== TEMPOS OTIMIZADOS PARA PERFORMANCE MUSICAL ====================
const unsigned long TEMPO_SUBIDA = 27000;      // 20 segundos - subida lenta e suave
const unsigned long TEMPO_ERGUIDO = 90000;     // 60 segundos - tensão musical antes da queda
const unsigned long TEMPO_DESCIDA = 2000;      // 2 segundos - queda livre para impacto
const unsigned long TEMPO_NO_TAMBOR = 45000;   // 45 segundos - ressonância do tambor

// ==================== CONTROLE DE ESTADOS ====================
enum Estado {
  SUBINDO_LENTO,          // Subida controlada e suave
  ERGUIDO_TENSAO,         // Parado no alto criando tensão
  QUEDA_LIVRE,            // Descida rápida simulando queda
  BATENDO_TAMBOR,         // Parado embaixo após impacto
  PREPARANDO_NOVO_CICLO,  // Breve pausa antes do próximo ciclo
  PARADA_EMERGENCIA       // Sistema parado
};

Estado estadoAtual = SUBINDO_LENTO;
unsigned long tempoInicio = 0;
unsigned long ultimoPulso = 0;  // Para controle não bloqueante de pulsos
bool estadoPulso = LOW;         // Estado atual do pulso
bool sistemaAtivo = true;       // Flag para controle geral do sistema
unsigned long contadorCiclos = 0; // Conta quantos ciclos já foram executados

void setup() {
  // ========== INICIALIZAÇÃO SISTEMA PERCUSSÃO ==========
  Serial.begin(9600);
  delay(2000);  // Aguarda ESP32 estabilizar
  
  Serial.println("=========================================");
  Serial.println("   SISTEMA PERCUSSAO AUTOMATICA V2.0    ");
  Serial.println("     Penduricalho de Carvao + Tambor     ");
  Serial.println("=========================================");
  Serial.println();
  
  // Configura os pinos
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  
  // ========== INICIALIZAÇÃO SEGURA DO MOTOR ==========
  Serial.println("Inicializando motor...");
  
  // Primeiro desabilita motor durante configuração
  digitalWrite(ENA_PIN, HIGH);   // Motor desabilitado
  digitalWrite(STEP_PIN, LOW);   
  digitalWrite(DIR_PIN, SUBIR);  
  delay(500);  
  
  // Habilita motor
  digitalWrite(ENA_PIN, LOW);    // Motor ligado
  Serial.println("-> Motor habilitado e pronto");
  
  // ========== PARÂMETROS DE PERFORMANCE ==========
  Serial.println("\n===== CONFIGURACAO DE PERFORMANCE =====");
  Serial.print("Tempo subida (lenta): ");
  Serial.print(TEMPO_SUBIDA/1000);
  Serial.println(" segundos");
  
  Serial.print("Tempo erguido (tensao): ");
  Serial.print(TEMPO_ERGUIDO/1000);
  Serial.println(" segundos");
  
  Serial.print("Tempo descida (queda livre): ");
  Serial.print(TEMPO_DESCIDA/1000);
  Serial.println(" segundos");
  
  Serial.print("Tempo no tambor (ressonancia): ");
  Serial.print(TEMPO_NO_TAMBOR/1000);
  Serial.println(" segundos");
  
  Serial.println("\n===== VELOCIDADES =====");
  Serial.print("Subida: ");
  Serial.print(VELOCIDADE_SUBIDA);
  Serial.print(" us/pulso (~");
  Serial.print(1000000 / VELOCIDADE_SUBIDA);
  Serial.println(" Hz)");
  
  Serial.print("Descida: ");
  Serial.print(VELOCIDADE_DESCIDA);
  Serial.print(" us/pulso (~");
  Serial.print(1000000 / VELOCIDADE_DESCIDA);
  Serial.println(" Hz) - QUEDA LIVRE!");
  
  Serial.println("\n===== COMANDOS DISPONIVEIS =====");
  Serial.println("STATUS - Mostra estado atual");
  Serial.println("STOP - Parada de emergencia");
  Serial.println("RESET - Reinicia apos parada");
  Serial.println("CICLOS - Mostra contador de ciclos");
  
  Serial.println("\n*** INICIANDO PERFORMANCE EM 3 SEGUNDOS ***");
  delay(3000);
  
  // Inicia o primeiro ciclo
  tempoInicio = millis();
  ultimoPulso = millis();
  estadoAtual = SUBINDO_LENTO;
  contadorCiclos = 1;
  
  Serial.println("\n🎵 CICLO 1 INICIADO - SUBIDA LENTA 🎵");
}

void loop() {
  if (!sistemaAtivo) {
    return; // Sistema parado
  }
  
  unsigned long tempoAtual = millis();
  unsigned long tempoDecorrido = tempoAtual - tempoInicio;
  
  switch(estadoAtual) {
    
    case SUBINDO_LENTO:
      // Subida lenta e controlada criando expectativa
      gerarPulsoNaoBloqueante(VELOCIDADE_SUBIDA);
      
      // Progress feedback mais musical
      if (tempoDecorrido % 3000 < 50 && tempoDecorrido > 100) {
        Serial.print("🔺 Subindo... ");
        Serial.print(tempoDecorrido/1000);
        Serial.print("/");
        Serial.print(TEMPO_SUBIDA/1000);
        Serial.println("s (criando tensao...)");
      }
      
      if(tempoDecorrido >= TEMPO_SUBIDA) {
        pararMotor();
        estadoAtual = ERGUIDO_TENSAO;
        tempoInicio = tempoAtual;
        Serial.println("⏸️  ERGUIDO - Momento de tensao maxima!");
      }
      break;
      
    case ERGUIDO_TENSAO:
      // Parado no topo, criando tensão musical antes da queda
      
      // Feedback da tensão crescente
      if (tempoDecorrido % 10000 < 50 && tempoDecorrido > 100) {
        Serial.print("⚡ Tensao... ");
        Serial.print(tempoDecorrido/1000);
        Serial.print("/");
        Serial.print(TEMPO_ERGUIDO/1000);
        Serial.print("s ");
        
        // Indicador visual da tensão crescente
        if (tempoDecorrido > TEMPO_ERGUIDO * 0.8) {
          Serial.println("(quase na hora...)");
        } else if (tempoDecorrido > TEMPO_ERGUIDO * 0.5) {
          Serial.println("(tensao aumentando...)");
        } else {
          Serial.println("(preparando...)");
        }
      }
      
      if(tempoDecorrido >= TEMPO_ERGUIDO) {
        prepararQuedaLivre();
        estadoAtual = QUEDA_LIVRE;
        tempoInicio = tempoAtual;
        ultimoPulso = tempoAtual;
        Serial.println("💥 QUEDA LIVRE INICIADA! 💥");
      }
      break;
      
    case QUEDA_LIVRE:
      // Descida muito rápida simulando queda livre
      gerarPulsoNaoBloqueante(VELOCIDADE_DESCIDA);
      
      // Feedback visual da queda
      Serial.print("⬇️💨 CAINDO! ");
      Serial.print(tempoDecorrido);
      Serial.println("ms");
      
      if(tempoDecorrido >= TEMPO_DESCIDA) {
        impactoTambor();
        estadoAtual = BATENDO_TAMBOR;
        tempoInicio = tempoAtual;
        Serial.println("🥁 IMPACTO NO TAMBOR! 🥁");
        Serial.println("♪♫ Deixando ressonar... ♫♪");
      }
      break;
      
    case BATENDO_TAMBOR:
      // Parado no tambor, deixando ressoar
      
      if (tempoDecorrido % 5000 < 50 && tempoDecorrido > 100) {
        Serial.print("🎶 Ressonando... ");
        Serial.print(tempoDecorrido/1000);
        Serial.print("/");
        Serial.print(TEMPO_NO_TAMBOR/1000);
        Serial.println("s");
      }
      
      if(tempoDecorrido >= TEMPO_NO_TAMBOR) {
        estadoAtual = PREPARANDO_NOVO_CICLO;
        tempoInicio = tempoAtual;
        Serial.println("🔄 Preparando proximo ciclo...");
      }
      break;
      
    case PREPARANDO_NOVO_CICLO:
      // Breve pausa antes do próximo ciclo
      if(tempoDecorrido >= 1000) { // 1 segundo
        iniciarNovoCiclo();
        estadoAtual = SUBINDO_LENTO;
        tempoInicio = tempoAtual;
        ultimoPulso = tempoAtual;
        contadorCiclos++;
        Serial.print("\n🎵 CICLO ");
        Serial.print(contadorCiclos);
        Serial.println(" INICIADO - SUBIDA LENTA 🎵");
      }
      break;
      
    case PARADA_EMERGENCIA:
      // Sistema parado
      break;
  }
  
  verificarComandos();
  delayMicroseconds(50); // Reduzido para melhor responsividade na queda livre
}

// ==================== FUNÇÕES ESPECIALIZADAS PARA PERCUSSÃO ====================

void gerarPulsoNaoBloqueante(int intervalo_micros) {
  unsigned long tempoAtual = micros();
  
  if (tempoAtual - ultimoPulso >= (intervalo_micros / 2)) {
    estadoPulso = !estadoPulso;
    digitalWrite(STEP_PIN, estadoPulso);
    ultimoPulso = tempoAtual;
  }
}

void pararMotor() {
  digitalWrite(STEP_PIN, LOW);
  estadoPulso = LOW;
  Serial.println("-> Motor parado mas energizado (posicao mantida)");
}

void prepararQuedaLivre() {
  Serial.println("Preparando queda livre...");
  
  // Para movimento atual
  digitalWrite(STEP_PIN, LOW);
  estadoPulso = LOW;
  delay(100); // Breve pausa para estabilização
  
  // Configura direção para queda
  digitalWrite(DIR_PIN, DESCER);
  delay(50);
  
  Serial.println("-> CONFIGURADO PARA QUEDA LIVRE!");
  Serial.println("-> Velocidade maxima ativada!");
}

void impactoTambor() {
  // Para o motor imediatamente após o impacto
  digitalWrite(STEP_PIN, LOW);
  estadoPulso = LOW;
  Serial.println("-> IMPACTO! Motor parado no tambor");
}

void iniciarNovoCiclo() {
  Serial.println("Configurando novo ciclo...");
  
  // Garante estado seguro
  digitalWrite(STEP_PIN, LOW);
  estadoPulso = LOW;
  
  // Configura para subida
  digitalWrite(ENA_PIN, LOW);     
  delay(50);
  digitalWrite(DIR_PIN, SUBIR);   
  delay(50);
  
  Serial.println("-> Pronto para nova subida");
}

// ==================== COMANDOS DE CONTROLE ====================

void verificarComandos() {
  if(Serial.available()) {
    String comando = Serial.readString();
    comando.trim();
    comando.toUpperCase();
    
    Serial.print("🎛️  Comando: ");
    Serial.println(comando);
    
    if(comando == "STOP") {
      pararEmergencia();
    }
    else if(comando == "RESET") {
      reiniciarSistema();
    }
    else if(comando == "STATUS") {
      mostrarStatus();
    }
    else if(comando == "CICLOS") {
      mostrarCiclos();
    }
    else {
      Serial.println("❌ Comando nao reconhecido!");
      Serial.println("✅ Comandos: STOP, RESET, STATUS, CICLOS");
    }
  }
}

void pararEmergencia() {
  Serial.println("\n🚨🚨🚨 PARADA DE EMERGENCIA 🚨🚨🚨");
  Serial.println("Performance interrompida!");
  
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(ENA_PIN, HIGH);  // Desabilita motor
  
  estadoAtual = PARADA_EMERGENCIA;
  sistemaAtivo = false;
  
  Serial.println("💤 Sistema desabilitado");
  Serial.println("Digite RESET para reativar");
}

void reiniciarSistema() {
  Serial.println("🔄 Reiniciando sistema de percussao...");
  
  digitalWrite(ENA_PIN, LOW);
  digitalWrite(DIR_PIN, SUBIR);
  digitalWrite(STEP_PIN, LOW);
  
  sistemaAtivo = true;
  estadoAtual = SUBINDO_LENTO;
  tempoInicio = millis();
  ultimoPulso = millis();
  estadoPulso = LOW;
  
  Serial.println("🎵 Sistema reativado! Performance continua...");
}

void mostrarStatus() {
  Serial.println("\n🎼 ===== STATUS DA PERFORMANCE =====");
  Serial.print("Estado atual: ");
  Serial.println(obterNomeEstado());
  Serial.print("Ciclo atual: ");
  Serial.println(contadorCiclos);
  Serial.print("Sistema ativo: ");
  Serial.println(sistemaAtivo ? "🟢 SIM" : "🔴 NAO");
  Serial.print("Motor: ");
  Serial.println(digitalRead(ENA_PIN) == LOW ? "🟢 HABILITADO" : "🔴 DESABILITADO");
  Serial.print("Direcao: ");
  Serial.println(digitalRead(DIR_PIN) == SUBIR ? "⬆️ SUBIR" : "⬇️ DESCER");
  Serial.print("Tempo no estado: ");
  Serial.print((millis() - tempoInicio)/1000);
  Serial.println(" segundos");
  Serial.println("=====================================\n");
}

void mostrarCiclos() {
  Serial.println("\n🔄 ===== CONTADOR DE CICLOS =====");
  Serial.print("Ciclos completados: ");
  Serial.println(contadorCiclos - 1); // -1 porque está no ciclo atual
  Serial.print("Ciclo atual: ");
  Serial.println(contadorCiclos);
  
  // Estimativa de tempo total
  unsigned long tempoCiclo = TEMPO_SUBIDA + TEMPO_ERGUIDO + TEMPO_DESCIDA + TEMPO_NO_TAMBOR + 1000;
  unsigned long tempoTotal = (contadorCiclos - 1) * tempoCiclo + (millis() - tempoInicio);
  
  Serial.print("Tempo total de performance: ");
  Serial.print(tempoTotal / 60000);
  Serial.print(" minutos e ");
  Serial.print((tempoTotal % 60000) / 1000);
  Serial.println(" segundos");
  Serial.println("===============================\n");
}

const char* obterNomeEstado() {
  switch(estadoAtual) {
    case SUBINDO_LENTO: return "🔺 SUBINDO LENTO";
    case ERGUIDO_TENSAO: return "⚡ ERGUIDO (TENSAO)";
    case QUEDA_LIVRE: return "💥 QUEDA LIVRE";
    case BATENDO_TAMBOR: return "🥁 BATENDO TAMBOR";
    case PREPARANDO_NOVO_CICLO: return "🔄 PREPARANDO CICLO";
    case PARADA_EMERGENCIA: return "🚨 PARADA EMERGENCIA";
    default: return "❓ DESCONHECIDO";
  }
}