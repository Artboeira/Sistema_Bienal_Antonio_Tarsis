/*
 * Controle de Motor de Passo - Sequência Pré-definida
 * 
 * Sequência:
 * 1. Posição inicial (0°)
 * 2. Aguarda 3 segundos
 * 3. Gira 50° anti-horário (-50°)
 * 4. Aguarda 2 segundos
 * 5. Gira 50° horário (volta para 0°)
 * 6. Repete o ciclo
 */

// ==================== CONFIGURAÇÃO DE MICROSTEPPING ====================
// Altere conforme sua configuração nas DIP switches do TB6600

// Para FULL STEP (máximo torque):
const int STEPS_PER_REV = 200;  // 200 passos por volta (Full Step)

// Para 1/8 STEP (mais suave):
//const int STEPS_PER_REV = 1600;  // 1600 passos por volta (1/8 Step)

// ==================== DEFINIÇÃO DOS PINOS ====================
const int STEP_PIN = 25;  // Pino de pulso (PUL+)
const int DIR_PIN = 26;   // Pino de direção (DIR+)
const int ENA_PIN = 27;   // Pino de habilitação (ENA+)

// ==================== CONFIGURAÇÕES DO MOVIMENTO ====================
const int ANGULO_MOVIMENTO = 60;        // Graus para girar em cada direção
const int DELAY_INICIAL = 3000;         // 3 segundos antes de começar
const int DELAY_ENTRE_MOVIMENTOS = 2000; // 2 segundos entre movimentos
const int VELOCIDADE_MOVIMENTO = 1500;   // Microsegundos entre passos (ajuste conforme necessário)

