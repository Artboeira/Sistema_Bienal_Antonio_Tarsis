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
// const int LED_BUILTIN = 2;     // LED embutido do ESP32

// ==================== CONFIGURAÇÕES DO SISTEMA ====================
// Tempo de cada etapa em milissegundos
const unsigned long TEMPO_DE_SUBIDA = 18000;      // 18 segundos para içar
const unsigned long TEMPO_DE_RETENCAO = 150000;    // 150 segundos mantendo suspenso
const unsigned long TEMPO_DE_QUEDA = 40000;       // 40 segundos com objeto baixo
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
  // Initialization States (executados apenas no setup)
  INICIALIZANDO_LIBERACAO_INDO,
  INICIALIZANDO_LIBERACAO_ESPERANDO,
  INICIALIZANDO_LIBERACAO_VOLTANDO,

  // NOVOS ESTADOS: Liberação Preventiva no Início do Loop Principal
  PREVENTIVO_CICLO1_INDO,
  PREVENTIVO_CICLO1_ESPERANDO,
  PREVENTIVO_CICLO1_VOLTANDO,
  PREVENTIVO_PULSO_DC,
  PREVENTIVO_CICLO2_INDO,
  PREVENTIVO_CICLO2_ESPERANDO,
  PREVENTIVO_CICLO2_VOLTANDO,

  // Estados Principais do Loop Operacional
  ICANDO,              // Içando o objeto
  RETENDO_ALTO,        // Mantendo o objeto suspenso

  // Ciclo de Liberação Final (mantido para o final da sequência)
  LIBERANDO_CICLO1_INDO,
  LIBERANDO_CICLO1_ESPERANDO,
  LIBERANDO_CICLO1_VOLTANDO,
  LIBERANDO_PULSO_DC,
  LIBERANDO_CICLO2_INDO,
  LIBERANDO_CICLO2_ESPERANDO,
  LIBERANDO_CICLO2_VOLTANDO,

  RETENDO_BAIXO        // Mantendo o objeto baixo
};

