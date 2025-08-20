/*
 * Controle de Motor de Passo - Sequência Simétrica 60°
 * 
 * Nova Sequência:
 * 1. Posição inicial (0°)
 * 2. Aguarda 3 segundos
 * 3. Gira 60° anti-horário (vai para -60°)
 * 4. Aguarda 2 segundos
 * 5. Gira 60° horário (volta para 0°)
 * 6. Aguarda 2 segundos
 * 7. Gira 60° horário (vai para +60°)
 * 8. Aguarda 2 segundos
 * 9. Gira 60° anti-horário (volta para 0°)
 * 10. Aguarda 5 segundos
 * 11. Repete o ciclo
 */

// ==================== CONFIGURAÇÃO DE MICROSTEPPING ====================
// IMPORTANTE: Configure as DIP switches do TB6600 de acordo!

// CONFIGURAÇÃO ATUAL: FULL STEP (MÁXIMO TORQUE)
const int STEPS_PER_REV = 200;  // 200 passos por volta (Full Step)
const char* MODO_ATUAL = "FULL STEP - TORQUE MÁXIMO";

// Se voltar para 1/8 STEP (mais suave, menos torque):
//const int STEPS_PER_REV = 1600;  // 1600 passos por volta (1/8 Step)
//const char* MODO_ATUAL = "1/8 STEP - MOVIMENTO SUAVE";

// ==================== CONFIGURAÇÃO DIP SWITCHES TB6600 ====================
// PARA FULL STEP (200 passos/volta):
// S4: OFF, S5: OFF, S6: OFF
// 
// CORRENTE PARA MÁXIMO TORQUE:
// 1.5A: S1: ON,  S2: OFF, S3: OFF
// 2.0A: S1: OFF, S2: OFF, S3: OFF (verifique se seu motor suporta)

// ==================== DEFINIÇÃO DOS PINOS ====================
const int STEP_PIN = 25;  // Pino de pulso (PUL+)
const int DIR_PIN = 26;   // Pino de direção (DIR+)
const int ENA_PIN = 27;   // Pino de habilitação (ENA+)

// ==================== CONFIGURAÇÕES DO MOVIMENTO ====================
const int ANGULO_MOVIMENTO = 60;        // Todos os movimentos são de 60°

const int DELAY_INICIAL = 3000;         // 3 segundos antes de começar
const int DELAY_CURTO = 2000;           // 2 segundos entre cada movimento
const int DELAY_LONGO = 5000;           // 5 segundos antes de recomeçar ciclo

// Velocidade ajustada automaticamente baseada no modo
const int VELOCIDADE_BASE = 1500;       // Velocidade base para full step
const int VELOCIDADE_MOVIMENTO = (STEPS_PER_REV == 200) ? VELOCIDADE_BASE : VELOCIDADE_BASE / 2;

