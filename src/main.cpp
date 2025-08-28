/*
 * Controle de Motor de Passo - Sequência Pré-definida para Instalação Artística
 * 
 * Sequência:
 * 1. ICANDO - Aciona o motor de içamento por 6 segundos
 * 2. RETENDO_ALTO - Mantém o objeto suspenso por 1 segundo
 * 3. LIBERANDO - Libera o objeto com movimento do motor de passo
 * 4. RETENDO_BAIXO - Mantém o objeto baixo por 4 segundos
 * 5. Repete o ciclo
 */

#include <Arduino.h>
#include <AccelStepper.h>

// ==================== DEFINIÇÃO DOS PINOS ====================
// Stepper Motor Pins
const int STEP_PIN = 25;  // Pino de pulso (PUL+)
const int DIR_PIN = 26;   // Pino de direção (DIR+)
const int ENA_PIN = 27;   // Pino de habilitação (ENA+)

// Relay Module Pins (ACTIVE LOW)
const int RELAY_CH1_PIN = 21;  // Controle do motor de içamento
const int RELAY_CH2_PIN = 22;  // Reserva para expansão

// Built-in LED pin
const int LED_BUILTIN = 2;     // LED embutido do ESP32

// ==================== CONFIGURAÇÕES DO SISTEMA ====================
// Tempo de cada etapa em milissegundos
const unsigned long TEMPO_DE_SUBIDA = 35000;      // 30 segundos para içar
const unsigned long TEMPO_DE_RETENCAO = 60000;    // 8 segundo mantendo suspenso
const unsigned long TEMPO_DE_QUEDA = 18000;       // 8 segundos com objeto baixo

// ==================== CONFIGURAÇÃO DO MOTOR DE PASSO ====================
// Configuração dos passos por revolução (sem microstepping):
const int STEPS_PER_REV = 200;  // 200 para 1/1 (full step), 400 para 1/2, 800 para 1/4, etc.

// Ângulo de movimento do motor de passo durante a liberação
const int ANGULO_LIBERACAO = 90;  // Graus para girar 

// Ângulo de movimento do motor de passo durante o HOMING
const int ANGULO_HOMING = 120;  // Graus para girar

// Passos extras para garantir o reset contra o batente físico
const int PASSOS_OVERDRIVE = 20;

// ==================== DEFINIÇÃO DA MÁQUINA DE ESTADOS ====================
enum EstadoSistema {
  // New Initialization States
  INICIALIZANDO_LIBERACAO_INDO,
  INICIALIZANDO_LIBERACAO_ESPERANDO,
  INICIALIZANDO_LIBERACAO_VOLTANDO,

  // Existing Main Loop States
  ICANDO,              // Içando o objeto
  RETENDO_ALTO,        // Mantendo o objeto suspenso
  LIBERANDO_INDO,      // Stepper moving to release position
  LIBERANDO_ESPERANDO, // Waiting for 1000ms
  LIBERANDO_VOLTANDO,  // Stepper returning to overdrive position
  RETENDO_BAIXO        // Mantendo o objeto baixo
};

// ==================== VARIÁVEIS GLOBAIS ====================
// Objeto AccelStepper para controle não bloqueante do motor de passo
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Estado atual do sistema
EstadoSistema estadoAtual = ICANDO;

// Timestamp para controle de tempo não bloqueante
unsigned long tempoInicioEstado = 0;