// ==================== VARIÁVEIS GLOBAIS ====================
int posicaoAtual = 0;        // Posição atual em graus
int cicloNumero = 0;         // Contador de ciclos completos
unsigned long tempoInicio = 0; // Tempo de início do ciclo
bool primeiraVez = true;     // Flag para primeira execução

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);  // Pequeno delay para estabilizar serial
  
  // Limpa o monitor serial e mostra cabeçalho
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════════════════════════╗");
  Serial.println("║         CONTROLE DE SEQUÊNCIA PRÉ-DEFINIDA              ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  SEQUÊNCIA PROGRAMADA:                                  ║");
  Serial.println("║  1. Posição inicial (0°)                                ║");
  Serial.println("║  2. Aguarda 3 segundos                                  ║");
  Serial.println("║  3. Gira 50° anti-horário                              ║");
  Serial.println("║  4. Aguarda 2 segundos                                  ║");
  Serial.println("║  5. Gira 50° horário (volta ao início)                 ║");
  Serial.println("║  6. Repete continuamente                                ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  // Mostra configuração atual
  Serial.println("\n📋 CONFIGURAÇÃO DO SISTEMA:");
  Serial.print("   • Passos por revolução: ");
  Serial.println(STEPS_PER_REV);
  Serial.print("   • Graus por passo: ");
  Serial.println(360.0 / STEPS_PER_REV);
  Serial.print("   • Ângulo de movimento: ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("°");
  Serial.print("   • Velocidade: ");
  Serial.print(1000000 / (VELOCIDADE_MOVIMENTO * 2));
  Serial.println(" passos/segundo");
  
  // Configuração dos pinos
  Serial.println("\n⚙️  Configurando pinos...");
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  
  // Estado inicial dos pinos
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  digitalWrite(ENA_PIN, LOW);  // LOW = motor habilitado
  
  Serial.println("✅ Pinos configurados");
  
  // Posição inicial
  Serial.println("\n🏁 ESTABELECENDO POSIÇÃO INICIAL (0°)");
  Serial.println("   Motor na posição de referência");
  posicaoAtual = 0;
  
  // Pequeno movimento para "acordar" o motor
  Serial.println("   Energizando motor...");
  for(int i = 0; i < 2; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(1000);
  }
  
  Serial.println("✅ Sistema pronto para iniciar sequência!");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // Incrementa contador de ciclos
  cicloNumero++;
  tempoInicio = millis();
  
  // Cabeçalho do ciclo
  Serial.println("╔══════════════════════════════════════════════════════════╗");
  Serial.print("║                    CICLO NÚMERO ");
  Serial.print(cicloNumero);
  if(cicloNumero < 10) Serial.print(" ");
  if(cicloNumero < 100) Serial.print(" ");
  Serial.println("                      ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  // ========== PASSO 1: POSIÇÃO INICIAL ==========
  Serial.println("\n[PASSO 1] 📍 POSIÇÃO INICIAL");
  Serial.print("   Posição atual: ");
  Serial.print(posicaoAtual);
  Serial.println("°");
  Serial.println("   Status: Aguardando 3 segundos para iniciar movimento...");
  
  // Contagem regressiva de 3 segundos
  for(int i = 3; i > 0; i--) {
    Serial.print("   ⏰ Iniciando em ");
    Serial.print(i);
    Serial.println(" segundo(s)...");
    delay(1000);
  }
  
  // ========== PASSO 2: MOVIMENTO ANTI-HORÁRIO ==========
  Serial.println("\n[PASSO 2] ↺ MOVIMENTO ANTI-HORÁRIO");
  Serial.print("   Iniciando giro de ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("° no sentido anti-horário");
  Serial.print("   Posição inicial: ");
  Serial.print(posicaoAtual);
  Serial.println("°");
  
  // Executa movimento anti-horário
  moverMotor(ANGULO_MOVIMENTO, false);  // false = anti-horário
  
  // Atualiza posição
  posicaoAtual -= ANGULO_MOVIMENTO;
  
  Serial.print("   ✅ Movimento concluído!");
  Serial.print(" Nova posição: ");
  Serial.print(posicaoAtual);
  Serial.println("°");
  
  // ========== PASSO 3: AGUARDA 2 SEGUNDOS ==========
  Serial.println("\n[PASSO 3] ⏸️  PAUSA");
  Serial.println("   Motor parado na posição -50°");
  Serial.println("   Aguardando 2 segundos...");
  
  for(int i = 2; i > 0; i--) {
    Serial.print("   ⏰ ");
    Serial.print(i);
    Serial.println(" segundo(s)...");
    delay(1000);
  }
  
  // ========== PASSO 4: MOVIMENTO HORÁRIO ==========
  Serial.println("\n[PASSO 4] ↻ MOVIMENTO HORÁRIO");
  Serial.print("   Iniciando giro de ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("° no sentido horário");
  Serial.print("   Posição inicial: ");
  Serial.print(posicaoAtual);
  Serial.println("°");
  
  // Executa movimento horário
  moverMotor(ANGULO_MOVIMENTO, true);  // true = horário
  
  // Atualiza posição
  posicaoAtual += ANGULO_MOVIMENTO;
  
  Serial.print("   ✅ Movimento concluído!");
  Serial.print(" Nova posição: ");
  Serial.print(posicaoAtual);
  Serial.println("°");
  
  // ========== RESUMO DO CICLO ==========
  unsigned long tempoCiclo = millis() - tempoInicio;
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("📊 RESUMO DO CICLO:");
  Serial.print("   • Ciclo número: ");
  Serial.println(cicloNumero);
  Serial.print("   • Tempo total do ciclo: ");
  Serial.print(tempoCiclo / 1000.0);
  Serial.println(" segundos");
  Serial.print("   • Posição final: ");
  Serial.print(posicaoAtual);
  Serial.println("°");
  Serial.println("   • Status: ✅ Ciclo completo - Iniciando próximo ciclo");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("\n");
  
  // Pequena pausa antes de reiniciar (opcional)
  delay(500);
}

// ==================== FUNÇÃO DE MOVIMENTO ====================
/**
 * Move o motor um número específico de graus
 * @param graus: Número de graus para mover
 * @param horario: true = sentido horário, false = sentido anti-horário
 */
void moverMotor(int graus, bool horario) {
  // Define a direção
  digitalWrite(DIR_PIN, horario ? HIGH : LOW);
  
  // Calcula número de passos necessários
  int passos = (graus * STEPS_PER_REV) / 360;
  
  Serial.print("   Executando ");
  Serial.print(passos);
  Serial.print(" passos");
  Serial.print(" (");
  Serial.print(graus);
  Serial.println("°)");
  
  // Mostra barra de progresso
  Serial.print("   Progresso: [");
  int barraTotal = 20;  // Tamanho da barra de progresso
  
  // Executa os passos
  for(int i = 0; i < passos; i++) {
    // Envia pulso para o driver
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(VELOCIDADE_MOVIMENTO);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(VELOCIDADE_MOVIMENTO);
    
    // Atualiza barra de progresso
    int progresso = (i * barraTotal) / passos;
    int progressoAnterior = ((i - 1) * barraTotal) / passos;
    
    if(progresso > progressoAnterior) {
      Serial.print("■");
    }
    
    // Mostra porcentagem em pontos específicos
    if(i == passos / 4) {
      Serial.print("] 25%");
      Serial.print("\n              [");
      for(int j = 0; j < 5; j++) Serial.print("■");
    } else if(i == passos / 2) {
      Serial.print("] 50%");
      Serial.print("\n              [");
      for(int j = 0; j < 10; j++) Serial.print("■");
    } else if(i == (passos * 3) / 4) {
      Serial.print("] 75%");
      Serial.print("\n              [");
      for(int j = 0; j < 15; j++) Serial.print("■");
    }
  }
  
  // Completa a barra
  while(Serial.print("■") && barraTotal-- > 0);
  Serial.println("] 100%");
}

// ==================== FUNÇÕES AUXILIARES ====================
/**
 * Função de emergência - para o motor imediatamente
 * (Pode ser chamada por uma interrupção se necessário)
 */
void pararEmergencia() {
  digitalWrite(ENA_PIN, HIGH);  // Desabilita motor
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  
  Serial.println("\n\n🛑 PARADA DE EMERGÊNCIA!");
  Serial.println("   Motor desabilitado");
  Serial.println("   Sistema parado");
  
  while(1) {
    // Fica travado aqui até reset
    delay(1000);
  }
}

/**
 * Função para resetar posição (calibração)
 * Use se o motor perder passos ou precisar recalibrar
 */
void resetarPosicao() {
  Serial.println("\n🔄 RESET DE POSIÇÃO");
  Serial.println("   Definindo posição atual como 0°");
  posicaoAtual = 0;
  Serial.println("   ✅ Posição resetada");
}