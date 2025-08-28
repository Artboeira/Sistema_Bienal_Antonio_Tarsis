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
const unsigned long TEMPO_PULSO_DC = 500; // 500ms de pulso para garantir a liberação

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

  // Ciclo de Liberação #1
  LIBERANDO_CICLO1_INDO,
  LIBERANDO_CICLO1_ESPERANDO,
  LIBERANDO_CICLO1_VOLTANDO,

  // Pulso Intermediário do Motor DC
  LIBERANDO_PULSO_DC,

  // Ciclo de Liberação #2
  LIBERANDO_CICLO2_INDO,
  LIBERANDO_CICLO2_ESPERANDO,
  LIBERANDO_CICLO2_VOLTANDO,

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
        estadoAtual = LIBERANDO_CICLO1_INDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case LIBERANDO_CICLO1_INDO:
      // Condição: Espera o stepper chegar na posição de -ANGULO_LIBERACAO°
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for LIBERANDO_CICLO1_ESPERANDO (nada além de logging)
        Serial.print("[LIBERANDO-C1] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000); // Manter hardcoded conforme solicitado
        Serial.println("ms...");

        // State Transition
        estadoAtual = LIBERANDO_CICLO1_ESPERANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case LIBERANDO_CICLO1_ESPERANDO:
      // Condição: Espera 1000ms
      if (millis() - tempoInicioEstado >= 1000) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for LIBERANDO_CICLO1_VOLTANDO
        Serial.print("[LIBERANDO-C1] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);

        // State Transition
        estadoAtual = LIBERANDO_CICLO1_VOLTANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;
      
    case LIBERANDO_CICLO1_VOLTANDO:
      // Condição: Espera o stepper voltar para a posição de overdrive
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        Serial.println("[LIBERANDO-C1] ✅ Primeiro ciclo de liberação concluído.");
        stepper.setCurrentPosition(0); // Recalibra após o primeiro ciclo

        // Entry Action for the NEXT state (LIBERANDO_PULSO_DC)
        Serial.print("[LIBERANDO-PULSO] ⚡ Acionando pulso de ");
        Serial.print(TEMPO_PULSO_DC);
        Serial.println("ms no motor DC...");
        digitalWrite(RELAY_CH1_PIN, LOW); // Ativa o relé (motor DC)
        digitalWrite(LED_BUILTIN, HIGH);  // Liga o LED junto

        // State Transition
        estadoAtual = LIBERANDO_PULSO_DC;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;

    case LIBERANDO_PULSO_DC:
      // Condição: Espera o tempo do pulso terminar
      if (millis() - tempoInicioEstado >= TEMPO_PULSO_DC) {
        // --- START OF TRANSITION BLOCK ---
        // Desativa o motor DC
        digitalWrite(RELAY_CH1_PIN, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("[LIBERANDO-PULSO] ✅ Pulso DC concluído.");

        // Entry Action for the NEXT state (LIBERANDO_CICLO2_INDO)
        // (A lógica é idêntica ao início do primeiro ciclo)
        Serial.print("[LIBERANDO-C2] 🔓 Iniciando SEGUNDO ciclo de liberação (-");
        Serial.print(ANGULO_LIBERACAO);
        Serial.println("°)...");
        int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
        stepper.moveTo(passos);

        // State Transition
        estadoAtual = LIBERANDO_CICLO2_INDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;

    case LIBERANDO_CICLO2_INDO:
      // Condição: Espera o stepper chegar na posição de -ANGULO_LIBERACAO°
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for LIBERANDO_CICLO2_ESPERANDO (nada além de logging)
        Serial.print("[LIBERANDO-C2] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000); // Manter hardcoded conforme solicitado
        Serial.println("ms...");

        // State Transition
        estadoAtual = LIBERANDO_CICLO2_ESPERANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;

    case LIBERANDO_CICLO2_ESPERANDO:
      // Condição: Espera 1000ms
      if (millis() - tempoInicioEstado >= 1000) {
        // --- START OF TRANSITION BLOCK ---
        // Entry Action for LIBERANDO_CICLO2_VOLTANDO
        Serial.print("[LIBERANDO-C2] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);

        // State Transition
        estadoAtual = LIBERANDO_CICLO2_VOLTANDO;
        tempoInicioEstado = millis();
        // --- END OF TRANSITION BLOCK ---
      }
      break;

    case LIBERANDO_CICLO2_VOLTANDO:
      // Condição: Espera o stepper voltar para a posição de overdrive
      if (stepper.distanceToGo() == 0) {
        // --- START OF TRANSITION BLOCK ---
        Serial.println("[LIBERANDO-C2] ✅ Segundo ciclo de liberação concluído.");
        stepper.setCurrentPosition(0); // Recalibra após o segundo ciclo

        // Entry Action for the NEXT state (RETENDO_BAIXO)
        Serial.println("[RETENDO_BAIXO] ⏸️  Mantendo objeto baixo...");

        // State Transition
        estadoAtual = RETENDO_BAIXO;
        tempoInicioEstado = millis();
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