// ==================== SETUP ====================
void setup() {
  // Inicializa a comunicação serial
  Serial.begin(115200);
  delay(500);  // Pequeno delay para estabilizar serial
  
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════════════════════════╗");
  Serial.println("║         CONTROLE DE SEQUÊNCIA ARTÍSTICA                 ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  INICIANDO CONFIGURAÇÃO DO SISTEMA                      ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  // Configuração dos pinos
  Serial.println("\n⚙️  Configurando pinos...");
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  pinMode(RELAY_CH1_PIN, OUTPUT);
  pinMode(RELAY_CH2_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Estado inicial dos pinos
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  digitalWrite(ENA_PIN, LOW);  // LOW = motor habilitado
  digitalWrite(RELAY_CH1_PIN, HIGH);  // HIGH = relé desativado (ativo baixo)
  digitalWrite(RELAY_CH2_PIN, HIGH);  // HIGH = relé desativado (ativo baixo)
  digitalWrite(LED_BUILTIN, LOW);     // LOW = LED desativado
  
  Serial.println("✅ Pinos configurados");
  
  // Configuração do motor de passo com AccelStepper
  Serial.println("\n⚙️  Configurando motor de passo...");
  stepper.setMaxSpeed(30.0);      // Velocidade máxima em passos/segundo
  stepper.setAcceleration(15.0);    // Aceleração em passos/segundo/segundo
  stepper.setCurrentPosition(0);     // Define posição inicial como zero
  
  // Corrige a direção do motor de passo
  stepper.setPinsInverted(true, false, false);
  
  Serial.println("✅ Motor de passo configurado");
  
  // Sequência de homing (calibração de posição)
  Serial.println("\n🏁 INICIANDO SEQUÊNCIA DE HOMING");
  Serial.print("   Movendo motor ");
  Serial.print(ANGULO_HOMING);
  Serial.println("° no sentido anti-horário para calibrar...");
  
  // Move o motor ANGULO_HOMING graus (horário) para posição de referência
  int passosHoming = (ANGULO_HOMING * STEPS_PER_REV) / 360;
  stepper.moveTo(passosHoming);
  
  // Espera bloqueante para conclusão do homing (aceitável pois é apenas no setup)
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  
  // Define a posição atual como zero após homing
  stepper.setCurrentPosition(0);
  Serial.println("✅ Posição calibrada - Zero absoluto estabelecido");
  
  // Inicia a sequência de liberação de segurança
  Serial.println("\n🛡️ INICIANDO SEQUÊNCIA DE LIBERAÇÃO DE SEGURANÇA");
  estadoAtual = INICIALIZANDO_LIBERACAO_INDO;
  tempoInicioEstado = millis();

  // Entry Action for the new initial state
  Serial.print("[INICIALIZANDO] Movendo para a posição de liberação (-");
  Serial.print(ANGULO_LIBERACAO);
  Serial.println("°)...");
  int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
  stepper.moveTo(passos);
  
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // Sempre chamar run() para processar movimentos do stepper de forma não bloqueante
  stepper.run();
  
  // Máquina de estados principal
  switch (estadoAtual) {
    case INICIALIZANDO_LIBERACAO_INDO:
      // Condição: Espera o stepper chegar na posição de -ANGULO_LIBERACAO°
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for INICIALIZANDO_LIBERACAO_ESPERANDO (nada além de logging)
        Serial.print("[INICIALIZANDO] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000); // Manter hardcoded conforme solicitado
        Serial.println("ms...");

        // State Transition
        estadoAtual = INICIALIZANDO_LIBERACAO_ESPERANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case INICIALIZANDO_LIBERACAO_ESPERANDO:
      // Condição: Espera 1000ms
      if (millis() - tempoInicioEstado >= 1000) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for INICIALIZANDO_LIBERACAO_VOLTANDO
        Serial.print("[INICIALIZANDO] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);

        // State Transition
        estadoAtual = INICIALIZANDO_LIBERACAO_VOLTANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case INICIALIZANDO_LIBERACAO_VOLTANDO:
      // Condição: Espera o stepper voltar para posição PASSOS_OVERDRIVE
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        Serial.println("[INICIALIZANDO] ✅ Liberação de segurança concluída.");
        
        // CRITICAL: Recalibrate the logical position to the physical reality (0).
        stepper.setCurrentPosition(0);

        // State Transition TO THE MAIN LOOP'S FIRST STATE
        estadoAtual = ICANDO;
        tempoInicioEstado = millis();
        
        // Entry Action for the main loop's first state (ICANDO)
        Serial.println("\n🚀 SISTEMA PRONTO. INICIANDO CICLO OPERACIONAL!");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("[ICANDO] 🚀 Acionando motor de içamento...");
        digitalWrite(RELAY_CH1_PIN, LOW);
        digitalWrite(LED_BUILTIN, HIGH);
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case ICANDO:
      // Condição: Tempo de subida concluído
      if (millis() - tempoInicioEstado >= TEMPO_DE_SUBIDA) {
        // --- START OF TRANSITION BLOCK ---
        // Desativa o motor de içamento
        digitalWrite(RELAY_CH1_PIN, HIGH);  // HIGH = relé desativado
        digitalWrite(LED_BUILTIN, LOW);     // LOW = LED desativado
        Serial.println("[ICANDO] ✅ Motor de içamento desativado");
        
        // Transição para próximo estado
        estadoAtual = RETENDO_ALTO;
        tempoInicioEstado = millis();
        Serial.println("[RETENDO_ALTO] ⏸️  Mantendo objeto suspenso...");
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case RETENDO_ALTO:
      // Condição: Tempo de retenção concluído
      if (millis() - tempoInicioEstado >= TEMPO_DE_RETENCAO) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for LIBERANDO_INDO
        Serial.println("[LIBERANDO] 🔓 Iniciando sequência de liberação...");
        Serial.print("[LIBERANDO] Movendo para a posição de liberação (-");
        Serial.print(ANGULO_LIBERACAO);
        Serial.println("°)...");
        int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
        stepper.moveTo(passos);

        // State Transition
        estadoAtual = LIBERANDO_INDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case LIBERANDO_INDO:
      // Condição: Espera o stepper chegar na posição de -ANGULO_LIBERACAO°
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for LIBERANDO_ESPERANDO (nada além de logging)
        Serial.print("[LIBERANDO] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000); // Manter hardcoded conforme solicitado
        Serial.println("ms...");

        // State Transition
        estadoAtual = LIBERANDO_ESPERANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case LIBERANDO_ESPERANDO:
      // Condição: Espera 1000ms
      if (millis() - tempoInicioEstado >= 1000) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for LIBERANDO_VOLTANDO
        Serial.print("[LIBERANDO] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);

        // State Transition
        estadoAtual = LIBERANDO_VOLTANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case LIBERANDO_VOLTANDO:
      // Condição: Espera o stepper voltar para posição PASSOS_OVERDRIVE
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for RETENDO_BAIXO (nada além de logging)
        Serial.println("[LIBERANDO] ✅ Sequência de liberação concluída, batente alcançado.");
        
        // CRITICAL: Recalibrate the logical position to the physical reality (0).
        stepper.setCurrentPosition(0);

        // State Transition
        estadoAtual = RETENDO_BAIXO;
        tempoInicioEstado = millis();
        Serial.println("[RETENDO_BAIXO] ⏸️  Mantendo objeto baixo...");
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case RETENDO_BAIXO:
      // Condição: Tempo de retenção concluído
      if (millis() - tempoInicioEstado >= TEMPO_DE_QUEDA) {
        // --- START OF TRANSITION BLOCK ---
        Serial.println("[CICLO COMPLETO] 🔄 Reiniciando ciclo...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // Entry Action for the NEXT state (ICANDO)
        Serial.println("[ICANDO] 🚀 Acionando motor de içamento...");
        digitalWrite(RELAY_CH1_PIN, LOW);   // LOW = relé ativado (ativo baixo)
        digitalWrite(LED_BUILTIN, HIGH);    // HIGH = LED ativado

        // State Transition
        estadoAtual = ICANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
  }
}