// ==================== VARIÁVEIS GLOBAIS ====================
// Objeto AccelStepper para controle não bloqueante do motor de passo
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// Estado atual do sistema
EstadoSistema estadoAtual = INICIALIZANDO_LIBERACAO_INDO;

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
    // ==================== ESTADOS DE INICIALIZAÇÃO (SETUP) ====================
    case INICIALIZANDO_LIBERACAO_INDO:
      if (stepper.distanceToGo() == 0) {
        Serial.print("[INICIALIZANDO] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000);
        Serial.println("ms...");
        estadoAtual = INICIALIZANDO_LIBERACAO_ESPERANDO;
        tempoInicioEstado = millis();
      }
      break;
      
    case INICIALIZANDO_LIBERACAO_ESPERANDO:
      if (millis() - tempoInicioEstado >= 1000) {
        Serial.print("[INICIALIZANDO] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);
        estadoAtual = INICIALIZANDO_LIBERACAO_VOLTANDO;
        tempoInicioEstado = millis();
      }
      break;
      
    case INICIALIZANDO_LIBERACAO_VOLTANDO:
      if (stepper.distanceToGo() == 0) {
        Serial.println("[INICIALIZANDO] ✅ Liberação de segurança concluída.");
        stepper.setCurrentPosition(0);
        
        // MUDANÇA CRÍTICA: Agora vai para a liberação preventiva, não direto para ICANDO
        estadoAtual = PREVENTIVO_CICLO1_INDO;
        tempoInicioEstado = millis();
        
        Serial.println("\n🚀 SISTEMA PRONTO. INICIANDO LIBERAÇÃO PREVENTIVA!");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.print("[PREVENTIVO-C1] 🔓 Iniciando liberação preventiva (-");
        Serial.print(ANGULO_LIBERACAO);
        Serial.println("°)...");
        int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
        stepper.moveTo(passos);
      }
      break;

    // ==================== NOVOS ESTADOS: LIBERAÇÃO PREVENTIVA ====================
    case PREVENTIVO_CICLO1_INDO:
      if (stepper.distanceToGo() == 0) {
        Serial.print("[PREVENTIVO-C1] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000);
        Serial.println("ms...");
        estadoAtual = PREVENTIVO_CICLO1_ESPERANDO;
        tempoInicioEstado = millis();
      }
      break;
      
    case PREVENTIVO_CICLO1_ESPERANDO:
      if (millis() - tempoInicioEstado >= 1000) {
        Serial.print("[PREVENTIVO-C1] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);
        estadoAtual = PREVENTIVO_CICLO1_VOLTANDO;
        tempoInicioEstado = millis();
      }
      break;
      
    case PREVENTIVO_CICLO1_VOLTANDO:
      if (stepper.distanceToGo() == 0) {
        Serial.println("[PREVENTIVO-C1] ✅ Primeiro ciclo preventivo concluído.");
        stepper.setCurrentPosition(0);
        
        Serial.print("[PREVENTIVO-PULSO] ⚡ Acionando pulso preventivo de ");
        Serial.print(TEMPO_PULSO_DC);
        Serial.println("ms no motor DC...");
        digitalWrite(RELAY_CH1_PIN, LOW);
        digitalWrite(LED_BUILTIN, HIGH);
        
        estadoAtual = PREVENTIVO_PULSO_DC;
        tempoInicioEstado = millis();
      }
      break;

    case PREVENTIVO_PULSO_DC:
      if (millis() - tempoInicioEstado >= TEMPO_PULSO_DC) {
        digitalWrite(RELAY_CH1_PIN, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("[PREVENTIVO-PULSO] ✅ Pulso preventivo concluído.");
        
        Serial.print("[PREVENTIVO-C2] 🔓 Iniciando SEGUNDO ciclo preventivo (-");
        Serial.print(ANGULO_LIBERACAO);
        Serial.println("°)...");
        int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
        stepper.moveTo(passos);
        
        estadoAtual = PREVENTIVO_CICLO2_INDO;
        tempoInicioEstado = millis();
      }
      break;

    case PREVENTIVO_CICLO2_INDO:
      if (stepper.distanceToGo() == 0) {
        Serial.print("[PREVENTIVO-C2] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000);
        Serial.println("ms...");
        estadoAtual = PREVENTIVO_CICLO2_ESPERANDO;
        tempoInicioEstado = millis();
      }
      break;

    case PREVENTIVO_CICLO2_ESPERANDO:
      if (millis() - tempoInicioEstado >= 1000) {
        Serial.print("[PREVENTIVO-C2] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);
        estadoAtual = PREVENTIVO_CICLO2_VOLTANDO;
        tempoInicioEstado = millis();
      }
      break;

    case PREVENTIVO_CICLO2_VOLTANDO:
      if (stepper.distanceToGo() == 0) {
        Serial.println("[PREVENTIVO-C2] ✅ Segundo ciclo preventivo concluído.");
        stepper.setCurrentPosition(0);
        
        // AGORA SIM: Transição para o ciclo operacional normal
        Serial.println("[PREVENTIVO] ✅ LIBERAÇÃO PREVENTIVA COMPLETA!");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.println("[ICANDO] 🚀 Iniciando ciclo operacional - Acionando motor de içamento...");
        digitalWrite(RELAY_CH1_PIN, LOW);
        digitalWrite(LED_BUILTIN, HIGH);
        
        estadoAtual = ICANDO;
        tempoInicioEstado = millis();
      }
      break;

    // ==================== ESTADOS PRINCIPAIS DO LOOP OPERACIONAL ====================
    case ICANDO:
      if (millis() - tempoInicioEstado >= TEMPO_DE_SUBIDA) {
        digitalWrite(RELAY_CH1_PIN, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("[ICANDO] ✅ Motor de içamento desativado");
        
        estadoAtual = RETENDO_ALTO;
        tempoInicioEstado = millis();
        Serial.println("[RETENDO_ALTO] ⏸️  Mantendo objeto suspenso...");
      }
      break;
      
    case RETENDO_ALTO:
      if (millis() - tempoInicioEstado >= TEMPO_DE_RETENCAO) {
        Serial.println("[LIBERANDO] 🔓 Iniciando sequência de liberação final...");
        Serial.print("[LIBERANDO] Movendo para a posição de liberação (-");
        Serial.print(ANGULO_LIBERACAO);
        Serial.println("°)...");
        int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
        stepper.moveTo(passos);

        estadoAtual = LIBERANDO_CICLO1_INDO;
        tempoInicioEstado = millis();
      }
      break;

    // ==================== ESTADOS DE LIBERAÇÃO FINAL (MANTIDOS IGUAIS) ====================
    case LIBERANDO_CICLO1_INDO:
      if (stepper.distanceToGo() == 0) {
        Serial.print("[LIBERANDO-C1] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000);
        Serial.println("ms...");
        estadoAtual = LIBERANDO_CICLO1_ESPERANDO;
        tempoInicioEstado = millis();
      }
      break;
      
    case LIBERANDO_CICLO1_ESPERANDO:
      if (millis() - tempoInicioEstado >= 1000) {
        Serial.print("[LIBERANDO-C1] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);
        estadoAtual = LIBERANDO_CICLO1_VOLTANDO;
        tempoInicioEstado = millis();
      }
      break;
      
    case LIBERANDO_CICLO1_VOLTANDO:
      if (stepper.distanceToGo() == 0) {
        Serial.println("[LIBERANDO-C1] ✅ Primeiro ciclo de liberação concluído.");
        stepper.setCurrentPosition(0);

        Serial.print("[LIBERANDO-PULSO] ⚡ Acionando pulso de ");
        Serial.print(TEMPO_PULSO_DC);
        Serial.println("ms no motor DC...");
        digitalWrite(RELAY_CH1_PIN, LOW);
        digitalWrite(LED_BUILTIN, HIGH);

        estadoAtual = LIBERANDO_PULSO_DC;
        tempoInicioEstado = millis();
      }
      break;

    case LIBERANDO_PULSO_DC:
      if (millis() - tempoInicioEstado >= TEMPO_PULSO_DC) {
        digitalWrite(RELAY_CH1_PIN, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
        Serial.println("[LIBERANDO-PULSO] ✅ Pulso DC concluído.");

        Serial.print("[LIBERANDO-C2] 🔓 Iniciando SEGUNDO ciclo de liberação (-");
        Serial.print(ANGULO_LIBERACAO);
        Serial.println("°)...");
        int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
        stepper.moveTo(passos);

        estadoAtual = LIBERANDO_CICLO2_INDO;
        tempoInicioEstado = millis();
      }
      break;

    case LIBERANDO_CICLO2_INDO:
      if (stepper.distanceToGo() == 0) {
        Serial.print("[LIBERANDO-C2] Posição de liberação alcançada. Aguardando ");
        Serial.print(1000);
        Serial.println("ms...");
        estadoAtual = LIBERANDO_CICLO2_ESPERANDO;
        tempoInicioEstado = millis();
      }
      break;

    case LIBERANDO_CICLO2_ESPERANDO:
      if (millis() - tempoInicioEstado >= 1000) {
        Serial.print("[LIBERANDO-C2] Retornando à posição zero com overdrive (alvo: +");
        Serial.print(PASSOS_OVERDRIVE);
        Serial.println(" passos)...");
        stepper.moveTo(PASSOS_OVERDRIVE);
        estadoAtual = LIBERANDO_CICLO2_VOLTANDO;
        tempoInicioEstado = millis();
      }
      break;

    case LIBERANDO_CICLO2_VOLTANDO:
      if (stepper.distanceToGo() == 0) {
        Serial.println("[LIBERANDO-C2] ✅ Segundo ciclo de liberação concluído.");
        stepper.setCurrentPosition(0);

        Serial.println("[RETENDO_BAIXO] ⏸️  Mantendo objeto baixo...");
        estadoAtual = RETENDO_BAIXO;
        tempoInicioEstado = millis();
      }
      break;
      
    case RETENDO_BAIXO:
      if (millis() - tempoInicioEstado >= TEMPO_DE_QUEDA) {
        Serial.println("[CICLO COMPLETO] 🔄 Reiniciando ciclo...");
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // MUDANÇA CRÍTICA: Ao reiniciar, volta para a liberação preventiva
        Serial.print("[PREVENTIVO-C1] 🔓 Iniciando nova liberação preventiva (-");
        Serial.print(ANGULO_LIBERACAO);
        Serial.println("°)...");
        int passos = (-ANGULO_LIBERACAO * STEPS_PER_REV) / 360;
        stepper.moveTo(passos);
        
        estadoAtual = PREVENTIVO_CICLO1_INDO;
        tempoInicioEstado = millis();
      }
      break;
  }
}