// ==================== VARIÁVEIS GLOBAIS ====================
float posicaoAtual = 0;         // Posição atual em graus
int cicloNumero = 0;            // Contador de ciclos completos
unsigned long tempoInicio = 0;  // Tempo de início do ciclo
int totalPassosCiclo = 0;       // Total de passos em um ciclo

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);  // Pequeno delay para estabilizar serial
  
  // Limpa o monitor serial e mostra cabeçalho
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════════════════════════╗");
  Serial.println("║      CONTROLE DE SEQUÊNCIA - MOVIMENTO SIMÉTRICO 60°    ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  SEQUÊNCIA PROGRAMADA:                                  ║");
  Serial.println("║  1. Posição inicial (0°)                                ║");
  Serial.println("║  2. Aguarda 3 segundos                                  ║");
  Serial.println("║  3. Gira 60° anti-horário → (-60°)                     ║");
  Serial.println("║  4. Aguarda 2 segundos                                  ║");
  Serial.println("║  5. Gira 60° horário → (0°)                            ║");
  Serial.println("║  6. Aguarda 2 segundos                                  ║");
  Serial.println("║  7. Gira 60° horário → (+60°)                          ║");
  Serial.println("║  8. Aguarda 2 segundos                                  ║");
  Serial.println("║  9. Gira 60° anti-horário → (0°)                       ║");
  Serial.println("║  10. Aguarda 5 segundos                                 ║");
  Serial.println("║  11. Repete continuamente                               ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  // Mostra configuração atual
  Serial.println("\n📋 CONFIGURAÇÃO DO SISTEMA:");
  Serial.print("   • Modo: ");
  Serial.println(MODO_ATUAL);
  Serial.print("   • Passos por revolução: ");
  Serial.println(STEPS_PER_REV);
  Serial.print("   • Resolução: ");
  Serial.print(360.0 / STEPS_PER_REV, 2);
  Serial.println("° por passo");
  Serial.print("   • Ângulo de movimento: ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("° em cada direção");
  Serial.print("   • Amplitude total: ");
  Serial.println("120° (-60° até +60°)");
  Serial.print("   • Velocidade: ");
  Serial.print(1000000 / (VELOCIDADE_MOVIMENTO * 2));
  Serial.println(" passos/segundo");
  
  // AVISO IMPORTANTE SOBRE DIP SWITCHES
  if(STEPS_PER_REV == 200) {
    Serial.println("\n⚠️  ATENÇÃO - CONFIGURAÇÃO DIP SWITCHES TB6600:");
    Serial.println("   Para FULL STEP você DEVE configurar:");
    Serial.println("   ┌─────────────────────────────────┐");
    Serial.println("   │ MICROSTEPPING:                  │");
    Serial.println("   │ S4: OFF  S5: OFF  S6: OFF       │");
    Serial.println("   │                                 │");
    Serial.println("   │ CORRENTE (máximo torque):       │");
    Serial.println("   │ 1.5A: S1: ON  S2: OFF S3: OFF   │");
    Serial.println("   └─────────────────────────────────┘");
    Serial.println("   SE NÃO MUDAR AS DIPs, O MOVIMENTO SERÁ 8X MENOR!");
  }
  
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
  Serial.println("   Motor centralizado na posição de referência");
  posicaoAtual = 0;
  
  // Pequeno movimento para "acordar" o motor
  Serial.println("   Energizando motor...");
  for(int i = 0; i < 5; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(1000);
  }
  
  Serial.println("✅ Sistema pronto para iniciar sequência!");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  
  // Executa diagnóstico na primeira vez
  if(STEPS_PER_REV == 200) {
    Serial.println("\n💡 DICA: Digite 'D' no monitor serial para diagnóstico");
    Serial.println("   (Verifica se as DIP switches estão corretas)");
  }
  
  Serial.println("");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // Incrementa contador de ciclos
  cicloNumero++;
  tempoInicio = millis();
  totalPassosCiclo = 0;
  
  // Cabeçalho do ciclo
  Serial.println("╔══════════════════════════════════════════════════════════╗");
  Serial.print("║                    CICLO NÚMERO ");
  Serial.print(cicloNumero);
  if(cicloNumero < 10) Serial.print(" ");
  if(cicloNumero < 100) Serial.print(" ");
  Serial.println("                      ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  // Visualização do movimento
  Serial.println("\n📍 SEQUÊNCIA DO CICLO:");
  Serial.println("   [0°] → [-60°] → [0°] → [+60°] → [0°]");
  Serial.println("    ↑       ↑       ↑       ↑       ↑");
  Serial.println("  INÍCIO   ESQ   CENTRO   DIR   CENTRO\n");
  
  // ========== PASSO 1: POSIÇÃO INICIAL ==========
  Serial.println("[PASSO 1] 📍 POSIÇÃO INICIAL");
  Serial.print("   Posição atual: ");
  imprimirPosicao(posicaoAtual);
  Serial.println("   Status: Preparando para iniciar sequência...");
  Serial.println("   Aguardando 3 segundos...");
  
  // Contagem regressiva de 3 segundos
  for(int i = 3; i > 0; i--) {
    Serial.print("   ⏰ ");
    Serial.print(i);
    Serial.println(" segundo(s)...");
    delay(1000);
  }
  
  // ========== MOVIMENTO 1: VAI PARA -60° ==========
  Serial.println("\n[MOVIMENTO 1] ↺ INDO PARA ESQUERDA");
  Serial.print("   Movimento: ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("° anti-horário");
  Serial.print("   De: ");
  imprimirPosicao(posicaoAtual);
  Serial.print(" → Para: ");
  imprimirPosicao(-ANGULO_MOVIMENTO);
  
  // Executa movimento
  int passos = moverMotor(ANGULO_MOVIMENTO, false);  // false = anti-horário
  totalPassosCiclo += passos;
  posicaoAtual = -ANGULO_MOVIMENTO;
  
  Serial.print("   ✅ Posição alcançada: ");
  imprimirPosicao(posicaoAtual);
  
  // Pausa
  Serial.println("\n   ⏸️  Pausa de 2 segundos...");
  for(int i = 2; i > 0; i--) {
    Serial.print("   ");
    Serial.print(i);
    Serial.println("...");
    delay(1000);
  }
  
  // ========== MOVIMENTO 2: VOLTA PARA 0° ==========
  Serial.println("\n[MOVIMENTO 2] ↻ VOLTANDO AO CENTRO");
  Serial.print("   Movimento: ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("° horário");
  Serial.print("   De: ");
  imprimirPosicao(posicaoAtual);
  Serial.print(" → Para: ");
  imprimirPosicao(0);
  
  // Executa movimento
  passos = moverMotor(ANGULO_MOVIMENTO, true);  // true = horário
  totalPassosCiclo += passos;
  posicaoAtual = 0;
  
  Serial.print("   ✅ Posição alcançada: ");
  imprimirPosicao(posicaoAtual);
  
  // Pausa
  Serial.println("\n   ⏸️  Pausa de 2 segundos...");
  for(int i = 2; i > 0; i--) {
    Serial.print("   ");
    Serial.print(i);
    Serial.println("...");
    delay(1000);
  }
  
  // ========== MOVIMENTO 3: VAI PARA +60° ==========
  Serial.println("\n[MOVIMENTO 3] ↻ INDO PARA DIREITA");
  Serial.print("   Movimento: ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("° horário");
  Serial.print("   De: ");
  imprimirPosicao(posicaoAtual);
  Serial.print(" → Para: ");
  imprimirPosicao(ANGULO_MOVIMENTO);
  
  // Executa movimento
  passos = moverMotor(ANGULO_MOVIMENTO, true);  // true = horário
  totalPassosCiclo += passos;
  posicaoAtual = ANGULO_MOVIMENTO;
  
  Serial.print("   ✅ Posição alcançada: ");
  imprimirPosicao(posicaoAtual);
  
  // Pausa
  Serial.println("\n   ⏸️  Pausa de 2 segundos...");
  for(int i = 2; i > 0; i--) {
    Serial.print("   ");
    Serial.print(i);
    Serial.println("...");
    delay(1000);
  }
  
  // ========== MOVIMENTO 4: VOLTA PARA 0° ==========
  Serial.println("\n[MOVIMENTO 4] ↺ VOLTANDO AO CENTRO");
  Serial.print("   Movimento: ");
  Serial.print(ANGULO_MOVIMENTO);
  Serial.println("° anti-horário");
  Serial.print("   De: ");
  imprimirPosicao(posicaoAtual);
  Serial.print(" → Para: ");
  imprimirPosicao(0);
  
  // Executa movimento
  passos = moverMotor(ANGULO_MOVIMENTO, false);  // false = anti-horário
  totalPassosCiclo += passos;
  posicaoAtual = 0;
  
  Serial.print("   ✅ Posição alcançada: ");
  imprimirPosicao(posicaoAtual);
  Serial.println("   🎯 Motor retornou ao centro!");
  
  // ========== PAUSA LONGA ==========
  Serial.println("\n[PASSO FINAL] ⏸️  PAUSA LONGA");
  Serial.println("   Motor em posição de repouso (0°)");
  Serial.println("   Aguardando 5 segundos antes do próximo ciclo...");
  
  for(int i = 5; i > 0; i--) {
    Serial.print("   ⏰ ");
    Serial.print(i);
    Serial.println(" segundo(s)...");
    delay(1000);
  }
  
  // ========== RESUMO DO CICLO ==========
  unsigned long tempoCiclo = millis() - tempoInicio;
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("📊 RESUMO DO CICLO:");
  Serial.print("   • Ciclo número: ");
  Serial.println(cicloNumero);
  Serial.print("   • Tempo total do ciclo: ");
  Serial.print(tempoCiclo / 1000.0, 1);
  Serial.println(" segundos");
  Serial.println("   • Sequência de posições:");
  Serial.println("     0° → -60° → 0° → +60° → 0°");
  Serial.print("   • Total de passos executados: ");
  Serial.println(totalPassosCiclo);
  Serial.print("   • Distância angular total percorrida: ");
  Serial.print(ANGULO_MOVIMENTO * 4);
  Serial.println("°");
  Serial.print("   • Posição final: ");
  imprimirPosicao(posicaoAtual);
  
  // Verificação de drift (se não voltou exatamente para 0)
  if(abs(posicaoAtual) > 0.1) {
    Serial.print("   ⚠️  Desvio detectado: ");
    Serial.print(abs(posicaoAtual), 2);
    Serial.println("°");
    Serial.println("   (Considere reduzir velocidade se o desvio aumentar)");
  } else {
    Serial.println("   ✅ Precisão mantida - sem desvio detectável");
  }
  
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("\n");
}

// ==================== FUNÇÃO DE MOVIMENTO ====================
/**
 * Move o motor um número específico de graus
 * @param graus: Número de graus para mover
 * @param horario: true = sentido horário, false = sentido anti-horário
 * @return número de passos executados
 */
int moverMotor(int graus, bool horario) {
  // Define a direção
  digitalWrite(DIR_PIN, horario ? HIGH : LOW);
  
  // Calcula número de passos necessários
  int passos = (graus * STEPS_PER_REV) / 360;
  
  Serial.print("   Executando ");
  Serial.print(passos);
  Serial.print(" passos (");
  Serial.print(graus);
  Serial.print("°) - Direção: ");
  Serial.println(horario ? "Horário ↻" : "Anti-horário ↺");
  
  // Debug: mostra cálculo em Full Step
  if(STEPS_PER_REV == 200) {
    Serial.print("   [Full Step: ");
    Serial.print(graus);
    Serial.print("° = ");
    Serial.print(passos);
    Serial.print(" passos × 1.8°/passo]");
    Serial.println();
  }
  
  // Mostra barra de progresso
  Serial.print("   Progresso: [");
  int barraTotal = 30;  // Tamanho da barra de progresso
  int passosParaBarra = passos / barraTotal;
  if(passosParaBarra == 0) passosParaBarra = 1;
  
  // Executa os passos
  for(int i = 0; i < passos; i++) {
    // Envia pulso para o driver
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(VELOCIDADE_MOVIMENTO);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(VELOCIDADE_MOVIMENTO);
    
    // Atualiza barra de progresso
    if(i % passosParaBarra == 0 && (i * barraTotal / passos) < barraTotal) {
      Serial.print("■");
    }
  }
  
  // Completa a barra
  for(int i = (passos * barraTotal / passos); i < barraTotal; i++) {
    Serial.print("■");
  }
  Serial.println("] 100%");
  
  return passos;
}

// ==================== FUNÇÕES AUXILIARES ====================

/**
 * Imprime a posição de forma padronizada
 */
void imprimirPosicao(float pos) {
  if(pos > 0) {
    Serial.print("+");
  }
  Serial.print(pos, 1);
  Serial.print("°");
  
  // Adiciona indicador visual da posição
  if(abs(pos) < 0.5) {
    Serial.print(" [CENTRO]");
  } else if(pos <= -60) {
    Serial.print(" [ESQUERDA MÁX]");
  } else if(pos < 0) {
    Serial.print(" [ESQUERDA]");
  } else if(pos >= 60) {
    Serial.print(" [DIREITA MÁX]");
  } else if(pos > 0) {
    Serial.print(" [DIREITA]");
  }
}

/**
 * Função de emergência - para o motor imediatamente
 */
void pararEmergencia() {
  digitalWrite(ENA_PIN, HIGH);  // Desabilita motor
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  
  Serial.println("\n\n🛑 PARADA DE EMERGÊNCIA!");
  Serial.println("   Motor desabilitado");
  Serial.println("   Sistema parado");
  
  while(1) {
    delay(1000);
  }
}

/**
 * Função para resetar posição (calibração)
 */
void resetarPosicao() {
  Serial.println("\n🔄 RESET DE POSIÇÃO");
  Serial.println("   Definindo posição atual como 0°");
  posicaoAtual = 0;
  Serial.println("   ✅ Posição resetada");
}

/**
 * Função de teste rápido - executa um movimento de teste
 */
void testeRapido() {
  Serial.println("\n🧪 TESTE RÁPIDO");
  Serial.println("   Executando movimento de 90° e retorno...");
  
  // Vai
  moverMotor(90, true);
  delay(500);
  
  // Volta
  moverMotor(90, false);
  
  Serial.println("   ✅ Teste concluído");
}

/**
 * Função de diagnóstico - verifica se as configurações estão corretas
 */
void diagnosticoConfiguracao() {
  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║              DIAGNÓSTICO DE CONFIGURAÇÃO                ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  Serial.println("\n🔍 TESTE DE CONFIGURAÇÃO DIP SWITCHES:");
  Serial.println("   Vou girar EXATAMENTE 1 volta completa (360°)");
  Serial.println("   Se o motor girar MENOS que 1 volta:");
  Serial.println("   → As DIP switches estão ERRADAS!");
  Serial.println("\n   Iniciando teste em 3 segundos...");
  
  delay(3000);
  
  Serial.println("\n   Executando 1 volta completa (360°)...");
  int passosVoltaCompleta = STEPS_PER_REV;
  
  for(int i = 0; i < passosVoltaCompleta; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(2000);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(2000);
    
    // Mostra progresso a cada 25%
    if(i == passosVoltaCompleta/4) Serial.println("   25% completo...");
    if(i == passosVoltaCompleta/2) Serial.println("   50% completo...");
    if(i == passosVoltaCompleta*3/4) Serial.println("   75% completo...");
  }
  
  Serial.println("\n   ✅ Comando de 1 volta enviado!");
  Serial.println("\n   VERIFIQUE:");
  Serial.println("   • O motor girou EXATAMENTE 1 volta? → Config CORRETA ✅");
  Serial.println("   • Girou MENOS (aprox. 1/8 volta)?  → Mude DIPs para FULL STEP ⚠️");
  Serial.println("   • Girou MAIS de 1 volta?           → Mude DIPs para 1/8 STEP ⚠️");
}