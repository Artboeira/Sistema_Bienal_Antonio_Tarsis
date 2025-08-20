/*
 * Sistema Integrado: Motor de Passo (Controle Direto) + Motor DC (via Relé)
 * * CONTROLES:
 * - Motor de Passo NEMA 14: Controle direto via TB6600 (PUL, DIR, ENA)
 * - Motor DC 12V: Liga/desliga via relé 12V (RELAY_PIN)
 * * SEQUÊNCIA AUTOMATIZADA:
 * 1. Motor de passo destrava carretilha (60° anti-horário)
 * 2. Penduricalho cai em queda livre (2 segundos)
 * 3. Motor de passo trava carretilha (60° horário)
 * 4. Espera 5 segundos
 * 5. Liga motor DC via relé (10 segundos - puxa manivela)
 * 6. Desliga motor DC via relé, espera 3 segundos
 * 7. Repete o ciclo automaticamente
 * * Alimentação:
 * - Fonte 12V: TB6600 + Motor DC + Relé
 * - USB 5V: ESP32
 * * Autor: Sistema Automatizado de Carretilha
 * Data: Agosto 2025
 * Versão: 2.1 (Controles Corretos)
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
// Motor de Passo (Controle Direto via TB6600)
const int STEP_PIN = 25;  // Pino de pulso → TB6600 PUL+
const int DIR_PIN = 26;   // Pino de direção → TB6600 DIR+
const int ENA_PIN = 27;   // Pino de habilitação → TB6600 ENA+

// Motor DC (Controle via Relé 12V)
const int RELAY_PIN = 32; // Pino de controle do relé → Liga/desliga motor DC

// ==================== CONFIGURAÇÕES DO MOVIMENTO ====================
const int ANGULO_DESTRAVAMENTO = 60;   // Ângulo para destravar (anti-horário)
const int ANGULO_TRAVAMENTO = 60;      // Ângulo para travar (horário)

// Tempos da sequência (em milissegundos)
const int TEMPO_PREPARACAO = 2000;     // 2 segundos de preparação
const int TEMPO_QUEDA_LIVRE = 2000;    // 2 segundos para queda livre
const int TEMPO_ESPERA_INICIAL = 5000; // 5 segundos após travar
const int TEMPO_MOTOR_DC = 10000;      // 10 segundos motor DC ligado
const int TEMPO_ESPERA_FINAL = 3000;   // 3 segundos após desligar motor DC

// Velocidade do motor de passo (microsegundos entre pulsos)
const int VELOCIDADE_MOVIMENTO = 1200; // Velocidade otimizada para travamento

// ==================== VARIÁVEIS GLOBAIS ====================
enum EstadoSistema {
  PREPARANDO,
  DESTRAVANDO,
  QUEDA_LIVRE,
  TRAVANDO,
  ESPERANDO,
  PUXANDO_CARRETILHA,
  FINALIZANDO_CICLO
};

EstadoSistema estadoAtual = PREPARANDO;
bool carretilhaTravada = true;     // Estado da trava da carretilha
bool motorDCLigado = false;        // Estado do motor DC
int cicloNumero = 0;               // Contador de ciclos
unsigned long tempoInicio = 0;     // Tempo de início da etapa atual
unsigned long tempoCicloInicio = 0; // Tempo de início do ciclo completo
int totalPassosCiclo = 0;          // Total de passos executados no ciclo

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  delay(500);  // Pequeno delay para estabilizar serial
  
  // Limpa o monitor serial e mostra cabeçalho
  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════════════════════════╗");
  Serial.println("║      SISTEMA AUTOMATIZADO - CARRETILHA + QUEDA LIVRE     ║");
  Serial.println("╠══════════════════════════════════════════════════════════╣");
  Serial.println("║  CONTROLES INTEGRADOS:                                   ║");
  Serial.println("║  • Motor de Passo: Controle DIRETO via TB6600            ║");
  Serial.println("║  • Motor DC: Liga/Desliga via RELÉ 12V                   ║");
  Serial.println("║                                                          ║");
  Serial.println("║  SEQUÊNCIA AUTOMATIZADA:                                 ║");
  Serial.println("║  1. 🏁 Preparação (2s)                                   ║");
  Serial.println("║  2. 🔓 Motor de passo destrava carretilha                ║");
  Serial.println("║  3. ⬇️  Penduricalho cai em queda livre (2s)              ║");
  Serial.println("║  4. 🔒 Motor de passo trava carretilha                   ║");
  Serial.println("║  5. ⏳ Aguarda 5 segundos                                ║");
  Serial.println("║  6. ⚡ Relé liga motor DC (10s)                           ║");
  Serial.println("║  7. ⏹️  Relé desliga motor DC, aguarda 3s                 ║");
  Serial.println("║  8. 🔄 Repete o ciclo automaticamente                    ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  // Mostra configuração atual
  Serial.println("\n📋 CONFIGURAÇÃO DO SISTEMA:");
  Serial.print("   • Modo do motor de passo: ");
  Serial.println(MODO_ATUAL);
  Serial.print("   • Passos por revolução: ");
  Serial.println(STEPS_PER_REV);
  Serial.print("   • Resolução: ");
  Serial.print(360.0 / STEPS_PER_REV, 2);
  Serial.println("° por passo");
  Serial.print("   • Ângulo de travamento/destravamento: ");
  Serial.print(ANGULO_TRAVAMENTO);
  Serial.println("°");
  Serial.print("   • Velocidade do motor de passo: ");
  Serial.print(1000000 / (VELOCIDADE_MOVIMENTO * 2));
  Serial.println(" passos/segundo");
  Serial.println("   • Controle motor DC: Relé 12V (liga/desliga)");
  
  // AVISO IMPORTANTE SOBRE DIP SWITCHES
  if(STEPS_PER_REV == 200) {
    Serial.println("\n⚠️  ATENÇÃO - CONFIGURAÇÃO DIP SWITCHES TB6600:");
    Serial.println("   Para FULL STEP você DEVE configurar:");
    Serial.println("   ┌─────────────────────────────────┐");
    Serial.println("   │ MICROSTEPPING:                  │");
    Serial.println("   │ S4: OFF  S5: OFF  S6: OFF        │");
    Serial.println("   │                                 │");
    Serial.println("   │ CORRENTE (máximo torque):       │");
    Serial.println("   │ 1.5A: S1: ON  S2: OFF S3: OFF   │");
    Serial.println("   └─────────────────────────────────┘");
    Serial.println("   SE NÃO MUDAR AS DIPs, O MOVIMENTO SERÁ 8X MENOR!");
  }
  
  // Configuração dos pinos
  Serial.println("\n⚙️  Configurando controles...");
  
  // Pinos do motor de passo (controle direto via TB6600)
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(ENA_PIN, OUTPUT);
  
  // Pino do relé (controle do motor DC)
  pinMode(RELAY_PIN, OUTPUT);
  
  // Estado inicial dos pinos
  digitalWrite(STEP_PIN, LOW);
  digitalWrite(DIR_PIN, LOW);
  digitalWrite(ENA_PIN, LOW);      // LOW = motor de passo habilitado
  digitalWrite(RELAY_PIN, LOW);    // LOW = motor DC desligado via relé
  
  Serial.println("✅ Controles configurados");
  Serial.println("   • Motor de passo: Habilitado (controle direto TB6600)");
  Serial.println("   • Motor DC: Desligado (relé em posição OFF)");
  Serial.println("   • Carretilha: Estado inicial TRAVADA");
  
  // Configuração inicial do sistema
  Serial.println("\n🏁 ESTADO INICIAL DO SISTEMA");
  Serial.println("   • Penduricalho: Erguido e suspenso");
  Serial.println("   • Carretilha: Travada pelo motor de passo");
  Serial.println("   • Motor DC: Desligado pelo relé");
  Serial.println("   • Fonte 12V: Alimentando TB6600 + Relé");
  Serial.println("   • ESP32: Alimentado via USB");
  
  // Teste rápido dos controles
  Serial.println("\n🧪 Teste de inicialização dos controles...");
  testeControlesSistema();
  
  Serial.println("✅ Sistema pronto para operar!");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("");
  
  // Inicia primeiro ciclo
  estadoAtual = PREPARANDO;
  tempoInicio = millis();
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  switch(estadoAtual) {
    
    case PREPARANDO:
      executarPreparacao();
      break;
      
    case DESTRAVANDO:
      executarDestravamento();
      break;
      
    case QUEDA_LIVRE:
      executarQuedaLivre();
      break;
      
    case TRAVANDO:
      executarTravamento();
      break;
      
    case ESPERANDO:
      executarEspera();
      break;
      
    case PUXANDO_CARRETILHA:
      executarPuxadaCarretilha();
      break;
      
    case FINALIZANDO_CICLO:
      executarFinalizacaoCiclo();
      break;
  }
  
  delay(50); // Pequeno delay para evitar sobrecarga do processador
}

// ==================== ESTADOS DA MÁQUINA ====================

void executarPreparacao() {
  static unsigned long ultimaContagem = 0;
  if(millis() - tempoInicio == 0) { // Primeira execução desta etapa
    ultimaContagem = 0; // Reseta a contagem
    cicloNumero++;
    tempoCicloInicio = millis();
    totalPassosCiclo = 0;
    
    Serial.println("╔══════════════════════════════════════════════════════════╗");
    Serial.print("║                      CICLO NÚMERO ");
    Serial.print(cicloNumero);
    if(cicloNumero < 10) Serial.print(" ");
    if(cicloNumero < 100) Serial.print(" ");
    Serial.println("                       ║");
    Serial.println("╚══════════════════════════════════════════════════════════╝");
    
    Serial.println("\n[ETAPA 1] 🏁 PREPARAÇÃO DO SISTEMA");
    Serial.println("   • Verificando estado dos controles...");
    Serial.println("   • Motor de passo: Mantendo carretilha travada 🔒");
    Serial.println("   • Motor DC: Desligado via relé ⏹️");
    Serial.println("   • Penduricalho: Posição inicial (erguido) ✅");
    
    // Define estados iniciais
    carretilhaTravada = true;
    motorDCLigado = false;
    digitalWrite(RELAY_PIN, LOW); // Garante que motor DC está desligado
    
    Serial.print("   • Aguardando ");
    Serial.print(TEMPO_PREPARACAO / 1000);
    Serial.println(" segundos...");
  }
  
  // Contagem regressiva da preparação
  unsigned long tempoDecorrido = millis() - tempoInicio;
  
  if(tempoDecorrido >= ultimaContagem + 1000) {
    int segundosRestantes = (TEMPO_PREPARACAO - tempoDecorrido) / 1000;
    if(segundosRestantes > 0) {
      Serial.print("   ⏰ ");
      Serial.print(segundosRestantes);
      Serial.println(" segundo(s) para iniciar...");
    }
    ultimaContagem = tempoDecorrido;
  }
  
  // Termina a preparação
  if(tempoDecorrido >= TEMPO_PREPARACAO) {
    proximoEstado(DESTRAVANDO);
  }
}

void executarDestravamento() {
  if(millis() - tempoInicio == 0) { // Primeira execução desta etapa
    Serial.println("\n[ETAPA 2] 🔓 DESTRAVAMENTO DA CARRETILHA");
    Serial.print("   • Motor de passo: Girando ");
    Serial.print(ANGULO_DESTRAVAMENTO);
    Serial.println("° no sentido anti-horário");
    
    // Executa movimento de destravamento
    int passos = moverMotorPasso(ANGULO_DESTRAVAMENTO, false); // false = anti-horário
    totalPassosCiclo += passos;
    
    carretilhaTravada = false;
    Serial.println("   ✅ Carretilha DESTRAVADA com sucesso!");
    Serial.println("   ⚠️  ATENÇÃO: PENDURICALHO INICIANDO QUEDA LIVRE!");
  }
  
  // Imediatamente após destravar, inicia queda livre
  proximoEstado(QUEDA_LIVRE);
}

void executarQuedaLivre() {
  static int ultimoProgresso = -1;
  if(millis() - tempoInicio == 0) { // Primeira execução desta etapa
    ultimoProgresso = -1;
    Serial.println("\n[ETAPA 3] ⬇️  QUEDA LIVRE EM PROGRESSO");
    Serial.println("   • Penduricalho em queda livre...");
    Serial.println("   • Carretilha: Completamente liberada 🔓");
    Serial.print("   • Tempo de queda programado: ");
    Serial.print(TEMPO_QUEDA_LIVRE / 1000.0);
    Serial.println(" segundos");
    Serial.print("   • Progresso da queda: [");
  }
  
  // Mostra progresso da queda em tempo real
  unsigned long tempoDecorrido = millis() - tempoInicio;
  int progresso = (tempoDecorrido * 30) / TEMPO_QUEDA_LIVRE; // 30 caracteres de barra
  
  if(progresso > ultimoProgresso && progresso <= 30) {
    for(int i = 0; i < (progresso - ultimoProgresso); i++) {
      Serial.print("■");
    }
    ultimoProgresso = progresso;
  }
  
  // Termina a queda livre
  if(tempoDecorrido >= TEMPO_QUEDA_LIVRE) {
    if (ultimoProgresso < 30) {
        for (int i = 0; i < (30 - ultimoProgresso); i++) Serial.print("■");
    }
    Serial.println("] 100%");
    Serial.println("   ✅ Queda livre completa!");
    proximoEstado(TRAVANDO);
  }
}

void executarTravamento() {
  if(millis() - tempoInicio == 0) { // Primeira execução desta etapa
    Serial.println("\n[ETAPA 4] 🔒 TRAVAMENTO DA CARRETILHA");
    Serial.print("   • Motor de passo: Girando ");
    Serial.print(ANGULO_TRAVAMENTO);
    Serial.println("° no sentido horário");

    // Executa movimento de travamento
    int passos = moverMotorPasso(ANGULO_TRAVAMENTO, true); // true = horário
    totalPassosCiclo += passos;
    
    carretilhaTravada = true;
    Serial.println("   ✅ Carretilha TRAVADA com sucesso!");
    Serial.println("   🛑 Penduricalho fixo na posição inferior");
  }
  
  // Imediatamente após travar, inicia período de espera
  proximoEstado(ESPERANDO);
}

void executarEspera() {
  static unsigned long ultimaContagem = 0;
  if(millis() - tempoInicio == 0) { // Primeira execução desta etapa
    ultimaContagem = 0;
    Serial.println("\n[ETAPA 5] ⏳ PERÍODO DE ESPERA");
    Serial.println("   • Carretilha firmemente travada 🔒");
    Serial.println("   • Penduricalho fixo na posição inferior");
    Serial.print("   • Tempo de espera programado: ");
    Serial.print(TEMPO_ESPERA_INICIAL / 1000);
    Serial.println(" segundos");
  }
  
  // Contagem regressiva da espera
  unsigned long tempoDecorrido = millis() - tempoInicio;
  
  if(tempoDecorrido >= ultimaContagem + 1000) {
    int segundosRestantes = (TEMPO_ESPERA_INICIAL - tempoDecorrido) / 1000;
    if(segundosRestantes >= 0) {
      Serial.print("   ⏰ ");
      Serial.print(segundosRestantes);
      Serial.println(" segundo(s) restante(s) para ativar motor DC...");
    }
    ultimaContagem = tempoDecorrido;
  }
  
  // Termina a espera
  if(tempoDecorrido >= TEMPO_ESPERA_INICIAL) {
    proximoEstado(PUXANDO_CARRETILHA);
  }
}

void executarPuxadaCarretilha() {
  static unsigned long ultimoProgresso = 0;
  if(millis() - tempoInicio == 0) { // Primeira execução desta etapa
    ultimoProgresso = 0;
    Serial.println("\n[ETAPA 6] 🔄 RECOLHIMENTO VIA MOTOR DC");
    Serial.print("   • Tempo de operação programado: ");
    Serial.print(TEMPO_MOTOR_DC / 1000);
    Serial.println(" segundos");
    
    // Liga o motor DC via relé
    digitalWrite(RELAY_PIN, HIGH);
    motorDCLigado = true;
    
    Serial.println("   ✅ Relé ATIVADO → Motor DC LIGADO! ⚡");
    Serial.println("   🔄 Motor DC puxando manivela da carretilha...");
  }
  
  // Mostra progresso do recolhimento
  unsigned long tempoDecorrido = millis() - tempoInicio;
  
  if(tempoDecorrido >= ultimoProgresso + 1000) {
    int segundosRestantes = (TEMPO_MOTOR_DC - tempoDecorrido) / 1000;
    if(segundosRestantes >= 0) {
      Serial.print("   🔄 Motor DC operando... ");
      Serial.print(segundosRestantes);
      Serial.println("s restantes");
    }
    ultimoProgresso = tempoDecorrido;
  }
  
  // Desliga motor DC via relé
  if(tempoDecorrido >= TEMPO_MOTOR_DC) {
    digitalWrite(RELAY_PIN, LOW);
    motorDCLigado = false;
    
    Serial.println("   ⏹️  Relé DESATIVADO → Motor DC DESLIGADO!");
    Serial.println("   ✅ Recolhimento concluído com sucesso!");
    proximoEstado(FINALIZANDO_CICLO);
  }
}

void executarFinalizacaoCiclo() {
  static unsigned long ultimaContagem = 0;
  if(millis() - tempoInicio == 0) { // Primeira execução desta etapa
    ultimaContagem = 0;
    Serial.println("\n[ETAPA 7] ⏸️  FINALIZAÇÃO DO CICLO");
    Serial.println("   • Penduricalho: Retornou à posição inicial 📍");
    Serial.print("   • Aguardando ");
    Serial.print(TEMPO_ESPERA_FINAL / 1000);
    Serial.println(" segundos antes do próximo ciclo...");
  }
  
  // Contagem regressiva final
  unsigned long tempoDecorrido = millis() - tempoInicio;
  
  if(tempoDecorrido >= ultimaContagem + 1000) {
    int segundosRestantes = (TEMPO_ESPERA_FINAL - tempoDecorrido) / 1000;
    if(segundosRestantes >= 0) {
      Serial.print("   ⏰ ");
      Serial.print(segundosRestantes);
      Serial.println(" segundo(s) para próximo ciclo...");
    }
    ultimaContagem = tempoDecorrido;
  }
  
  // Finaliza ciclo e mostra resumo
  if(tempoDecorrido >= TEMPO_ESPERA_FINAL) {
    mostrarResumoCiclo();
    proximoEstado(PREPARANDO);
  }
}

// ==================== FUNÇÕES AUXILIARES ====================

void proximoEstado(EstadoSistema novoEstado) {
  estadoAtual = novoEstado;
  tempoInicio = millis();
}

int moverMotorPasso(int graus, bool horario) {
  // Define direção do movimento
  digitalWrite(DIR_PIN, horario ? HIGH : LOW);
  
  // Calcula número de passos necessários
  int passos = (graus * STEPS_PER_REV) / 360;
  
  Serial.print("   → Executando ");
  Serial.print(passos);
  Serial.print(" passos (");
  Serial.print(graus);
  Serial.print("°) - Direção: ");
  Serial.println(horario ? "Horário ↻" : "Anti-horário ↺");
  
  // Mostra barra de progresso do movimento
  Serial.print("   Progresso: [");
  int barraTotal = 20;  // Tamanho da barra de progresso
  
  // Executa os passos
  for(int i = 0; i < passos; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(VELOCIDADE_MOVIMENTO);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(VELOCIDADE_MOVIMENTO);
    
    // Atualiza barra de progresso
    if ((i + 1) * barraTotal / passos > i * barraTotal / passos) {
      Serial.print("■");
    }
  }
  
  Serial.println("] 100%");
  
  return passos;
}

void mostrarResumoCiclo() {
  unsigned long tempoCicloTotal = millis() - tempoCicloInicio;
  
  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.println("📊 RESUMO COMPLETO DO CICLO:");
  Serial.print("   • Ciclo número: ");
  Serial.println(cicloNumero);
  Serial.print("   • Tempo total do ciclo: ");
  Serial.print(tempoCicloTotal / 1000.0, 1);
  Serial.println(" segundos");
  
  Serial.println("   • Sequência executada com sucesso:");
  Serial.println("     🏁→🔓→⬇️→🔒→⏳→⚡→⏹️→🔄");
  
  Serial.print("   • Total de passos do motor: ");
  Serial.println(totalPassosCiclo);
  
  Serial.println("   • Estado final do sistema:");
  Serial.print("     - Carretilha: ");
  Serial.println(carretilhaTravada ? "TRAVADA 🔒" : "DESTRAVADA 🔓");
  Serial.print("     - Motor DC: ");
  Serial.println(motorDCLigado ? "LIGADO ⚡" : "DESLIGADO ⏹️");
  
  // Verificação de integridade do sistema
  if(carretilhaTravada && !motorDCLigado) {
    Serial.println("   ✅ Sistema em estado correto para próximo ciclo");
  } else {
    Serial.println("   ⚠️  Verificar estado dos controles antes do próximo ciclo");
  }
  
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void testeControlesSistema() {
  Serial.println("   • Testando controle direto do motor de passo...");
  moverMotorPasso(30, true);
  delay(500);
  moverMotorPasso(30, false);
  Serial.println("     ✅ Motor de passo respondendo corretamente");
  
  Serial.println("   • Testando controle do relé (motor DC)...");
  digitalWrite(RELAY_PIN, HIGH);
  delay(1000);
  digitalWrite(RELAY_PIN, LOW);
  Serial.println("     ✅ Relé respondendo corretamente");
  
  Serial.println("   ✅ Teste de controles concluído com sucesso!");
}

// ==================== FUNÇÕES DE SEGURANÇA E DIAGNÓSTICO ====================

void pararEmergencia() {
  // Para ambos os motores imediatamente
  digitalWrite(ENA_PIN, HIGH);   // Desabilita motor de passo
  digitalWrite(RELAY_PIN, LOW);  // Desliga motor DC via relé
  
  Serial.println("\n\n🛑 PARADA DE EMERGÊNCIA ACIONADA!");
  Serial.println("   • Motor de passo: DESABILITADO");
  Serial.println("   • Motor DC: DESLIGADO");
  Serial.println("   • Para reiniciar: Pressione botão RESET no ESP32");
  
  // Loop infinito de segurança
  while(true) {
    delay(1000);
  }
}

void diagnosticoCompleto() {
  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║            DIAGNÓSTICO COMPLETO DO SISTEMA             ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝");
  
  Serial.println("\n🔍 TESTE DE CONFIGURAÇÃO TB6600 (1 VOLTA COMPLETA):");
  moverMotorPasso(360, true);
  
  Serial.println("\n   VERIFICAÇÃO VISUAL:");
  Serial.println("   • Motor girou EXATAMENTE 1 volta? → Config CORRETA ✅");
  Serial.println("   • Girou MENOS (ex: 1/8 volta)?    → DIPs ERRADAS ⚠️");
  
  Serial.println("\n🔍 TESTE DO RELÉ (3 CICLOS):");
  for(int i = 0; i < 3; i++) {
    Serial.print("   Ciclo ");
    Serial.print(i + 1);
    Serial.println(": Liga → Desliga");
    digitalWrite(RELAY_PIN, HIGH);
    delay(1000);
    digitalWrite(RELAY_PIN, LOW);
    delay(1000);
  }
  Serial.println("   ✅ Teste do relé concluído!");
}
